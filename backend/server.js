const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

const app = express();
app.use(bodyParser.json({ limit: "50mb" }));
app.use(cors());

app.use(express.static(path.join(__dirname, "frontend", "dist")));

const projectRoot = path.join(__dirname, "..");
const dataInputDir = path.join(projectRoot, "data_input");
const dataOutputDir = path.join(projectRoot, "data_output");
if (!fs.existsSync(dataInputDir)) fs.mkdirSync(dataInputDir, { recursive: true });
if (!fs.existsSync(dataOutputDir)) fs.mkdirSync(dataOutputDir, { recursive: true });

const simPath = path.join(projectRoot, "build", "simulator");
const defaultConfigPath = path.join(projectRoot, "config", "config.txt");

// ─── GET /config — read config/config.txt ───
app.get("/config", (req, res) => {
    try {
        const raw = fs.readFileSync(defaultConfigPath, "utf-8");
        const params = {};
        raw.split("\n").forEach(line => {
            const l = line.trim();
            if (!l || l.startsWith("#")) return;
            const eq = l.indexOf("=");
            if (eq > 0) params[l.substring(0, eq).trim()] = l.substring(eq + 1).trim();
        });
        return res.json({ ok: true, config: params, raw });
    } catch (err) {
        return res.json({ error: err.message });
    }
});

// ─── POST /run — run the C++ simulator ───
// Uses spawn() instead of exec() to avoid maxBuffer overflow
app.post("/run", (req, res) => {
    const { configOverrides, topology } = req.body;

    try {
        // 1. Build config file
        let raw = fs.readFileSync(defaultConfigPath, "utf-8");
        if (configOverrides && typeof configOverrides === "object") {
            const params = {};
            raw.split("\n").forEach(line => {
                const l = line.trim();
                if (!l || l.startsWith("#")) return;
                const eq = l.indexOf("=");
                if (eq > 0) params[l.substring(0, eq).trim()] = l.substring(eq + 1).trim();
            });
            for (let key in configOverrides) params[key] = configOverrides[key];
            raw = "# Auto-generated config\n";
            for (let k in params) raw += `${k}=${params[k]}\n`;
        }
        const runConfigPath = path.join(dataInputDir, "config.txt");
        fs.writeFileSync(runConfigPath, raw);

        // Parse config for node count
        const cfgParams = {};
        raw.split("\n").forEach(line => {
            const l = line.trim();
            if (!l || l.startsWith("#")) return;
            const eq = l.indexOf("=");
            if (eq > 0) cfgParams[l.substring(0, eq).trim()] = l.substring(eq + 1).trim();
        });
        const nodeCount = parseInt(cfgParams.nodes) || 10;

        // 2. Determine args
        const args = [runConfigPath];
        if (topology && topology.trim().length > 0) {
            const topoPath = path.join(dataInputDir, "topology.txt");
            fs.writeFileSync(topoPath, topology);
            args.push(topoPath);
        }

        // 3. Spawn the simulator — avoids any buffer overflow
        const child = spawn(simPath, args, { cwd: projectRoot });

        // Drain stdout to prevent backpressure (C++ TeeStreamBuffer writes to simulation_console.log)
        child.stdout.resume();

        let stderrData = "";
        child.stderr.on("data", d => { stderrData += d.toString(); });

        child.on("close", (code) => {
            if (code !== 0) {
                return res.json({ error: stderrData || `Simulator exited with code ${code}` });
            }

            try {
                let payload = { nodeCount };

                // Read result.json
                const resultPath = path.join(dataOutputDir, "result.json");
                if (fs.existsSync(resultPath)) {
                    try {
                        const parsed = JSON.parse(fs.readFileSync(resultPath, "utf-8"));
                        payload = { ...payload, ...parsed, nodeCount };
                    } catch (e) {
                        payload.parseError = e.message;
                    }
                } else {
                    payload.error = "No result.json produced.";
                }

                // Read full console log
                const logPath = path.join(projectRoot, "simulation_console.log");
                if (fs.existsSync(logPath)) {
                    payload.consoleLog = fs.readFileSync(logPath, "utf-8");
                }

                // Read FULL topology edges exported by C++ simulator
                const topoEdgesPath = path.join(dataOutputDir, "topology_edges.json");
                if (fs.existsSync(topoEdgesPath)) {
                    try {
                        payload.topology_edges = JSON.parse(fs.readFileSync(topoEdgesPath, "utf-8"));
                    } catch (e) {
                        // fallback: extract from CSV
                        const csvPath = path.join(projectRoot, "simulation_events.csv");
                        if (fs.existsSync(csvPath)) {
                            const edgeSet = new Set();
                            const csvLines = fs.readFileSync(csvPath, "utf-8").split("\n");
                            for (let i = 1; i < csvLines.length; i++) {
                                const parts = csvLines[i].split(",");
                                if (parts.length < 7) continue;
                                const evType = parts[2].trim();
                                if (evType === "FORWARDED" || evType === "ROUTING") {
                                    const from = parseInt(parts[5]);
                                    const to = parseInt(parts[6]);
                                    if (!isNaN(from) && !isNaN(to) && from >= 0 && to >= 0 && from !== to) {
                                        edgeSet.add(Math.min(from, to) + "-" + Math.max(from, to));
                                    }
                                }
                            }
                            payload.topology_edges = [...edgeSet].map(k => {
                                const [a, b] = k.split("-").map(Number);
                                return { from: a, to: b, weight: 1 };
                            });
                        }
                    }
                }

                return res.json(payload);
            } catch (err) {
                return res.json({ error: err.message });
            }
        });

        child.on("error", (err) => {
            return res.json({ error: "Failed to start simulator: " + err.message });
        });

    } catch (error) {
        res.json({ error: error.message });
    }
});
// ─── POST /analyze — run the C++ analysis tool ───
app.post("/analyze", (req, res) => {
    const { nodeCount, source, dest, edges } = req.body;
    try {
        const topoPath = path.join(dataInputDir, "topology_edges.json");
        fs.writeFileSync(topoPath, JSON.stringify(edges));

        const analysisPath = path.join(projectRoot, "build", "analysis-tool");
        const args = [`${nodeCount}`, `${source}`, `${dest}`, topoPath];

        const child = spawn(analysisPath, args, { cwd: projectRoot });
        let stdoutData = "";
        let stderrData = "";

        child.stdout.on("data", d => { stdoutData += d.toString(); });
        child.stderr.on("data", d => { stderrData += d.toString(); });

        child.on("close", (code) => {
            if (code !== 0) {
                return res.json({ error: stderrData || `Analysis tool exited with code ${code}` });
            }
            try {
                // Find "{", as there might be other prints, though analysis_tool just prints JSON.
                const startIdx = stdoutData.indexOf("{");
                if (startIdx === -1) throw new Error("No JSON found in analysis output");
                const jsonStr = stdoutData.substring(startIdx);
                const parsed = JSON.parse(jsonStr);
                return res.json({ ok: true, data: parsed });
            } catch (err) {
                return res.json({ error: "Parse error: " + err.message + "\nOutput: " + stdoutData });
            }
        });

        child.on("error", (err) => {
            return res.json({ error: "Failed to run analysis_tool: " + err.message });
        });
    } catch (err) {
        res.json({ error: err.message });
    }
});

app.listen(3000, () => {
    console.log("Server running on http://localhost:3000");
});
