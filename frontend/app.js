const state = {
  nodes: [
    { id: 0, weight: 1 },
    { id: 1, weight: 1 },
    { id: 2, weight: 1 },
    { id: 3, weight: 1 },
  ],
  edges: [
    { u: 0, v: 1, weight: 2, bandwidth: 10, latency: 1 },
    { u: 1, v: 2, weight: 3, bandwidth: 10, latency: 1 },
    { u: 2, v: 3, weight: 2, bandwidth: 10, latency: 1 },
    { u: 0, v: 3, weight: 9, bandwidth: 10, latency: 1 },
  ],
  result: null,
};

const $ = (id) => document.getElementById(id);

function numeric(id, fallback) {
  const value = Number($(id).value);
  return Number.isFinite(value) ? value : fallback;
}

function renderNodes() {
  const target = $("nodes");
  target.innerHTML = "";
  state.nodes.forEach((node, index) => {
    const row = document.createElement("div");
    row.className = "item-row";
    row.innerHTML = `
      <label>Node<input value="${node.id}" disabled></label>
      <label>Weight<input type="number" min="1" value="${node.weight}"></label>
      <button class="danger" type="button">Delete</button>
    `;
    row.querySelector("input[type='number']").addEventListener("input", (event) => {
      node.weight = Math.max(1, Number(event.target.value) || 1);
    });
    row.querySelector("button").addEventListener("click", () => deleteNode(index));
    target.appendChild(row);
  });
}

function renderEdges() {
  const target = $("edges");
  target.innerHTML = "";
  state.edges.forEach((edge, index) => {
    const row = document.createElement("div");
    row.className = "item-row edge-row";
    row.innerHTML = `
      <label>From<input type="number" min="0" value="${edge.u}"></label>
      <label>To<input type="number" min="0" value="${edge.v}"></label>
      <label>Weight<input type="number" min="1" value="${edge.weight}"></label>
      <button class="danger" type="button">Delete</button>
    `;
    const inputs = row.querySelectorAll("input");
    inputs[0].addEventListener("input", (event) => edge.u = Math.max(0, Number(event.target.value) || 0));
    inputs[1].addEventListener("input", (event) => edge.v = Math.max(0, Number(event.target.value) || 0));
    inputs[2].addEventListener("input", (event) => edge.weight = Math.max(1, Number(event.target.value) || 1));
    row.querySelector("button").addEventListener("click", () => {
      state.edges.splice(index, 1);
      renderAll();
    });
    target.appendChild(row);
  });
}

function addNode() {
  state.nodes.push({ id: state.nodes.length, weight: 1 });
  $("destination").value = state.nodes.length - 1;
  renderAll();
}

function deleteNode(index) {
  if (state.nodes.length <= 1) return;
  state.nodes.splice(index, 1);
  state.nodes.forEach((node, id) => node.id = id);
  state.edges = state.edges
    .filter((edge) => edge.u !== index && edge.v !== index)
    .map((edge) => ({
      ...edge,
      u: edge.u > index ? edge.u - 1 : edge.u,
      v: edge.v > index ? edge.v - 1 : edge.v,
    }));
  $("source").value = Math.min(numeric("source", 0), state.nodes.length - 1);
  $("destination").value = Math.min(numeric("destination", 0), state.nodes.length - 1);
  renderAll();
}

function addEdge() {
  const last = Math.max(0, state.nodes.length - 1);
  state.edges.push({ u: 0, v: last, weight: 1, bandwidth: numeric("bandwidth", 10), latency: numeric("latency", 1) });
  renderAll();
}

function currentPayload() {
  return {
    topology: $("topology").value,
    nodeCount: state.nodes.length,
    nodes: state.nodes,
    edges: state.edges.map((edge) => ({
      ...edge,
      bandwidth: numeric("bandwidth", 10),
      latency: numeric("latency", 1),
    })),
    settings: {
      probability: numeric("probability", 0.55),
      rows: numeric("rows", 2),
      cols: numeric("cols", 3),
      source: numeric("source", 0),
      destination: numeric("destination", state.nodes.length - 1),
      packets: numeric("packets", 12),
      packetSize: numeric("packetSize", 512),
      bandwidth: numeric("bandwidth", 10),
      latency: numeric("latency", 1),
    },
  };
}

async function runSimulation() {
  const button = $("runBtn");
  button.disabled = true;
  button.textContent = "Running...";
  $("status").textContent = "Compiling if needed, then running the simulation.";

  try {
    const response = await fetch("/api/run", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(currentPayload()),
    });
    const result = await response.json();
    if (!result.ok) throw new Error(result.error || "Simulation failed");
    state.result = result;
    $("status").textContent = `Path ${result.path.length ? result.path.join(" -> ") : "not found"} | cost ${result.pathCost}`;
    renderResult(result);
    drawGraph(result.graphEdges, result.nodes, result.path);
  } catch (error) {
    $("status").textContent = "Run failed.";
    $("debug").textContent = error.message;
  } finally {
    button.disabled = false;
    button.textContent = "Run simulation";
  }
}

function metric(label, value) {
  return `<div class="metric"><strong>${value}</strong><span>${label}</span></div>`;
}

function row(label, value) {
  return `<div class="table-row"><span>${label}</span><strong>${value}</strong></div>`;
}

function fmt(value, digits = 4) {
  return Number(value || 0).toFixed(digits);
}

function renderResult(result) {
  $("metrics").innerHTML = [
    metric("Packets", result.metrics.totalPackets),
    metric("Delivered", result.metrics.delivered),
    metric("Dropped", result.metrics.dropped),
    metric("Avg latency", fmt(result.metrics.averageLatency, 2)),
    metric("Loss rate %", fmt(result.metrics.lossRate, 2)),
    metric("Avg hops", fmt(result.metrics.averageHops, 2)),
    metric("Throughput", fmt(result.metrics.throughput, 4)),
    metric("Edges", result.edges),
  ].join("");

  $("timings").innerHTML = [
    row("Integration", `${fmt(result.timings.integrationMs)} ms`),
    row("Dijkstra", `${fmt(result.timings.dijkstraMs)} ms`),
    row("Bellman-Ford", `${fmt(result.timings.bellmanFordMs)} ms`),
    row("Routing table", `${fmt(result.timings.routingTableMs)} ms`),
    row("Kruskal MST", `${fmt(result.timings.kruskalMs)} ms`),
    row("Prim MST", `${fmt(result.timings.primMs)} ms`),
    row("Packet run", `${fmt(result.timings.packetSimulationMs)} ms`),
  ].join("");

  $("stats").innerHTML = [
    row("Topology", result.topology),
    row("Nodes", result.nodes),
    row("Average degree", fmt(result.averageDegree, 2)),
    row("Bellman-Ford valid", result.bellmanFordOk ? "yes" : "no"),
    row("Algorithm avg", `${fmt(result.statistics.algorithmAverageMs)} ms`),
    row("Algorithm variance", fmt(result.statistics.algorithmVarianceMs)),
    row("Debug file", result.files.debug),
    row("Metrics CSV", result.files.metricsCsv),
  ].join("");

  $("debug").textContent = result.debugTail || "Debug file is empty.";
}

function drawGraph(edges = state.edges, nodeCount = state.nodes.length, path = []) {
  const svg = $("graphSvg");
  const width = svg.clientWidth || 800;
  const height = svg.clientHeight || 420;
  svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
  svg.innerHTML = "";

  const cx = width / 2;
  const cy = height / 2;
  const radius = Math.max(90, Math.min(width, height) * 0.36);
  const points = Array.from({ length: nodeCount }, (_, index) => {
    const angle = (-Math.PI / 2) + (index / Math.max(1, nodeCount)) * Math.PI * 2;
    return {
      x: cx + Math.cos(angle) * radius,
      y: cy + Math.sin(angle) * radius,
    };
  });

  const pathLinks = new Set();
  for (let i = 0; i < path.length - 1; i += 1) {
    pathLinks.add([path[i], path[i + 1]].sort((a, b) => a - b).join("-"));
  }

  edges.forEach((edge) => {
    if (!points[edge.u] || !points[edge.v]) return;
    const a = points[edge.u];
    const b = points[edge.v];
    const key = [edge.u, edge.v].sort((x, y) => x - y).join("-");
    const line = svgEl("line", {
      x1: a.x, y1: a.y, x2: b.x, y2: b.y,
      class: pathLinks.has(key) ? "path-link" : "link",
    });
    svg.appendChild(line);
    const label = svgEl("text", {
      x: (a.x + b.x) / 2,
      y: (a.y + b.y) / 2,
      class: "edge-label",
    });
    label.textContent = edge.w ?? edge.weight;
    svg.appendChild(label);
  });

  points.forEach((point, index) => {
    const circle = svgEl("circle", {
      cx: point.x,
      cy: point.y,
      r: 22,
      class: path.includes(index) ? "node path-node" : "node",
    });
    svg.appendChild(circle);
    const label = svgEl("text", { x: point.x, y: point.y, class: "node-label" });
    label.textContent = index;
    svg.appendChild(label);
  });
}

function svgEl(name, attrs) {
  const el = document.createElementNS("http://www.w3.org/2000/svg", name);
  Object.entries(attrs).forEach(([key, value]) => el.setAttribute(key, value));
  return el;
}

function renderAll() {
  renderNodes();
  renderEdges();
  drawGraph();
}

$("addNode").addEventListener("click", addNode);
$("addEdge").addEventListener("click", addEdge);
$("runBtn").addEventListener("click", runSimulation);
window.addEventListener("resize", () => drawGraph(state.result?.graphEdges || state.edges, state.result?.nodes || state.nodes.length, state.result?.path || []));

renderAll();
