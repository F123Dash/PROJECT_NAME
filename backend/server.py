#!/usr/bin/env python3
import json
import os
import subprocess
import sys
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
FRONTEND = ROOT / "frontend"
RUNNER_CPP = ROOT / "backend" / "simulation_runner.cpp"
RUNNER_BIN = ROOT / "backend" / "simulation_runner"

SOURCES = [
    "backend/simulation_runner.cpp",
    "graphs/graphs.cpp",
    "graphs/graph_generator.cpp",
    "graphs/graph_analysis.cpp",
    "algorithms/shortest_path.cpp",
    "algorithms/routing_table.cpp",
    "algorithms/mst.cpp",
    "algorithms/union_find.cpp",
    "engine/integration.cpp",
    "engine/node.cpp",
    "engine/packet.cpp",
    "engine/event_queue.cpp",
    "engine/statistics.cpp",
    "network/network_layer.cpp",
    "network/metrics.cpp",
    "network/logger.cpp",
    "network/debug.cpp",
    "network/tcp.cpp",
    "network/udp.cpp",
]


def runner_is_stale() -> bool:
    if not RUNNER_BIN.exists():
        return True
    binary_mtime = RUNNER_BIN.stat().st_mtime
    return any((ROOT / source).stat().st_mtime > binary_mtime for source in SOURCES)


def compile_runner() -> None:
    if not runner_is_stale():
        return
    cmd = ["g++", "-std=c++17", "-O2", "-I.", *SOURCES, "-o", str(RUNNER_BIN)]
    subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True, text=True)


def number(value, fallback, cast=float):
    try:
        return cast(value)
    except (TypeError, ValueError):
        return fallback


def build_protocol(payload: dict) -> str:
    topology = payload.get("topology", "custom")
    nodes = payload.get("nodes", [])
    edges = payload.get("edges", [])
    settings = payload.get("settings", {})
    node_count = max(1, int(number(payload.get("nodeCount", len(nodes) or 4), 4, int)))

    if nodes:
        node_count = max(node_count, len(nodes))

    node_weights = []
    for index in range(node_count):
        default_weight = nodes[index].get("weight", 1) if index < len(nodes) and isinstance(nodes[index], dict) else 1
        node_weights.append(max(1, int(number(default_weight, 1, int))))

    lines = [
        f"TOPOLOGY {topology}",
        f"NODES {node_count}",
        f"PROBABILITY {number(settings.get('probability'), 0.5)}",
        f"ROWS {max(1, int(number(settings.get('rows'), 2, int)))}",
        f"COLS {max(1, int(number(settings.get('cols'), 2, int)))}",
        f"SOURCE {max(0, int(number(settings.get('source'), 0, int)))}",
        f"DESTINATION {max(0, int(number(settings.get('destination'), node_count - 1, int)))}",
        f"PACKETS {max(0, int(number(settings.get('packets'), 10, int)))}",
        f"PACKET_SIZE {max(1, int(number(settings.get('packetSize'), 512, int)))}",
        f"DEFAULT_BANDWIDTH {max(0.001, number(settings.get('bandwidth'), 10.0))}",
        f"DEFAULT_LATENCY {max(0.0, number(settings.get('latency'), 1.0))}",
        "NODE_WEIGHTS " + str(len(node_weights)) + " " + " ".join(str(weight) for weight in node_weights),
    ]

    clean_edges = []
    for edge in edges:
        if not isinstance(edge, dict):
            continue
        u = int(number(edge.get("u"), 0, int))
        v = int(number(edge.get("v"), 0, int))
        if u == v or u < 0 or v < 0 or u >= node_count or v >= node_count:
            continue
        clean_edges.append((
            u,
            v,
            max(1, int(number(edge.get("weight"), 1, int))),
            max(0.001, number(edge.get("bandwidth"), settings.get("bandwidth", 10.0))),
            max(0.0, number(edge.get("latency"), settings.get("latency", 1.0))),
        ))

    lines.append(f"EDGES {len(clean_edges)}")
    for u, v, weight, bandwidth, latency in clean_edges:
        lines.append(f"{u} {v} {weight} {bandwidth} {latency}")
    lines.append("END")
    return "\n".join(lines) + "\n"


class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.end_headers()

    def do_POST(self):
        if self.path != "/api/run":
            self.send_error(404, "Unknown endpoint")
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
            payload = json.loads(self.rfile.read(length) or b"{}")
            compile_runner()
            protocol = build_protocol(payload)
            proc = subprocess.run(
                [str(RUNNER_BIN)],
                input=protocol,
                cwd=ROOT,
                capture_output=True,
                text=True,
                timeout=20,
            )
            if proc.returncode != 0:
                try:
                    data = json.loads(proc.stdout)
                except json.JSONDecodeError:
                    data = {"ok": False, "error": proc.stderr or proc.stdout or "Simulation failed"}
            else:
                data = json.loads(proc.stdout)
            self.send_json(data)
        except subprocess.CalledProcessError as exc:
            self.send_json({"ok": False, "error": exc.stderr or str(exc)}, status=500)
        except Exception as exc:
            self.send_json({"ok": False, "error": str(exc)}, status=500)

    def do_GET(self):
        if self.path.startswith("/api/file/"):
            relative = unquote(self.path.removeprefix("/api/file/"))
            path = (ROOT / relative).resolve()
            if ROOT not in path.parents and path != ROOT:
                self.send_error(403)
                return
            if not path.exists() or not path.is_file():
                self.send_error(404)
                return
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(path.read_bytes())
            return

        requested = self.path.split("?", 1)[0]
        if requested == "/":
            requested = "/index.html"
        path = (FRONTEND / requested.lstrip("/")).resolve()
        if FRONTEND not in path.parents and path != FRONTEND:
            self.send_error(403)
            return
        if not path.exists() or not path.is_file():
            self.send_error(404)
            return
        return self.serve_path(path)

    def serve_path(self, path: Path):
        content_types = {
            ".html": "text/html; charset=utf-8",
            ".css": "text/css; charset=utf-8",
            ".js": "application/javascript; charset=utf-8",
        }
        self.send_response(200)
        self.send_header("Content-Type", content_types.get(path.suffix, "application/octet-stream"))
        self.end_headers()
        self.wfile.write(path.read_bytes())

    def send_json(self, data, status=200):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    port = int(os.environ.get("PORT", "8080"))
    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"Network simulator web app: http://127.0.0.1:{port}")
    server.serve_forever()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
