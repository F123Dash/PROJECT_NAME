# Minty NS

An interactive network simulation and graph analysis platform that combines:

- High-performance C++ simulation and algorithm engines
- A Node.js backend bridge that orchestrates native binaries
- A React + Vite frontend for topology building, playback, and metrics visualization

## What This Project Does

Minty NS lets you model network topologies, run packet-flow simulations, and compare graph algorithms (Shortest Path and MST) using a visual interface.

Core capabilities include:

- Topology generation (tree, random) and manual topology editing
- Packet simulation with configurable traffic parameters
- Event playback timeline and queue-focused visualization
- Metrics reporting (latency, throughput, loss, jitter, hops)
- Algorithm analysis via native C++ tooling:
  - Dijkstra
  - Bellman-Ford
  - BFS distance
  - Kruskal MST
  - Prim MST
  - Path stretch comparison (MST path vs shortest path)
- Stress testing for large node counts

## Architecture

The stack is organized into three layers:

1. Frontend (React)
- Path: frontend/
- Main UI: frontend/src/App.jsx
- Uses React Flow for graph rendering and controls.

2. Backend (Node.js / Express)
- Path: backend/server.js
- Exposes REST endpoints used by the frontend.
- Spawns native C++ binaries and returns JSON results.

3. Native C++ Engines
- Main simulator binary: build/simulator
- Analysis binary: build/analysis-tool
- Additional backend-oriented runner: build/backend-runner
- Source code spans engine/, network/, graphs/, algorithms/, performance/, and src/.

## Repository Layout

High-level folders:

- algorithms/: shortest path, MST, routing utilities, metrics helpers
- backend/: Node.js API server
- config/: simulation config presets
- data_input/: runtime-generated config and topology inputs
- data_output/: runtime-generated JSON/CSV outputs
- engine/: simulation core, pipeline, events, integration logic
- frontend/: React UI (Vite)
- graphs/: graph containers and generators
- network/: transport/network behavior, queueing, logging, metrics
- performance/: profiler and stress testing support
- src/: C++ entry points (simulator main, backend runner, analysis tool)

## Prerequisites

- Linux or macOS (Linux recommended)
- Node.js 18+ and npm
- g++ with C++17 support
- make

## Quick Start

### 1) Install JavaScript dependencies

At repo root:

```bash
npm install
npm --prefix frontend install
```

### 2) Build native binaries

```bash
make release
```

This builds:

- build/simulator
- build/backend-runner
- build/analysis-tool

### 3) Start full development stack

```bash
npm run dev
```

This launches:

- Backend API on http://localhost:3000
- Frontend dev server on http://localhost:5173 (or next available Vite port)

### 4) Open the app

Navigate to the frontend URL shown in terminal output.

## Running Components Separately

Backend only:

```bash
npm run dev:backend
```

Frontend only:

```bash
npm run dev:frontend
```

Native simulator directly:

```bash
./build/simulator config/config.txt
```

## Configuration

Default config file: config/config.txt

Common keys:

- nodes: number of nodes
- topology: tree or random
- probability: edge probability for random topology
- traffic_type: constant, random, or burst
- rate: packets per second
- packet_size: bytes
- simulation_time: simulation window
- bandwidth: link bandwidth baseline
- delay: link delay baseline
- enable_retransmission, rtx_timeout_ms, max_retransmissions
- enable_profiling, enable_parallel_routing
- max_nodes

The backend can override these values per run.

## Backend API

### GET /config

Reads config/config.txt and returns parsed key/value pairs.

Example:

```bash
curl http://localhost:3000/config
```

### POST /run

Runs the C++ simulator.

Request body:

```json
{
  "configOverrides": {
    "nodes": 200,
    "topology": "tree",
    "traffic_type": "constant",
    "rate": 5,
    "packet_size": 512,
    "simulation_time": 30,
    "source": 0,
    "destination": 199
  },
  "topology": "0 1 2\n1 2 3\n2 3 1"
}
```

topology is optional and uses line format:

u v w

Response includes simulation metrics, events, console log tail/full, and topology edge data.

### POST /analyze

Runs build/analysis-tool for shortest path and MST analysis.

Request body:

```json
{
  "nodeCount": 5,
  "source": 0,
  "dest": 4,
  "edges": [
    { "from": 0, "to": 1, "weight": 2 },
    { "from": 1, "to": 4, "weight": 5 },
    { "from": 0, "to": 2, "weight": 1 }
  ]
}
```

Response includes:

- shortest_path
- mst_path
- kruskal
- prim
- complexity
- bfs_distance
- bellman_ford_no_neg_cycle

## Build Targets

Useful Makefile targets:

```bash
make
make debug
make release
make clean
make rebuild
make run
make run-debug
make help
```

## Output Artifacts

Generated during runs:

- data_output/result.json
- data_output/topology_edges.json (when exported)
- simulation_console.log
- simulation_events.csv
- simulation_events.log

## Troubleshooting

1. Backend fails with missing binary
- Run: make release
- Confirm files exist in build/.

2. Frontend cannot reach backend
- Verify backend is running on port 3000.
- Check browser console/network tab.

3. Port conflicts
- npm run dev attempts to free ports 3000/5173/5174/5175 first.
- If needed, manually stop stale processes.

4. Empty or malformed analysis response
- Ensure build/analysis-tool exists.
- Ensure /analyze edges payload uses valid node indices and weights.

## Development Notes

- Backend server is currently designed around backend/server.js and native binaries.
- Frontend algorithm panels rely on /analyze for authoritative C++ results.
- Source and destination should be kept consistent between simulation and analysis flows.

## License

ISC (see package.json).
