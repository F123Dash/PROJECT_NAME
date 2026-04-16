import React, { useState, useEffect, useRef, useCallback, useMemo } from 'react';
import {
  ReactFlow, Controls, Background,
  useNodesState, useEdgesState, addEdge,
  useReactFlow, ReactFlowProvider,
  MarkerType, Handle, Position, useViewport
} from '@xyflow/react';
import axios from 'axios';
import {
  Play, Pause, Square, SkipForward, SkipBack,
  Activity, Settings, ListTree, BarChart3,
  AlertCircle, Terminal, Loader2, Info,
  Plus, Zap, Upload, Route, Target, MapPin
} from 'lucide-react';
import * as d3 from 'd3-force';


// ═══════════════════════════════════════════════════════════════
// JS implementations of shortest path algorithms (for comparison)
// ═══════════════════════════════════════════════════════════════
function buildAdjList(rfEdges) {
  const adj = {};
  for (const e of rfEdges) {
    const u = parseInt(e.source ?? e.from), v = parseInt(e.target ?? e.to), w = e.data?.weight ?? e.weight ?? 1;
    if (!adj[u]) adj[u] = [];
    if (!adj[v]) adj[v] = [];
    adj[u].push({ to: v, w });
    adj[v].push({ to: u, w });
  }
  return adj;
}

function dijkstraJS(adj, nodeCount, src) {
  const dist = Array(nodeCount).fill(Infinity);
  const parent = Array(nodeCount).fill(-1);
  dist[src] = 0;
  const visited = new Set();
  // Min-heap using array
  const pq = [[0, src]];
  while (pq.length > 0) {
    pq.sort((a, b) => a[0] - b[0]);
    const [d, u] = pq.shift();
    if (visited.has(u)) continue;
    visited.add(u);
    if (!adj[u]) continue;
    for (const { to: v, w } of adj[u]) {
      if (dist[u] + w < dist[v]) {
        dist[v] = dist[u] + w;
        parent[v] = u;
        pq.push([dist[v], v]);
      }
    }
  }
  return { dist, parent };
}

function bellmanFordJS(adj, nodeCount, src) {
  const dist = Array(nodeCount).fill(Infinity);
  const parent = Array(nodeCount).fill(-1);
  dist[src] = 0;
  const edges = [];
  for (let u = 0; u < nodeCount; u++) {
    if (!adj[u]) continue;
    for (const { to: v, w } of adj[u]) edges.push({ u, v, w });
  }
  for (let i = 0; i < nodeCount - 1; i++) {
    for (const { u, v, w } of edges) {
      if (dist[u] !== Infinity && dist[u] + w < dist[v]) {
        dist[v] = dist[u] + w;
        parent[v] = u;
      }
    }
  }
  let hasNegCycle = false;
  for (const { u, v, w } of edges) {
    if (dist[u] !== Infinity && dist[u] + w < dist[v]) { hasNegCycle = true; break; }
  }
  return { dist, parent, hasNegCycle };
}

function bfsJS(adj, nodeCount, src) {
  const dist = Array(nodeCount).fill(Infinity);
  const parent = Array(nodeCount).fill(-1);
  dist[src] = 0;
  const queue = [src];
  let head = 0;
  while (head < queue.length) {
    const u = queue[head++];
    if (!adj[u]) continue;
    for (const { to: v } of adj[u]) {
      if (dist[v] === Infinity) {
        dist[v] = dist[u] + 1;
        parent[v] = u;
        queue.push(v);
      }
    }
  }
  return { dist, parent };
}

function reconstructPath(parent, dst) {
  const path = [];
  for (let v = dst; v !== -1; v = parent[v]) {
    path.unshift(v);
    if (path.length > 1000) break;
  }
  return path[0] === undefined ? [] : path;
}


// ═══════════════════════════════════════════════════════════════
// Custom Node
// ═══════════════════════════════════════════════════════════════
const CustomNode = ({ id, data, selected }) => {
  const isSrc = data.isSource;
  const isDst = data.isDestination;
  const border = selected ? '#fff' :
    isSrc ? '#10b981' :
    isDst ? '#f59e0b' :
    data.active ? 'var(--accent-color)' : 'var(--panel-border)';
  const shadow = isSrc ? '0 0 14px rgba(16,185,129,0.6)' :
    isDst ? '0 0 14px rgba(245,158,11,0.6)' :
    data.active ? '0 0 14px var(--accent-glow)' : 'none';

  return (
    <div style={{
      background: 'var(--panel-bg)', border: `2px solid ${border}`,
      borderRadius: '50%', width: 44, height: 44,
      display: 'flex', alignItems: 'center', justifyContent: 'center',
      boxShadow: shadow, position: 'relative', transition: 'all .2s'
    }}>
      <Handle type="target" position={Position.Top} style={{ opacity: 0 }} />
      <Handle type="source" position={Position.Bottom} style={{ opacity: 0 }} />
      <Handle type="target" position={Position.Left} style={{ opacity: 0 }} />
      <Handle type="source" position={Position.Right} style={{ opacity: 0 }} />
      <div style={{ fontSize: 10, color: isSrc ? '#10b981' : isDst ? '#f59e0b' : 'var(--text-secondary)', position: 'absolute', top: -18, fontWeight: isSrc || isDst ? 700 : 400 }}>
        {isSrc ? 'SRC' : isDst ? 'DST' : `N${id}`}
      </div>
      {(data.queueCount || 0) > 0 && (
        <div style={{
          position: 'absolute', top: -8, right: -8,
          background: 'var(--accent-secondary)', color: '#fff', borderRadius: 10,
          fontSize: 9, padding: '1px 5px', fontWeight: 700,
          border: '1.5px solid var(--panel-bg)', zIndex: 10
        }}>{data.queueCount}</div>
      )}
      <div style={{ fontWeight: 700, pointerEvents: 'none', fontSize: 13,
        color: isSrc ? '#10b981' : isDst ? '#f59e0b' : '#fff'
      }}>{id}</div>
    </div>
  );
};

const nodeTypes = { custom: CustomNode };
const defaultEdgeOptions = {
  type: 'default',
  style: { stroke: 'var(--text-secondary)', strokeWidth: 2 },
  markerEnd: { type: MarkerType.ArrowClosed, color: 'var(--text-secondary)' },
};

// Packet SVG overlay
const PacketOverlay = ({ packets }) => {
  const { x, y, zoom } = useViewport();
  return (
    <svg style={{ position: 'absolute', inset: 0, width: '100%', height: '100%', pointerEvents: 'none', zIndex: 100 }}>
      <g transform={`translate(${x},${y}) scale(${zoom})`}>
        {Object.entries(packets).map(([k, p]) => (
          <g key={`pkt-${k}`}>
            <circle cx={p.x} cy={p.y} r={5} fill="var(--accent-color)" opacity={0.85}>
              <animate attributeName="r" values="4;7;4" dur="0.5s" repeatCount="indefinite" />
            </circle>
            <circle cx={p.x} cy={p.y} r={2.5} fill="#fff" />
          </g>
        ))}
      </g>
    </svg>
  );
};


// ═══════════════════════════════════════════════════════════════
// Main Application
// ═══════════════════════════════════════════════════════════════
const NetworkSimulatorApp = () => {
  const wrapper = useRef(null);
  const { fitView } = useReactFlow();
  const fileInputRef = useRef(null);

  const [nodes, setNodes, onNodesChange] = useNodesState([]);
  const [edges, setEdges, onEdgesChange] = useEdgesState([]);

  const [cfg, setCfg] = useState({
    nodes: '10', topology: 'tree', probability: '0.4',
    simulation_time: '30', bandwidth: '100', delay: '2',
    traffic_type: 'constant', rate: '5', packet_size: '512',
    grid_rows: '3', grid_cols: '3'
  });

  // Source / Destination
  const [sourceNode, setSourceNode] = useState('0');
  const [destNode, setDestNode] = useState('');

  // Shortest path
  const [spAlgorithm, setSpAlgorithm] = useState('dijkstra');
  const [spResult, setSpResult] = useState(null);
  const [spCompare, setSpCompare] = useState(null);

  // Simulation telemetry
  const [events, setEvents] = useState([]);
  const [animSegments, setAnimSegments] = useState([]);
  const [metrics, setMetrics] = useState(null);
  const [fullLog, setFullLog] = useState('');

  // Animation
  const [simTime, setSimTime] = useState(0);
  const [isPaused, setIsPaused] = useState(true);
  const [packetDuration, setPacketDuration] = useState(12);
  const [msPerFrame, setMsPerFrame] = useState(0.05);
  const [activePackets, setActivePackets] = useState({});

  // UI
  const [tab, setTab] = useState('config');
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState('');
  const [status, setStatus] = useState('');
  const [selectedNode, setSelectedNode] = useState(null);
  const [connectionSource, setConnectionSource] = useState(null);
  const [showFullLog, setShowFullLog] = useState(false);

  // Refs
  const isPausedRef = useRef(true); isPausedRef.current = isPaused;
  const simTimeRef = useRef(0); simTimeRef.current = simTime;
  const eventsRef = useRef([]); eventsRef.current = events;
  const animSegmentsRef = useRef([]); animSegmentsRef.current = animSegments;


  // ─── D3 Layout ───
  const layoutAndSet = useCallback((rawNodes, rawEdges) => {
    const w = wrapper.current?.offsetWidth || 800;
    const h = wrapper.current?.offsetHeight || 600;
    const simN = rawNodes.map(n => ({
      id: n.id,
      x: w / 2 + (Math.random() - 0.5) * Math.min(w, 300),
      y: h / 2 + (Math.random() - 0.5) * Math.min(h, 250)
    }));
    const simE = rawEdges.map(e => ({ source: e.source, target: e.target }));
    const n = rawNodes.length;
    const sim = d3.forceSimulation(simN)
      .force('link', d3.forceLink(simE).id(d => d.id).distance(n > 200 ? 30 : 70).strength(1.5))
      .force('charge', d3.forceManyBody().strength(n > 200 ? -50 : -300))
      .force('collide', d3.forceCollide(n > 200 ? 12 : 35))
      .force('center', d3.forceCenter(w / 2, h / 2))
      .force('x', d3.forceX(w / 2).strength(0.08))
      .force('y', d3.forceY(h / 2).strength(0.08))
      .stop();
    for (let i = 0; i < (n > 500 ? 80 : 300); i++) sim.tick();
    setNodes(rawNodes.map((nd, i) => ({ ...nd, position: { x: simN[i].x, y: simN[i].y } })));
    setEdges(rawEdges);
    setTimeout(() => fitView({ padding: 0.1, duration: 500 }), 150);
  }, [setNodes, setEdges, fitView]);


  // ─── Load config on mount ───
  useEffect(() => {
    axios.get('http://localhost:3000/config').then(r => {
      if (r.data.config) setCfg(prev => ({ ...prev, ...r.data.config }));
    }).catch(() => {});
  }, []);


  // ─── Upload config file ───
  const handleConfigUpload = (e) => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (evt) => {
      const parsed = {};
      evt.target.result.split('\n').forEach(line => {
        const l = line.trim();
        if (!l || l.startsWith('#')) return;
        const eq = l.indexOf('=');
        if (eq > 0) parsed[l.substring(0, eq).trim()] = l.substring(eq + 1).trim();
      });
      setCfg(prev => ({ ...prev, ...parsed }));
      setStatus('Config loaded: ' + file.name);
      setTimeout(() => setStatus(''), 3000);
    };
    reader.readAsText(file);
    e.target.value = '';
  };


  // ─── Update source/dest markers on nodes ───
  useEffect(() => {
    setNodes(nds => nds.map(n => ({
      ...n,
      data: {
        ...n.data,
        isSource: n.id === sourceNode,
        isDestination: n.id === destNode
      }
    })));
  }, [sourceNode, destNode, setNodes]);


  // ─── Run simulation ───
  const runSimulation = async (overrides = {}, customTopo = null) => {
    setIsPaused(true);
    setIsLoading(true);
    setError('');
    setEvents([]);
    setMetrics(null);
    setFullLog('');
    setSimTime(0);
    simTimeRef.current = 0;
    setActivePackets({});
    setSpResult(null);
    setSpCompare(null);
    setStatus('Running C++ simulator...');

    try {
      const payload = { configOverrides: { ...cfg, ...overrides } };
      if (customTopo) payload.topology = customTopo;

      const res = await axios.post('http://localhost:3000/run', payload, { timeout: 300000 });

      if (res.data.error) { setError(res.data.error); setStatus(''); return; }

      if (res.data.consoleLog) setFullLog(res.data.consoleLog);

      if (res.data.packets_sent !== undefined) {
        setMetrics({
          packets_sent: res.data.packets_sent,
          packets_delivered: res.data.packets_delivered,
          packets_dropped: res.data.packets_dropped,
          latency: res.data.latency,
          min_latency: res.data.min_latency,
          max_latency: res.data.max_latency,
          jitter: res.data.jitter,
          packet_loss: res.data.packet_loss,
          average_hops: res.data.average_hops,
          throughput: res.data.throughput,
        });
      }

      const evts = res.data.events || [];
      const nodeCount = res.data.nodeCount || parseInt(cfg.nodes) || parseInt(overrides.nodes) || 10;

      // Build graph from C++ exported topology edges
      const rfNodes = [];
      for (let i = 0; i < nodeCount; i++) {
        rfNodes.push({
          id: `${i}`, position: { x: 0, y: 0 },
          data: {
            label: `Node ${i}`, weight: 1, active: false, queueCount: 0,
            isSource: `${i}` === sourceNode,
            isDestination: `${i}` === destNode
          },
          type: 'custom'
        });
      }

      let rfEdges = [];
      if (res.data.topology_edges && res.data.topology_edges.length > 0) {
        rfEdges = res.data.topology_edges.map(e => ({
          id: `e${e.from}-${e.to}`, source: `${e.from}`, target: `${e.to}`,
          label: `${e.weight || 1}`,
          data: { weight: e.weight || 1, bandwidth: 10, latency: 1 }
        }));
      }

      if (rfEdges.length === 0 && evts.length > 0) {
        // Fallback: extract from events
        const edgeSet = new Set();
        for (const ev of evts) {
          if ((ev.type === 'FORWARDED' || ev.type === 'ROUTING') && ev.from !== ev.to && ev.from >= 0 && ev.to >= 0) {
            edgeSet.add(Math.min(ev.from, ev.to) + '-' + Math.max(ev.from, ev.to));
          }
        }
        rfEdges = [...edgeSet].map(k => {
          const [a, b] = k.split('-').map(Number);
          return { id: `e${a}-${b}`, source: `${a}`, target: `${b}`, label: '1', data: { weight: 1 } };
        });
      }

      // Pre-process events: extract actual hop data from ROUTING logLines for animation
      // ROUTING logLines look like: "[LOG] T=0 Route decision: pkt 0 at node 0 to dst 9 via 2"
      // We parse "at node X" and "via Y" to get actual from→to for packet animation
      const processedEvts = evts.map(ev => {
        if (ev.type === 'ROUTING' && ev.logLine) {
          const m = ev.logLine.match(/at node (\d+).*via (\d+)/);
          if (m) {
            return { ...ev, _animFrom: parseInt(m[1]), _animTo: parseInt(m[2]) };
          }
        }
        return ev;
      });

      // Deduplicate: keep only unique (packetId, time, _animFrom, _animTo) routing events
      const seen = new Set();
      const dedupedEvts = processedEvts.filter(ev => {
        if (ev._animFrom !== undefined) {
          const key = `${ev.packetId}-${ev.time}-${ev._animFrom}-${ev._animTo}`;
          if (seen.has(key)) return false;
          seen.add(key);
        }
        return true;
      });

      // NEW LOGIC: Pre-calculate exact sequential animation segments per packet
      const packetHops = {};
      dedupedEvts.forEach(ev => {
        if (ev._animFrom !== undefined && ev._animTo !== undefined) {
          if (!packetHops[ev.packetId]) packetHops[ev.packetId] = [];
          packetHops[ev.packetId].push({
             time: ev.time,
             from: ev._animFrom,
             to: ev._animTo
          });
        }
      });

      const segments = [];
      Object.keys(packetHops).forEach(pktId => {
        const hops = packetHops[pktId].sort((a,b) => a.time - b.time);
        for(let i=0; i<hops.length; i++) {
           let duration = 2; // default if last hop
           if (i < hops.length - 1) {
              duration = hops[i+1].time - hops[i].time;
           } else {
              const delivered = dedupedEvts.find(e => e.packetId == pktId && e.type === 'DELIVERED');
              if (delivered && delivered.time > hops[i].time) {
                 duration = delivered.time - hops[i].time;
              }
           }
           if (duration > 0) {
             segments.push({
               packetId: pktId,
               time: hops[i].time,
               from: hops[i].from,
               to: hops[i].to,
               duration: duration
             });
           }
        }
      });

      setAnimSegments(segments);
      setEvents(dedupedEvts);
      layoutAndSet(rfNodes, rfEdges);

      // Auto-set destination from events
      if (evts.length > 0) {
        const firstCreated = evts.find(e => e.type === 'CREATED');
        if (firstCreated && firstCreated.logLine) {
          const m = firstCreated.logLine.match(/(\d+)\s*->\s*(\d+)/);
          if (m) {
            setSourceNode(m[1]);
            setDestNode(m[2]);
          }
        }
      }

      setTab('queue');
      setStatus('');
    } catch (err) {
      setError('Server error: ' + err.message);
      setStatus('');
    } finally {
      setIsLoading(false);
    }
  };


  // ─── Stress test ───
  const runStressTest = () => {
    const n = parseInt(prompt('Node count for stress test:', '1000'));
    if (!n || n < 2) return;
    runSimulation({
      nodes: `${n}`, topology: 'random',
      probability: `${Math.min(0.1, 5 / n).toFixed(4)}`,
      simulation_time: '10', rate: '20'
    });
  };


  // ─── Manual node ───
  const addNode = () => {
    const nextId = nodes.length;
    const w = wrapper.current?.offsetWidth || 800;
    const h = wrapper.current?.offsetHeight || 600;
    setNodes(nds => [...nds, {
      id: `${nextId}`,
      position: { x: w / 2 + (Math.random() - 0.5) * 200, y: h / 2 + (Math.random() - 0.5) * 200 },
      data: { label: `Node ${nextId}`, weight: 1, active: false, queueCount: 0, isSource: false, isDestination: false },
      type: 'custom'
    }]);
  };

  const onConnect = useCallback((params) => {
    const weight = parseInt(prompt('Link weight:', '1')) || 1;
    setEdges(eds => addEdge({
      ...params, id: `e${params.source}-${params.target}`,
      label: `${weight}`, data: { weight, bandwidth: 10, latency: 1 }, type: 'default'
    }, eds));
  }, [setEdges]);

  const onNodeClick = useCallback((_, node) => {
    if (connectionSource) {
      if (connectionSource !== node.id) {
        const weight = parseInt(prompt('Link weight:', '1')) || 1;
        setEdges(eds => addEdge({
          source: connectionSource, target: node.id,
          id: `e${connectionSource}-${node.id}`,
          label: `${weight}`, data: { weight, bandwidth: 10, latency: 1 }, type: 'default'
        }, eds));
      }
      setConnectionSource(null);
    } else {
      setSelectedNode(node.id);
    }
  }, [connectionSource, setEdges]);

  const runWithCurrentGraph = () => {
    if (edges.length === 0) { setError('No edges.'); return; }
    const maxN = Math.max(...nodes.map(n => parseInt(n.id))) + 1;
    const topo = edges.map(e => `${e.source} ${e.target} ${e.data?.weight || 1}`).join('\n');
    runSimulation({ nodes: `${maxN}` }, topo);
  };


  // ─── Shortest Path ───
  const runShortestPath = (algo) => {
    if (edges.length === 0) { setError('No graph loaded.'); return; }
    const src = parseInt(sourceNode);
    const dst = parseInt(destNode);
    if (isNaN(src) || isNaN(dst)) { setError('Set source and destination first.'); return; }
    const adj = buildAdjList(edges);
    const n = nodes.length;

    const t0 = performance.now();
    let res;
    if (algo === 'dijkstra') res = dijkstraJS(adj, n, src);
    else if (algo === 'bellman-ford') res = bellmanFordJS(adj, n, src);
    else if (algo === 'bfs') res = bfsJS(adj, n, src);
    const elapsed = performance.now() - t0;

    const path = reconstructPath(res.parent, dst);
    const cost = res.dist[dst];
    const result = {
      algorithm: algo, path, cost: cost === Infinity ? -1 : cost,
      hops: path.length > 0 ? path.length - 1 : 0, time_ms: elapsed.toFixed(3)
    };
    setSpResult(result);

    // Highlight path edges
    setEdges(eds => eds.map(e => {
      const s = parseInt(e.source), t = parseInt(e.target);
      const onPath = path.some((p, i) => i < path.length - 1 && (
        (p === s && path[i + 1] === t) || (p === t && path[i + 1] === s)
      ));
      return {
        ...e,
        style: onPath
          ? { stroke: '#10b981', strokeWidth: 4, strokeDasharray: 'none' }
          : { stroke: 'var(--text-secondary)', strokeWidth: 2 }
      };
    }));
  };

  const runComparison = () => {
    if (edges.length === 0 || !sourceNode || !destNode) { setError('Set source/dest and load a graph first.'); return; }
    const src = parseInt(sourceNode), dst = parseInt(destNode);
    const adj = buildAdjList(edges);
    const n = nodes.length;
    const results = [];

    for (const algo of ['dijkstra', 'bellman-ford', 'bfs']) {
      const t0 = performance.now();
      const res = algo === 'dijkstra' ? dijkstraJS(adj, n, src)
                : algo === 'bellman-ford' ? bellmanFordJS(adj, n, src)
                : bfsJS(adj, n, src);
      const elapsed = performance.now() - t0;
      const path = reconstructPath(res.parent, dst);
      results.push({
        algorithm: algo,
        cost: res.dist[dst] === Infinity ? '∞' : res.dist[dst],
        hops: path.length > 0 ? path.length - 1 : 0,
        time_ms: elapsed.toFixed(3),
        path: path.join('→')
      });
    }
    setSpCompare(results);
  };


  // ─── SVG edge path lookup ───
  const getEdgePoint = (from, to, t) => {
    let p = document.querySelector(`[data-id="e${from}-${to}"] path.react-flow__edge-path`);
    let rev = false;
    if (!p) { p = document.querySelector(`[data-id="e${to}-${from}"] path.react-flow__edge-path`); rev = true; }
    if (!p) return null;
    return p.getPointAtLength((rev ? 1 - t : t) * p.getTotalLength());
  };


  // ─── Animation loop ───
  useEffect(() => {
    let raf;
    let lastTs = performance.now();
    const tick = (now) => {
      if (!isPausedRef.current && eventsRef.current.length > 0) {
        const dt = now - lastTs;
        const simDelta = msPerFrame * (dt / 16.667);
        let next = simTimeRef.current + simDelta;
        const maxT = eventsRef.current[eventsRef.current.length - 1].time + packetDuration + 2;
        if (next > maxT) { next = maxT; setIsPaused(true); }
        simTimeRef.current = next;
        setSimTime(next);

        const pkts = {};
        const activeNodes = new Set();

        // 1. Plot sequential packet animation using precomputed segments
        for (const seg of animSegmentsRef.current) {
          if (seg.time > next + 10) break; // optimize, depends on sort order if sorted
          if (next >= seg.time && next <= seg.time + seg.duration) {
            const prog = Math.max(0, Math.min(1, (next - seg.time) / seg.duration));
            const pt = getEdgePoint(seg.from, seg.to, prog);
            if (pt) pkts[`${seg.packetId}`] = { x: pt.x, y: pt.y };
          }
        }

        // 2. Active Nodes
        for (const ev of eventsRef.current) {
          if (ev.time > next + 5) break;
          if (ev.time <= next) {
            if (ev._animFrom !== undefined) activeNodes.add(String(ev._animFrom));
            if (ev._animTo !== undefined) activeNodes.add(String(ev._animTo));
            if (ev.from !== undefined && ev.from >= 0) activeNodes.add(String(ev.from));
          }
        }
        setActivePackets(pkts);

        const qs = {};
        for (const ev of eventsRef.current) {
          if (ev.time > next) break;
          if (ev.type === 'QUEUE_ENQUEUE') qs[ev.from] = (qs[ev.from] || 0) + 1;
          else if (ev.type === 'QUEUE_DEQUEUE') qs[ev.from] = Math.max(0, (qs[ev.from] || 0) - 1);
        }
        setNodes(nds => nds.map(n => ({
          ...n, data: { ...n.data, active: activeNodes.has(n.id), queueCount: qs[n.id] || 0 }
        })));
      }
      lastTs = now;
      raf = requestAnimationFrame(tick);
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [msPerFrame, packetDuration, setNodes]);


  // ─── Checkpoints ───
  const checkpoints = useMemo(() => [...new Set(events.map(e => e.time))].sort((a, b) => a - b), [events]);
  const goNext = () => { const t = checkpoints.find(c => c > simTime + 0.001); if (t !== undefined) { simTimeRef.current = t; setSimTime(t); setActivePackets({}); } };
  const goPrev = () => { const t = [...checkpoints].reverse().find(c => c < simTime - 0.001); if (t !== undefined) { simTimeRef.current = t; setSimTime(t); setActivePackets({}); } };
  const handleStop = () => { setIsPaused(true); simTimeRef.current = 0; setSimTime(0); setActivePackets({}); setNodes(nds => nds.map(n => ({ ...n, data: { ...n.data, active: false, queueCount: 0 } }))); };


  // ─── Derived ───
  const maxEvtTime = events.length > 0 ? events[events.length - 1].time : 1;
  const progress = events.length > 0 ? Math.min(1, simTime / (maxEvtTime + packetDuration)) : 0;
  let currentIdx = 0;
  while (currentIdx < events.length && events[currentIdx].time <= simTime) currentIdx++;
  const queueSize = Math.max(0, events.length - currentIdx);
  const progressiveLog = useMemo(() => events.slice(0, currentIdx).map(e => e.logLine).filter(Boolean), [events, currentIdx]);


  // ═══════════════════════════════════════════════════════════
  return (
    <div className="simulator-layout">

      {/* TOP BAR */}
      <div className="timeline-bar">
        <Activity color="var(--accent-color)" size={20} />
        <h2 style={{ margin: 0, border: 'none', color: '#fff', whiteSpace: 'nowrap', fontSize: 14 }}>NSim Pro</h2>

        <div className="timeline-controls">
          <button className="btn" onClick={() => { if (events.length) setIsPaused(!isPaused); }} title={isPaused ? 'Play' : 'Pause'}>
            {isPaused ? <Play size={14} /> : <Pause size={14} />}
          </button>
          <button className="btn" onClick={handleStop} disabled={!events.length} title="Stop"><Square size={14} /></button>
          <button className="btn" onClick={goPrev} disabled={!events.length} title="Prev Checkpoint"><SkipBack size={14} /></button>
          <button className="btn" onClick={goNext} disabled={!events.length} title="Next Checkpoint"><SkipForward size={14} /></button>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: 5, fontSize: 11 }}>
          <span style={{ color: 'var(--text-secondary)' }}>Speed</span>
          <input type="range" min="0.005" max="2" step="0.005" value={msPerFrame}
            onChange={e => setMsPerFrame(Number(e.target.value))}
            style={{ width: 60, accentColor: 'var(--accent-color)' }} />
          <span style={{ color: 'var(--accent-color)', fontWeight: 600, fontSize: 10, minWidth: 40 }}>{msPerFrame.toFixed(3)}</span>
        </div>
        <div style={{ display: 'flex', alignItems: 'center', gap: 5, fontSize: 11 }}>
          <span style={{ color: 'var(--text-secondary)' }}>Flight</span>
          <input type="range" min="1" max="40" step="1" value={packetDuration}
            onChange={e => setPacketDuration(Number(e.target.value))}
            style={{ width: 50, accentColor: 'var(--event-color)' }} />
          <span style={{ color: 'var(--event-color)', fontWeight: 600, fontSize: 10, minWidth: 30 }}>{packetDuration}ms</span>
        </div>

        <div className="timeline-progress">
          <div style={{ color: 'var(--accent-color)', fontWeight: 600, fontSize: 12, minWidth: 90 }}>T={simTime.toFixed(2)}ms</div>
          <div className="time-scrubber" onClick={e => {
            if (!events.length) return;
            const rect = e.currentTarget.getBoundingClientRect();
            const t = ((e.clientX - rect.left) / rect.width) * (maxEvtTime + packetDuration);
            simTimeRef.current = t; setSimTime(t); setActivePackets({});
          }}>
            <div className="time-fill" style={{ width: `${progress * 100}%` }} />
          </div>
          <span style={{ fontSize: 11, color: 'var(--text-secondary)', minWidth: 70 }}>Evt {currentIdx}/{events.length}</span>
        </div>
      </div>


      {/* SIDEBAR */}
      <div className="sidebar">
        <div className="tab-container">
          <button className={`tab ${tab === 'config' ? 'active' : ''}`} onClick={() => setTab('config')}>
            <Settings size={12} /> Config
          </button>
          <button className={`tab ${tab === 'queue' ? 'active' : ''}`} onClick={() => setTab('queue')}>
            <ListTree size={12} /> Queue ({queueSize})
          </button>
          <button className={`tab ${tab === 'metrics' ? 'active' : ''}`} onClick={() => setTab('metrics')}>
            <BarChart3 size={12} /> Metrics
          </button>
          <button className={`tab ${tab === 'path' ? 'active' : ''}`} onClick={() => setTab('path')}>
            <Route size={12} /> Path
          </button>
        </div>

        <div className="scrollable-pane">
          {error && (
            <div style={{ padding: 8, background: 'rgba(248,81,73,0.1)', border: '1px solid var(--accent-secondary)', borderRadius: 6, marginBottom: 10, fontSize: 12 }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: 4, color: 'var(--accent-secondary)', fontWeight: 700, marginBottom: 3 }}>
                <AlertCircle size={14} /> Error
              </div>
              <div style={{ whiteSpace: 'pre-wrap', maxHeight: 80, overflowY: 'auto' }}>{error}</div>
            </div>
          )}
          {status && (
            <div style={{ padding: 8, display: 'flex', gap: 6, alignItems: 'center', background: 'rgba(88,166,255,0.08)', border: '1px solid var(--accent-color)', borderRadius: 6, marginBottom: 10, fontSize: 12 }}>
              {isLoading ? <Loader2 size={14} className="lucide-spin" /> : <Info size={14} color="var(--accent-color)" />}
              {status}
            </div>
          )}

          {/* CONFIG TAB */}
          {tab === 'config' && (
            <>
              <input type="file" ref={fileInputRef} accept=".txt,.cfg,.conf" onChange={handleConfigUpload} style={{ display: 'none' }} />
              <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 8, fontSize: 11, borderColor: 'var(--accent-color)', color: 'var(--accent-color)' }}
                onClick={() => fileInputRef.current?.click()}>
                <Upload size={13} /> Load Config File (.txt)
              </button>

              <h3 style={{ fontSize: 11, marginBottom: 5, color: 'var(--text-secondary)' }}>Simulator Config</h3>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 4, marginBottom: 6 }}>
                <div><label style={{ fontSize: 10 }}>Nodes</label><input type="number" value={cfg.nodes} onChange={e => setCfg({ ...cfg, nodes: e.target.value })} /></div>
                <div>
                  <label style={{ fontSize: 10 }}>Topology</label>
                  <select value={cfg.topology} onChange={e => setCfg({ ...cfg, topology: e.target.value })}
                    style={{ width: '100%', padding: 5, background: 'rgba(0,0,0,.3)', border: '1px solid var(--panel-border)', color: 'var(--text-primary)', borderRadius: 4, fontSize: 12 }}>
                    <option value="tree">Tree</option>
                    <option value="random">Random (Erdős–Rényi)</option>
                    <option value="grid">Grid / Mesh</option>
                  </select>
                </div>
                {cfg.topology === 'random' && (
                  <div><label style={{ fontSize: 10 }}>Probability</label><input type="number" step="0.01" value={cfg.probability} onChange={e => setCfg({ ...cfg, probability: e.target.value })} /></div>
                )}
                {cfg.topology === 'grid' && (<>
                  <div><label style={{ fontSize: 10 }}>Rows</label><input type="number" value={cfg.grid_rows} onChange={e => setCfg({ ...cfg, grid_rows: e.target.value })} /></div>
                  <div><label style={{ fontSize: 10 }}>Cols</label><input type="number" value={cfg.grid_cols} onChange={e => setCfg({ ...cfg, grid_cols: e.target.value })} /></div>
                </>)}
                <div><label style={{ fontSize: 10 }}>Sim Time</label><input type="number" value={cfg.simulation_time} onChange={e => setCfg({ ...cfg, simulation_time: e.target.value })} /></div>
                <div><label style={{ fontSize: 10 }}>Bandwidth</label><input type="number" value={cfg.bandwidth} onChange={e => setCfg({ ...cfg, bandwidth: e.target.value })} /></div>
                <div><label style={{ fontSize: 10 }}>Delay (ms)</label><input type="number" value={cfg.delay} onChange={e => setCfg({ ...cfg, delay: e.target.value })} /></div>
                <div>
                  <label style={{ fontSize: 10 }}>Traffic</label>
                  <select value={cfg.traffic_type} onChange={e => setCfg({ ...cfg, traffic_type: e.target.value })}
                    style={{ width: '100%', padding: 5, background: 'rgba(0,0,0,.3)', border: '1px solid var(--panel-border)', color: 'var(--text-primary)', borderRadius: 4, fontSize: 12 }}>
                    <option value="constant">Constant</option>
                    <option value="random">Random</option>
                    <option value="burst">Burst</option>
                  </select>
                </div>
                <div><label style={{ fontSize: 10 }}>Rate (pkt/s)</label><input type="number" value={cfg.rate} onChange={e => setCfg({ ...cfg, rate: e.target.value })} /></div>
                <div><label style={{ fontSize: 10 }}>Pkt Size</label><input type="number" value={cfg.packet_size} onChange={e => setCfg({ ...cfg, packet_size: e.target.value })} /></div>
              </div>

              {/* Source / Destination */}
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 4, marginBottom: 8, borderTop: '1px solid var(--panel-border)', paddingTop: 6 }}>
                <div>
                  <label style={{ fontSize: 10, display: 'flex', alignItems: 'center', gap: 3 }}><MapPin size={10} color="#10b981" /> Source</label>
                  <input type="number" value={sourceNode} onChange={e => setSourceNode(e.target.value)} style={{ borderColor: '#10b981' }} />
                </div>
                <div>
                  <label style={{ fontSize: 10, display: 'flex', alignItems: 'center', gap: 3 }}><Target size={10} color="#f59e0b" /> Destination</label>
                  <input type="number" value={destNode} onChange={e => setDestNode(e.target.value)} style={{ borderColor: '#f59e0b' }} />
                </div>
              </div>

              <button className="btn primary" style={{ width: '100%', justifyContent: 'center', height: 34, marginBottom: 5 }}
                onClick={() => runSimulation()} disabled={isLoading}>
                <Play size={14} /> {isLoading ? 'Building...' : 'Build Network'}
              </button>
              <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 8, fontSize: 11, borderColor: 'var(--event-color)', color: 'var(--event-color)' }}
                onClick={runStressTest} disabled={isLoading}>
                <Zap size={13} /> Stress Test (1K+ Nodes)
              </button>

              <div style={{ borderTop: '1px solid var(--panel-border)', paddingTop: 6 }}>
                <h3 style={{ fontSize: 11, marginBottom: 5, color: 'var(--text-secondary)' }}>Manual Topology</h3>
                <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 4, fontSize: 11 }} onClick={addNode}><Plus size={13} /> Add Node</button>
                {connectionSource
                  ? <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 4, fontSize: 11, borderColor: 'var(--accent-secondary)', color: 'var(--accent-secondary)' }}
                      onClick={() => setConnectionSource(null)}>Cancel (from N{connectionSource})</button>
                  : <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 4, fontSize: 11 }}
                      onClick={() => { if (selectedNode) setConnectionSource(selectedNode); else alert('Click a node first'); }}>Connect From Selected</button>
                }
                <button className="btn" style={{ width: '100%', justifyContent: 'center', fontSize: 11, background: 'rgba(168,85,247,0.08)', borderColor: 'var(--event-color)', color: 'var(--event-color)' }}
                  onClick={runWithCurrentGraph} disabled={isLoading || nodes.length < 2}>
                  <Play size={13} /> Run Manual Graph
                </button>
              </div>
            </>
          )}

          {/* QUEUE TAB */}
          {tab === 'queue' && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
              {events.length === 0 && <span style={{ color: 'var(--text-secondary)', fontSize: 12, padding: 8 }}>No events.</span>}
              {events.slice(Math.max(0, currentIdx - 5), currentIdx + 30).map((ev, i) => {
                const idx = Math.max(0, currentIdx - 5) + i;
                const isA = idx === currentIdx;
                return (
                  <div key={`q-${idx}`} className={`queue-item ${isA ? 'active' : ''}`}
                    onClick={() => { simTimeRef.current = ev.time; setSimTime(ev.time); setActivePackets({}); }}>
                    <span style={{ color: 'var(--text-secondary)', marginRight: 6, fontSize: 11 }}>T={ev.time.toFixed(2)}</span>
                    <span style={{ color: isA ? 'var(--accent-color)' : 'var(--text-primary)', fontSize: 11 }}>{ev.type}</span>
                    <span style={{ color: 'var(--text-secondary)', marginLeft: 'auto', fontSize: 10 }}>#{ev.packetId} {ev.from}→{ev.to}</span>
                  </div>
                );
              })}
            </div>
          )}

          {/* METRICS TAB */}
          {tab === 'metrics' && (
            <div style={{ fontSize: 12 }}>
              {!metrics ? <span style={{ color: 'var(--text-secondary)' }}>Run simulator first.</span> : (
                <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 12 }}>
                  <tbody>
                    {[
                      ['Total Packets', metrics.packets_sent],
                      ['Delivered', metrics.packets_delivered],
                      ['Dropped', metrics.packets_dropped],
                      ['Avg Latency (ms)', metrics.latency?.toFixed(4)],
                      ['Min Latency (ms)', metrics.min_latency],
                      ['Max Latency (ms)', metrics.max_latency],
                      ['Jitter / StdDev (ms)', metrics.jitter?.toFixed(4)],
                      ['Packet Loss (%)', (metrics.packet_loss * 100)?.toFixed(2)],
                      ['Avg Hops', metrics.average_hops],
                      ['Throughput (bit/ms)', metrics.throughput?.toFixed(2)],
                    ].map(([k, v]) => (
                      <tr key={k} style={{ borderBottom: '1px solid rgba(255,255,255,.04)' }}>
                        <td style={{ padding: '4px 2px', color: 'var(--text-secondary)' }}>{k}</td>
                        <td style={{ padding: '4px 2px', fontWeight: 600, color: 'var(--accent-color)', textAlign: 'right' }}>{v ?? '—'}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              )}

              {/* Shortest path comparison results */}
              {spCompare && (
                <>
                  <h4 style={{ fontSize: 11, margin: '12px 0 6px', color: 'var(--event-color)', borderTop: '1px solid var(--panel-border)', paddingTop: 8 }}>
                    Shortest Path Comparison ({sourceNode}→{destNode})
                  </h4>
                  <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 11 }}>
                    <thead>
                      <tr style={{ borderBottom: '1px solid var(--panel-border)' }}>
                        <th style={{ textAlign: 'left', padding: '3px 2px', color: 'var(--text-secondary)' }}>Algorithm</th>
                        <th style={{ textAlign: 'right', padding: '3px 2px', color: 'var(--text-secondary)' }}>Cost</th>
                        <th style={{ textAlign: 'right', padding: '3px 2px', color: 'var(--text-secondary)' }}>Hops</th>
                        <th style={{ textAlign: 'right', padding: '3px 2px', color: 'var(--text-secondary)' }}>Time</th>
                      </tr>
                    </thead>
                    <tbody>
                      {spCompare.map(r => (
                        <tr key={r.algorithm} style={{ borderBottom: '1px solid rgba(255,255,255,.03)' }}>
                          <td style={{ padding: '3px 2px', color: 'var(--accent-color)', fontWeight: 600 }}>{r.algorithm}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right' }}>{r.cost}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right' }}>{r.hops}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right', color: 'var(--event-color)' }}>{r.time_ms}ms</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                  <div style={{ fontSize: 10, color: 'var(--text-secondary)', marginTop: 4 }}>
                    {spCompare[0] && `Path: ${spCompare[0].path}`}
                  </div>
                </>
              )}
            </div>
          )}

          {/* PATH TAB */}
          {tab === 'path' && (
            <div style={{ fontSize: 12 }}>
              <h3 style={{ fontSize: 12, marginBottom: 8 }}>Shortest Path Analysis</h3>

              <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 4, marginBottom: 8 }}>
                <div>
                  <label style={{ fontSize: 10, color: '#10b981' }}>Source</label>
                  <input type="number" value={sourceNode} onChange={e => setSourceNode(e.target.value)} style={{ borderColor: '#10b981' }} />
                </div>
                <div>
                  <label style={{ fontSize: 10, color: '#f59e0b' }}>Destination</label>
                  <input type="number" value={destNode} onChange={e => setDestNode(e.target.value)} style={{ borderColor: '#f59e0b' }} />
                </div>
              </div>

              <div style={{ marginBottom: 8 }}>
                <label style={{ fontSize: 10 }}>Algorithm</label>
                <select value={spAlgorithm} onChange={e => setSpAlgorithm(e.target.value)}
                  style={{ width: '100%', padding: 5, background: 'rgba(0,0,0,.3)', border: '1px solid var(--panel-border)', color: 'var(--text-primary)', borderRadius: 4, fontSize: 12 }}>
                  <option value="dijkstra">Dijkstra</option>
                  <option value="bellman-ford">Bellman-Ford</option>
                  <option value="bfs">BFS (unweighted)</option>
                </select>
              </div>

              <button className="btn primary" style={{ width: '100%', justifyContent: 'center', height: 32, marginBottom: 5 }}
                onClick={() => runShortestPath(spAlgorithm)} disabled={edges.length === 0}>
                <Route size={13} /> Find Shortest Path
              </button>

              <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 8, fontSize: 11, borderColor: '#a855f7', color: '#a855f7' }}
                onClick={runComparison} disabled={edges.length === 0}>
                <BarChart3 size={13} /> Compare All Algorithms
              </button>

              {spResult && (
                <div style={{ background: 'rgba(16,185,129,0.08)', border: '1px solid #10b981', borderRadius: 6, padding: 8, marginBottom: 8 }}>
                  <div style={{ fontWeight: 700, color: '#10b981', marginBottom: 4, fontSize: 12 }}>{spResult.algorithm.toUpperCase()}</div>
                  <div style={{ fontSize: 11, lineHeight: 1.6 }}>
                    <div>Cost: <b>{spResult.cost === -1 ? 'No path' : spResult.cost}</b></div>
                    <div>Hops: <b>{spResult.hops}</b></div>
                    <div>Compute: <b>{spResult.time_ms}ms</b></div>
                    <div style={{ marginTop: 4, color: 'var(--text-secondary)', wordBreak: 'break-all' }}>
                      Path: {spResult.path.join(' → ')}
                    </div>
                  </div>
                </div>
              )}

              {spCompare && (
                <div style={{ background: 'rgba(168,85,247,0.08)', border: '1px solid #a855f7', borderRadius: 6, padding: 8 }}>
                  <div style={{ fontWeight: 700, color: '#a855f7', marginBottom: 6, fontSize: 12 }}>Algorithm Comparison</div>
                  <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 11 }}>
                    <thead>
                      <tr>
                        <th style={{ textAlign: 'left', padding: '2px', color: 'var(--text-secondary)' }}>Algo</th>
                        <th style={{ textAlign: 'right', padding: '2px', color: 'var(--text-secondary)' }}>Cost</th>
                        <th style={{ textAlign: 'right', padding: '2px', color: 'var(--text-secondary)' }}>Hops</th>
                        <th style={{ textAlign: 'right', padding: '2px', color: 'var(--text-secondary)' }}>Time</th>
                      </tr>
                    </thead>
                    <tbody>
                      {spCompare.map(r => (
                        <tr key={r.algorithm}>
                          <td style={{ padding: '3px 2px', fontWeight: 600, color: 'var(--accent-color)' }}>{r.algorithm}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right' }}>{r.cost}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right' }}>{r.hops}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right', color: 'var(--event-color)' }}>{r.time_ms}ms</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}
            </div>
          )}
        </div>
      </div>


      {/* CANVAS */}
      <div className="canvas-area" ref={wrapper}>
        <ReactFlow
          nodes={nodes} edges={edges}
          onNodesChange={onNodesChange} onEdgesChange={onEdgesChange}
          onConnect={onConnect}
          onNodeClick={(e, n) => { onNodeClick(e, n); e.stopPropagation(); }}
          onPaneClick={() => { setSelectedNode(null); setConnectionSource(null); }}
          nodeTypes={nodeTypes}
          defaultEdgeOptions={defaultEdgeOptions}
          fitView fitViewOptions={{ padding: 0.1 }}
          minZoom={0.01}
          maxZoom={10}
        >
          <Background color="var(--text-secondary)" gap={25} size={1.5} opacity={0.1} />
          <Controls />
          <PacketOverlay packets={activePackets} />
        </ReactFlow>

        {selectedNode && (() => {
          const nd = nodes.find(n => n.id === selectedNode);
          return (
            <div className="object-inspector" style={{ position: 'absolute', bottom: 16, left: 16, top: 'auto', background: 'rgba(10,13,18,.95)' }}>
              <h3 style={{ display: 'flex', alignItems: 'center', gap: 5, color: 'var(--accent-color)', fontSize: 12 }}>
                <Info size={13} /> Node {selectedNode}
                {nd?.data?.isSource && <span style={{ color: '#10b981', fontSize: 10 }}>(Source)</span>}
                {nd?.data?.isDestination && <span style={{ color: '#f59e0b', fontSize: 10 }}>(Destination)</span>}
              </h3>
              <div style={{ fontSize: 11, color: 'var(--text-secondary)', marginBottom: 4 }}>
                State: {nd?.data?.active ? 'ACTIVE' : 'IDLE'} | Queue: {nd?.data?.queueCount || 0}
              </div>
              <div style={{ display: 'flex', gap: 4 }}>
                <button onClick={() => setSourceNode(selectedNode)} style={{ fontSize: 10, padding: '2px 6px', background: 'rgba(16,185,129,0.2)', border: '1px solid #10b981', borderRadius: 3, color: '#10b981', cursor: 'pointer' }}>Set Source</button>
                <button onClick={() => setDestNode(selectedNode)} style={{ fontSize: 10, padding: '2px 6px', background: 'rgba(245,158,11,0.2)', border: '1px solid #f59e0b', borderRadius: 3, color: '#f59e0b', cursor: 'pointer' }}>Set Dest</button>
              </div>
            </div>
          );
        })()}
      </div>


      {/* BOTTOM LOG */}
      <div className="log-panel">
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', borderBottom: '1px solid var(--panel-border)', paddingBottom: 3, marginBottom: 3 }}>
          <h3 style={{ fontSize: 11, display: 'flex', gap: 5, alignItems: 'center', color: 'var(--text-secondary)', margin: 0 }}>
            <Terminal size={12} /> {showFullLog ? 'Full C++ Console Output' : 'Simulation Event Log (Progressive)'}
          </h3>
          <button onClick={() => setShowFullLog(!showFullLog)}
            style={{ fontSize: 10, padding: '2px 8px', background: 'rgba(255,255,255,.05)', border: '1px solid var(--panel-border)', borderRadius: 4, color: 'var(--text-secondary)', cursor: 'pointer' }}>
            {showFullLog ? 'Progressive' : 'Full Output'}
          </button>
        </div>
        <div className="log-scroll" style={{ whiteSpace: 'pre-wrap', fontFamily: "'JetBrains Mono', monospace", fontSize: 11, color: 'var(--text-secondary)', lineHeight: 1.45 }}>
          {showFullLog
            ? (fullLog || 'Run the simulator to see output.')
            : (progressiveLog.length > 0 ? progressiveLog.join('\n') : 'Waiting for simulation events...')}
        </div>
      </div>
    </div>
  );
};

export default function App() {
  return (
    <ReactFlowProvider>
      <NetworkSimulatorApp />
    </ReactFlowProvider>
  );
}