const cy = cytoscape({
  container: document.getElementById('cy'),
  elements: [
    { data: { id: 'A' }, position: { x: 160, y: 180 } },
    { data: { id: 'B' }, position: { x: 380, y: 180 } },
    { data: { id: 'C' }, position: { x: 270, y: 340 } },
    { data: { id: 'A-B', source: 'A', target: 'B', weight: 10, delay: 57 } },
    { data: { id: 'B-C', source: 'B', target: 'C', weight: 5, delay: 20 } }
  ],
  style: [
    {
      selector: 'node',
      style: {
        'background-color': '#2ecc71',
        'label': 'data(id)',
        'text-valign': 'center',
        'text-halign': 'center',
        'color': '#fff',
        'font-weight': 700,
        'width': 48,
        'height': 48
      }
    },
    {
      selector: 'edge',
      style: {
        'curve-style': 'bezier',
        'target-arrow-shape': 'triangle',
        'label': 'data(weight)',
        'line-color': '#aaa',
        'target-arrow-color': '#aaa',
        'width': 3,
        'font-size': 12,
        'text-background-color': '#fff',
        'text-background-opacity': 1,
        'text-background-padding': 3
      }
    },
    {
      selector: ':selected',
      style: {
        'background-color': '#d35400',
        'line-color': '#d35400',
        'target-arrow-color': '#d35400'
      }
    },
    {
      selector: '.path-highlight',
      style: {
        'background-color': '#c0392b',
        'line-color': '#c0392b',
        'target-arrow-color': '#c0392b',
        'width': 6
      }
    }
  ],
  layout: { name: 'preset' },
  userPanningEnabled: true,
  userZoomingEnabled: true,
  autoungrabify: false
});

const dom = {
  uploadTab: document.getElementById('uploadTab'),
  manualTab: document.getElementById('manualTab'),
  uploadPanel: document.getElementById('uploadPanel'),
  manualPanel: document.getElementById('manualPanel'),
  fileUpload: document.getElementById('fileUpload'),
  fileName: document.getElementById('fileName'),
  nodeId: document.getElementById('nodeId'),
  edgeSource: document.getElementById('edgeSource'),
  edgeTarget: document.getElementById('edgeTarget'),
  edgeWeight: document.getElementById('edgeWeight'),
  edgeDelay: document.getElementById('edgeDelay'),
  addNode: document.getElementById('addNode'),
  addEdge: document.getElementById('addEdge'),
  deleteSelected: document.getElementById('deleteSelected'),
  clearGraph: document.getElementById('clearGraph'),
  runSimulation: document.getElementById('runSimulation'),
  jsonPreview: document.getElementById('jsonPreview'),
  graphSummary: document.getElementById('graphSummary'),
  latency: document.getElementById('latency'),
  packetLoss: document.getElementById('packetLoss'),
  throughput: document.getElementById('throughput'),
  paths: document.getElementById('paths'),
  logs: document.getElementById('logs'),
  status: document.getElementById('status')
};

let statusTimer;

function showStatus(message) {
  clearTimeout(statusTimer);
  dom.status.textContent = message;
  dom.status.classList.add('visible');
  statusTimer = setTimeout(() => dom.status.classList.remove('visible'), 4200);
}

function setTab(tab) {
  const uploadActive = tab === 'upload';
  dom.uploadTab.classList.toggle('active', uploadActive);
  dom.manualTab.classList.toggle('active', !uploadActive);
  dom.uploadPanel.classList.toggle('active', uploadActive);
  dom.manualPanel.classList.toggle('active', !uploadActive);
}

function cleanId(value) {
  return String(value || '').trim();
}

function cleanNumber(value, fallback = 1) {
  const number = Number(value);
  return Number.isFinite(number) && number >= 0 ? number : fallback;
}

function edgeId(source, target) {
  return `${source}-${target}`;
}

function currentTopology() {
  return {
    nodes: cy.nodes().map(node => ({ id: node.id() })),
    edges: cy.edges().map(edge => ({
      source: edge.data('source'),
      target: edge.data('target'),
      weight: cleanNumber(edge.data('weight')),
      delay: cleanNumber(edge.data('delay'))
    }))
  };
}

function updatePreview() {
  const topology = currentTopology();
  dom.jsonPreview.value = JSON.stringify(topology, null, 2);
  dom.graphSummary.textContent = `${topology.nodes.length} nodes, ${topology.edges.length} edges`;
}

function loadTopology(topology) {
  if (!topology || !Array.isArray(topology.nodes) || !Array.isArray(topology.edges)) {
    throw new Error('Topology must contain nodes and edges arrays.');
  }

  const elements = [];
  const nodeIds = new Set();

  topology.nodes.forEach((node, index) => {
    const id = cleanId(node.id ?? node);
    if (!id || nodeIds.has(id)) return;
    nodeIds.add(id);
    elements.push({
      data: { id },
      position: {
        x: 180 + (index % 4) * 150,
        y: 150 + Math.floor(index / 4) * 130
      }
    });
  });

  topology.edges.forEach(edge => {
    const source = cleanId(edge.source);
    const target = cleanId(edge.target);
    if (!source || !target) return;

    if (!nodeIds.has(source)) {
      nodeIds.add(source);
      elements.push({ data: { id: source } });
    }

    if (!nodeIds.has(target)) {
      nodeIds.add(target);
      elements.push({ data: { id: target } });
    }

    elements.push({
      data: {
        id: edgeId(source, target),
        source,
        target,
        weight: cleanNumber(edge.weight),
        delay: cleanNumber(edge.delay)
      }
    });
  });

  cy.elements().remove();
  cy.add(elements);
  cy.layout({ name: 'cose', animate: false, padding: 45 }).run();
  cy.elements().removeClass('path-highlight');
  updatePreview();
}

function metricFrom(result, keys) {
  const containers = [result, result.metrics, result.summary, result.networkMetrics].filter(Boolean);

  for (const container of containers) {
    for (const key of keys) {
      if (container[key] !== undefined && container[key] !== null) return container[key];
    }
  }

  return '-';
}

function resultPaths(result) {
  const paths = result.packetPaths || result.packet_paths || result.paths || result.routes || [];
  return Array.isArray(paths) ? paths : [];
}

function resultLogs(result) {
  const logs = result.logs || result.log || result.events || [];
  return Array.isArray(logs) ? logs.join('\n') : String(logs || '');
}

function pathNodeList(path) {
  if (Array.isArray(path)) return path.map(String);
  if (Array.isArray(path.nodes)) return path.nodes.map(String);
  if (Array.isArray(path.path)) return path.path.map(String);
  if (Array.isArray(path.route)) return path.route.map(String);
  return [];
}

function pathLabel(path) {
  const nodes = pathNodeList(path);
  if (nodes.length) return nodes.join(' -> ');
  if (path && typeof path === 'object') return JSON.stringify(path);
  return String(path);
}

function highlightPath(path) {
  const nodes = pathNodeList(path);
  cy.elements().removeClass('path-highlight');

  nodes.forEach(id => cy.getElementById(id).addClass('path-highlight'));

  for (let i = 0; i < nodes.length - 1; i += 1) {
    cy.edges().filter(edge => {
      return edge.data('source') === nodes[i] && edge.data('target') === nodes[i + 1];
    }).addClass('path-highlight');
  }
}

function renderResults(result) {
  dom.latency.textContent = metricFrom(result, ['latency', 'averageLatency', 'avg_latency']);
  dom.packetLoss.textContent = metricFrom(result, ['packetLoss', 'packet_loss', 'loss']);
  dom.throughput.textContent = metricFrom(result, ['throughput', 'bandwidth']);
  dom.logs.textContent = resultLogs(result);
  dom.paths.innerHTML = '';

  resultPaths(result).forEach(path => {
    const item = document.createElement('li');
    item.textContent = pathLabel(path);
    item.addEventListener('click', () => highlightPath(path));
    dom.paths.appendChild(item);
  });
}

function addNode() {
  const id = cleanId(dom.nodeId.value);

  if (!id) {
    showStatus('Enter a node id.');
    return;
  }

  if (cy.getElementById(id).length) {
    showStatus('That node already exists.');
    return;
  }

  cy.add({
    group: 'nodes',
    data: { id },
    position: { x: 160 + Math.random() * 360, y: 160 + Math.random() * 260 }
  });
  dom.nodeId.value = '';
  updatePreview();
}

function addEdge() {
  const source = cleanId(dom.edgeSource.value);
  const target = cleanId(dom.edgeTarget.value);
  const weight = cleanNumber(dom.edgeWeight.value);
  const delay = cleanNumber(dom.edgeDelay.value);

  if (!cy.getElementById(source).length || !cy.getElementById(target).length) {
    showStatus('Source and target nodes must exist.');
    return;
  }

  cy.getElementById(edgeId(source, target)).remove();
  cy.add({
    group: 'edges',
    data: { id: edgeId(source, target), source, target, weight, delay }
  });
  updatePreview();
}

function deleteSelected() {
  cy.$(':selected').remove();
  updatePreview();
}

function clearGraph() {
  cy.elements().remove();
  renderResults({});
  updatePreview();
}

function fillEdgeForm(edge) {
  dom.edgeSource.value = edge.data('source');
  dom.edgeTarget.value = edge.data('target');
  dom.edgeWeight.value = edge.data('weight');
  dom.edgeDelay.value = edge.data('delay');
}

async function uploadTopology(file) {
  const formData = new FormData();
  formData.append('topology', file);

  const response = await fetch('/upload', {
    method: 'POST',
    body: formData
  });
  const data = await response.json();

  if (!response.ok) throw new Error(data.error || 'Upload failed.');
  return data;
}

async function runSimulation() {
  dom.runSimulation.disabled = true;
  dom.runSimulation.textContent = 'Running...';

  try {
    const response = await fetch('/simulate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(currentTopology())
    });

    const text = await response.text();
    let data = {};

    try {
      data = text ? JSON.parse(text) : {};
    } catch {
      data = {};
    }

    if (!response.ok) throw new Error(data.error || text || 'Simulation failed.');
    renderResults(data);
    showStatus('Simulation complete.');
  } catch (error) {
    showStatus(error.message);
  } finally {
    dom.runSimulation.disabled = false;
    dom.runSimulation.textContent = 'Run Simulation';
  }
}

dom.uploadTab.addEventListener('click', () => setTab('upload'));
dom.manualTab.addEventListener('click', () => setTab('manual'));
dom.addNode.addEventListener('click', addNode);
dom.addEdge.addEventListener('click', addEdge);
dom.deleteSelected.addEventListener('click', deleteSelected);
dom.clearGraph.addEventListener('click', clearGraph);
dom.runSimulation.addEventListener('click', runSimulation);

dom.nodeId.addEventListener('keydown', event => {
  if (event.key === 'Enter') addNode();
});

document.addEventListener('keydown', event => {
  if (event.key === 'Delete' || event.key === 'Backspace') {
    const editing = ['INPUT', 'TEXTAREA'].includes(document.activeElement.tagName);
    if (!editing) deleteSelected();
  }
});

dom.fileUpload.addEventListener('change', async event => {
  const [file] = event.target.files;
  if (!file) return;

  const allowed = ['.json', '.txt', '.csv'];
  const ext = file.name.slice(file.name.lastIndexOf('.')).toLowerCase();

  if (!allowed.includes(ext)) {
    showStatus('Upload JSON, TXT, or CSV only.');
    event.target.value = '';
    return;
  }

  try {
    const data = await uploadTopology(file);
    loadTopology(data.topology);
    dom.fileName.textContent = `${file.name} loaded as ${data.format.toUpperCase()}`;
    showStatus('Topology loaded.');
  } catch (error) {
    showStatus(error.message);
  } finally {
    event.target.value = '';
  }
});

cy.on('tap', 'node', event => {
  dom.nodeId.value = event.target.id();
});

cy.on('tap', 'edge', event => {
  fillEdgeForm(event.target);
});

cy.on('add remove dragfree data', updatePreview);

updatePreview();
