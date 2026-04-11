const express = require('express');
const multer = require('multer');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { execFile } = require('child_process');

const app = express();
const PORT = Number(process.env.PORT || 3000);
const simulatorPath = path.join(__dirname, '../engine/simulator');
const MAX_INPUT_BYTES = 2 * 1024 * 1024;
const ALLOWED_EXTENSIONS = new Set(['.json', '.txt', '.csv']);

let latestResults = null;

const upload = multer({
  storage: multer.memoryStorage(),
  limits: { fileSize: MAX_INPUT_BYTES },
  fileFilter: (req, file, cb) => {
    cb(null, ALLOWED_EXTENSIONS.has(path.extname(file.originalname).toLowerCase()));
  }
});

app.use(express.json({ limit: '2mb' }));
app.use(express.text({ limit: '2mb', type: ['text/plain', 'text/csv'] }));
app.use(express.static(__dirname));

function isObject(value) {
  return value && typeof value === 'object' && !Array.isArray(value);
}

function cleanId(value) {
  if (typeof value === 'number' && Number.isFinite(value)) return String(value);
  if (typeof value !== 'string') return '';
  return value.trim();
}

function cleanNumber(value, fallback = 1) {
  const number = Number(value);
  return Number.isFinite(number) && number >= 0 ? number : fallback;
}

function parseJson(content) {
  return typeof content === 'string' ? JSON.parse(content) : content;
}

function parseTxt(content) {
  const edges = [];
  const nodeIds = new Set();

  String(content)
    .split(/\r?\n/)
    .map(line => line.trim())
    .filter(line => line && !line.startsWith('#'))
    .forEach(line => {
      const [source, target, weight = 1, delay = 1] = line.split(/\s+/);
      if (!source || !target) throw new Error('TXT rows must be: source target weight delay');
      nodeIds.add(source);
      nodeIds.add(target);
      edges.push({ source, target, weight: cleanNumber(weight), delay: cleanNumber(delay) });
    });

  return {
    nodes: [...nodeIds].map(id => ({ id })),
    edges
  };
}

function parseCsvLine(line) {
  const values = [];
  let current = '';
  let quoted = false;

  for (let i = 0; i < line.length; i += 1) {
    const char = line[i];
    const next = line[i + 1];

    if (char === '"' && quoted && next === '"') {
      current += '"';
      i += 1;
    } else if (char === '"') {
      quoted = !quoted;
    } else if (char === ',' && !quoted) {
      values.push(current.trim());
      current = '';
    } else {
      current += char;
    }
  }

  values.push(current.trim());
  return values;
}

function parseCsv(content) {
  const rows = String(content)
    .split(/\r?\n/)
    .map(line => line.trim())
    .filter(Boolean)
    .map(parseCsvLine);

  if (!rows.length) return { nodes: [], edges: [] };

  const first = rows[0].map(value => value.toLowerCase());
  const hasHeader = first.includes('source') && first.includes('target');
  const header = hasHeader ? first : ['source', 'target', 'weight', 'delay'];
  const dataRows = hasHeader ? rows.slice(1) : rows;
  const sourceIndex = header.indexOf('source');
  const targetIndex = header.indexOf('target');
  const weightIndex = header.indexOf('weight');
  const delayIndex = header.indexOf('delay');
  const edges = [];
  const nodeIds = new Set();

  dataRows.forEach(row => {
    const source = row[sourceIndex];
    const target = row[targetIndex];
    if (!source || !target) throw new Error('CSV rows must include source and target.');
    nodeIds.add(source);
    nodeIds.add(target);
    edges.push({
      source,
      target,
      weight: cleanNumber(row[weightIndex]),
      delay: cleanNumber(row[delayIndex])
    });
  });

  return {
    nodes: [...nodeIds].map(id => ({ id })),
    edges
  };
}

function detectFormat(content, filename = '') {
  const ext = path.extname(filename).toLowerCase();
  const text = typeof content === 'string' ? content.trim() : '';

  if (ext === '.json') return 'json';
  if (ext === '.txt') return 'txt';
  if (ext === '.csv') return 'csv';
  if (isObject(content)) return 'json';
  if (text.startsWith('{') || text.startsWith('[')) return 'json';
  if (text.split(/\r?\n/)[0]?.includes(',')) return 'csv';
  return 'txt';
}

function parseTopology(content, filename = '') {
  const format = detectFormat(content, filename);

  if (format === 'json') return { format, topology: parseJson(content) };
  if (format === 'csv') return { format, topology: parseCsv(content) };
  return { format, topology: parseTxt(content) };
}

function validateTopology(topology) {
  if (!isObject(topology)) {
    return { ok: false, error: 'Topology must be an object.' };
  }

  const nodes = Array.isArray(topology.nodes) ? topology.nodes : [];
  const edges = Array.isArray(topology.edges) ? topology.edges : [];

  if (nodes.length > 500 || edges.length > 2000) {
    return { ok: false, error: 'Topology exceeds maximum size.' };
  }

  const nodeIds = new Set();
  const cleanNodes = [];

  for (const node of nodes) {
    const id = cleanId(isObject(node) ? node.id : node);
    if (!id || id.length > 80 || nodeIds.has(id)) {
      return { ok: false, error: 'Each node must have a unique id up to 80 characters.' };
    }
    nodeIds.add(id);
    cleanNodes.push({ id });
  }

  for (const edge of edges) {
    if (!isObject(edge)) {
      return { ok: false, error: 'Each edge must be an object.' };
    }

    const source = cleanId(edge.source);
    const target = cleanId(edge.target);
    if (!source || !target) {
      return { ok: false, error: 'Each edge must include source and target.' };
    }
    if (!nodeIds.has(source)) {
      nodeIds.add(source);
      cleanNodes.push({ id: source });
    }
    if (!nodeIds.has(target)) {
      nodeIds.add(target);
      cleanNodes.push({ id: target });
    }
  }

  const cleanEdges = edges.map(edge => ({
    source: cleanId(edge.source),
    target: cleanId(edge.target),
    weight: cleanNumber(edge.weight),
    delay: cleanNumber(edge.delay)
  }));

  return {
    ok: true,
    topology: {
      nodes: cleanNodes,
      edges: cleanEdges
    }
  };
}

function runSimulator(topology, res) {
  if (!fs.existsSync(simulatorPath)) {
    return res.status(500).send('Simulator not found at ' + simulatorPath);
  }

  try {
    fs.accessSync(simulatorPath, fs.constants.X_OK);
  } catch {
    console.warn('Simulator is not executable. Run: chmod +x engine/simulator');
  }

  const tempDir = fs.mkdtempSync(path.join(os.tmpdir(), 'network-sim-'));
  const inputPath = path.join(tempDir, 'input.txt');
  const outputPath = path.join(tempDir, 'output.json');

  fs.writeFileSync(inputPath, JSON.stringify(topology, null, 2));

  console.log('Running simulator at:', simulatorPath);

  execFile(simulatorPath, [inputPath, outputPath], (err, stdout, stderr) => {
    try {
      if (err) {
        console.error(stderr);
        res.status(500).send('Simulation failed');
        return;
      }

      const result = JSON.parse(fs.readFileSync(outputPath));
      latestResults = result;
      res.json(result);
    } catch (error) {
      res.status(500).send('Invalid output from simulator');
    } finally {
      fs.rmSync(tempDir, { recursive: true, force: true });
    }
  });
}

app.post('/simulate', (req, res) => {
  try {
    const payload = typeof req.body === 'string' ? req.body : req.body?.topology || req.body;
    const filename = typeof req.body === 'object' ? req.body?.filename : '';
    const { topology } = parseTopology(payload, filename);
    const validation = validateTopology(topology);

    if (!validation.ok) {
      res.status(400).json({ error: validation.error });
      return;
    }

    runSimulator(validation.topology, res);
  } catch (error) {
    res.status(400).json({ error: error.message });
  }
});

app.post('/upload', upload.single('topology'), (req, res) => {
  if (!req.file) {
    res.status(400).json({ error: 'Upload a JSON, TXT, or CSV topology file.' });
    return;
  }

  try {
    const content = req.file.buffer.toString('utf8');
    const { format, topology } = parseTopology(content, req.file.originalname);
    const validation = validateTopology(topology);

    if (!validation.ok) {
      res.status(400).json({ error: validation.error });
      return;
    }

    res.json({ format, topology: validation.topology });
  } catch (error) {
    res.status(400).json({ error: error.message });
  }
});

app.get('/results', (req, res) => {
  if (!latestResults) {
    res.status(404).json({ error: 'No simulation results available.' });
    return;
  }

  res.json(latestResults);
});

app.use((error, req, res, next) => {
  if (error instanceof multer.MulterError) {
    res.status(400).json({ error: error.message });
    return;
  }
  next(error);
});

app.listen(PORT, () => {
  console.log(`Network simulation web UI running at http://localhost:${PORT}`);
});
