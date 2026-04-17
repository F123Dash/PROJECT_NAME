import React, { useState, useEffect, useRef, useCallback, useMemo } from 'react';
import {
  ReactFlow, Controls, Background,
  useNodesState, useEdgesState, addEdge,
  useReactFlow, ReactFlowProvider,
  MarkerType, Handle, Position
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

  // MST
  const [mstResult, setMstResult] = useState(null);
  const [mstCompare, setMstCompare] = useState(null);
  const [mstShowOverlay, setMstShowOverlay] = useState(false);
  const [mstVsSpResult, setMstVsSpResult] = useState(null);
  const [complexityData, setComplexityData] = useState(null);

  // Simulation telemetry
  const [events, setEvents] = useState([]);
  const [metrics, setMetrics] = useState(null);
  const [fullLog, setFullLog] = useState('');

  // Animation
  const [simTime, setSimTime] = useState(0);
  const [isPaused, setIsPaused] = useState(true);
  const [packetDuration, setPacketDuration] = useState(12);
  const [msPerFrame, setMsPerFrame] = useState(0.05);

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
    setSpResult(null);
    setSpCompare(null);
    setStatus('Running C++ simulator...');

    try {
      const sourceValue = parseInt(sourceNode, 10);
      const destinationValue = parseInt(destNode, 10);
      const fallbackDestination = Math.max(0, (parseInt(cfg.nodes, 10) || 1) - 1);

      const payload = {
        configOverrides: {
          ...cfg,
          ...overrides,
          source: Number.isFinite(sourceValue) ? sourceValue : 0,
          destination: Number.isFinite(destinationValue) ? destinationValue : fallbackDestination
        }
      };
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

  // ─── Direct Backend Analysis C++ Bridge ───
  const callAnalysisTool = async () => {
    if (edges.length === 0) { setError('Load a graph first.'); return null; }
    const src = parseInt(sourceNode) || 0;
    const dst = parseInt(destNode) || (nodes.length > 1 ? 1 : 0);
    const n = nodes.length;

    const exportEdges = edges.map(e => ({
      from: parseInt(e.source || e.from),
      to: parseInt(e.target || e.to),
      weight: e.data?.weight || parseInt(e.label) || 1
    }));

    try {
      const res = await axios.post('http://localhost:3000/analyze', {
        nodeCount: n,
        source: src,
        dest: dst,
        edges: exportEdges
      });
      if (res.data.error) {
        setError(res.data.error);
        return null;
      }
      return res.data.data;
    } catch(err) {
      setError(err.message);
      return null;
    }
  };


  // ─── Shortest Path ───
  const runShortestPath = async (algo) => {
    if (edges.length === 0) { setError('No graph loaded.'); return; }
    setIsLoading(true);
    const data = await callAnalysisTool();
    setIsLoading(false);
    if (!data) return;

    let pathStr = "", cost = -1, hops = 0, time_ms = "0.000";
    if (algo === 'dijkstra') {
        pathStr = data.shortest_path.path;
        cost = data.shortest_path.cost;
        hops = data.shortest_path.hops;
        time_ms = data.shortest_path.dijkstra_time_ms.toFixed(3);
    } else if (algo === 'bellman-ford') {
        pathStr = "N/A (Check console/all algos)";
        cost = "N/A";
        time_ms = data.complexity.results.find(r => r.name === 'Bellman-Ford')?.measured_ms.toFixed(3) || 0;
    } else if (algo === 'bfs') {
        pathStr = "N/A";
        cost = data.bfs_distance;
        time_ms = data.complexity.results.find(r => r.name === 'BFS')?.measured_ms.toFixed(3) || 0;
    }

    const pathArr = (algo === 'dijkstra' && pathStr) ? pathStr.split(" -> ").map(Number) : [];

    setSpResult({
      algorithm: algo,
      path: pathArr, // array here so we can highlight edges
      cost,
      hops,
      time_ms
    });

    // Highlight path edges (only visual for algorithms that output exact path)
    setEdges(eds => eds.map(e => {
      const s = parseInt(e.source), t = parseInt(e.target);
      const onPath = pathArr.some((p, i) => i < pathArr.length - 1 && (
        (p === s && pathArr[i + 1] === t) || (p === t && pathArr[i + 1] === s)
      ));
      return {
        ...e,
        style: onPath
          ? { stroke: '#10b981', strokeWidth: 4, strokeDasharray: 'none' }
          : { stroke: 'var(--text-secondary)', strokeWidth: 2 }
      };
    }));
  };

  const runComparison = async () => {
    if (edges.length === 0 || !sourceNode || !destNode) { setError('Set source/dest and load a graph first.'); return; }
    setIsLoading(true);
    const data = await callAnalysisTool();
    setIsLoading(false);
    if (!data) return;

    setSpCompare([
      {
        algorithm: 'dijkstra',
        cost: data.shortest_path.cost === -1 ? '∞' : data.shortest_path.cost,
        hops: data.shortest_path.hops,
        time_ms: data.shortest_path.dijkstra_time_ms.toFixed(3),
        path: data.shortest_path.path
      },
      {
        algorithm: 'bellman-ford',
        cost: 'N/A',
        hops: 'N/A',
        time_ms: data.complexity.results.find(r=>r.name==='Bellman-Ford')?.measured_ms.toFixed(3) || 0,
        path: 'N/A'
      },
      {
        algorithm: 'bfs',
        cost: data.bfs_distance === -1 ? '∞' : data.bfs_distance,
        hops: data.bfs_distance === -1 ? 'N/A' : data.bfs_distance,
        time_ms: data.complexity.results.find(r=>r.name==='BFS')?.measured_ms.toFixed(3) || 0,
        path: 'N/A'
      }
    ]);
  };

  // ─── MST Computation ───
  const runMST = async (algo) => {
    if (edges.length === 0) { setError('Load a graph first.'); return; }
    setIsLoading(true);
    const data = await callAnalysisTool();
    setIsLoading(false);
    if (!data) return;

    const res = data[algo];
    if (!res) return;

    setMstResult({
      algorithm: algo,
      edges: res.edges,
      totalCost: res.totalCost,
      edgeCount: res.edgeCount,
      time_ms: res.time_ms.toFixed(3)
    });

    if (mstShowOverlay) {
      applyMstOverlay(res.edges);
    }
  };

  const applyMstOverlay = (mstEdgeList) => {
    const mstSet = new Set();
    for (const e of mstEdgeList) {
      mstSet.add(Math.min(e.u, e.v) + '-' + Math.max(e.u, e.v));
    }
    setEdges(eds => eds.map(e => {
      const s = parseInt(e.source), t = parseInt(e.target);
      const key = Math.min(s, t) + '-' + Math.max(s, t);
      const isMst = mstSet.has(key);
      return {
        ...e,
        style: isMst
          ? { stroke: '#f59e0b', strokeWidth: 3.5, strokeDasharray: 'none' }
          : { stroke: 'rgba(255,255,255,0.08)', strokeWidth: 1 }
      };
    }));
  };

  const clearEdgeStyles = () => {
    setEdges(eds => eds.map(e => ({
      ...e,
      style: { stroke: 'var(--text-secondary)', strokeWidth: 2 }
    })));
  };

  const runMSTComparison = async () => {
    if (edges.length === 0) { setError('Load a graph first.'); return; }
    setIsLoading(true);
    const data = await callAnalysisTool();
    setIsLoading(false);
    if (!data) return;
    
    setMstCompare([
       { algorithm: 'kruskal', totalCost: data.kruskal.totalCost, edgeCount: data.kruskal.edgeCount, time_ms: data.kruskal.time_ms.toFixed(3) },
       { algorithm: 'prim', totalCost: data.prim.totalCost, edgeCount: data.prim.edgeCount, time_ms: data.prim.time_ms.toFixed(3) }
    ]);
  };

  const runMSTvsShortestPath = async () => {
    if (edges.length === 0 || !sourceNode || !destNode) { setError('Set source/dest and load a graph first.'); return; }
    setIsLoading(true);
    const data = await callAnalysisTool();
    setIsLoading(false);
    if (!data) return;

    setMstVsSpResult({
      shortest: {
        path: data.shortest_path.path,
        cost: data.shortest_path.cost === -1 ? 'No path' : data.shortest_path.cost,
        hops: data.shortest_path.hops,
        time_ms: data.shortest_path.dijkstra_time_ms.toFixed(3)
      },
      mst: {
        path: data.mst_path.path,
        cost: data.mst_path.cost === -1 ? 'No path' : data.mst_path.cost,
        hops: data.mst_path.hops,
        totalMstCost: data.kruskal.totalCost,
        time_ms: data.kruskal.time_ms.toFixed(3)
      },
      pathStretch: (data.mst_path.path_stretch == null || data.mst_path.path_stretch === -1 || isNaN(data.mst_path.path_stretch)) 
          ? 'N/A' : data.mst_path.path_stretch.toFixed(3)
    });
  };

  // ─── Complexity Analysis ───
  const runComplexityAnalysis = async () => {
    if (edges.length === 0) { setError('Load a graph first.'); return; }
    setIsLoading(true);
    const data = await callAnalysisTool();
    setIsLoading(false);
    if (!data) return;

    setComplexityData({
       V: data.complexity.V,
       E: data.complexity.E,
       results: data.complexity.results.map(r => ({
           name: r.name,
           bigO: r.bigO,
           measured: r.measured_ms.toFixed(3)
       }))
    });
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

        const activeNodes = new Set();

        // 2. Active Nodes
        for (const ev of eventsRef.current) {
          if (ev.time > next + 5) break;
          if (ev.time <= next) {
            if (ev._animFrom !== undefined) activeNodes.add(String(ev._animFrom));
            if (ev._animTo !== undefined) activeNodes.add(String(ev._animTo));
            if (ev.from !== undefined && ev.from >= 0) activeNodes.add(String(ev.from));
          }
        }

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
  const goNext = () => { const t = checkpoints.find(c => c > simTime + 0.001); if (t !== undefined) { simTimeRef.current = t; setSimTime(t); } };
  const goPrev = () => { const t = [...checkpoints].reverse().find(c => c < simTime - 0.001); if (t !== undefined) { simTimeRef.current = t; setSimTime(t); } };
  const handleStop = () => { setIsPaused(true); simTimeRef.current = 0; setSimTime(0); setNodes(nds => nds.map(n => ({ ...n, data: { ...n.data, active: false, queueCount: 0 } }))); };


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
            simTimeRef.current = t; setSimTime(t);
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
          <button className={`tab ${tab === 'mst' ? 'active' : ''}`} onClick={() => setTab('mst')}>
            <Activity size={12} /> MST
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
                    onClick={() => { simTimeRef.current = ev.time; setSimTime(ev.time); }}>
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

              {/* COMPLEXITY ANALYSIS */}
              <div style={{ marginTop: 12, borderTop: '1px solid var(--panel-border)', paddingTop: 8 }}>
                <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 8, fontSize: 11, borderColor: '#06b6d4', color: '#06b6d4' }}
                  onClick={runComplexityAnalysis} disabled={edges.length === 0}>
                  <Zap size={13} /> Run Complexity Analysis
                </button>
                {complexityData && (
                  <div style={{ background: 'rgba(6,182,212,0.06)', border: '1px solid rgba(6,182,212,0.3)', borderRadius: 6, padding: 8 }}>
                    <div style={{ fontWeight: 700, color: '#06b6d4', marginBottom: 6, fontSize: 11 }}>
                      Complexity Analysis (V={complexityData.V}, E={complexityData.E})
                    </div>
                    <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 10 }}>
                      <thead>
                        <tr style={{ borderBottom: '1px solid rgba(6,182,212,0.2)' }}>
                          <th style={{ textAlign: 'left', padding: '3px 2px', color: 'var(--text-secondary)' }}>Algorithm</th>
                          <th style={{ textAlign: 'right', padding: '3px 2px', color: 'var(--text-secondary)' }}>Big-O</th>
                          <th style={{ textAlign: 'right', padding: '3px 2px', color: 'var(--text-secondary)' }}>Time (ms)</th>
                        </tr>
                      </thead>
                      <tbody>
                        {complexityData.results.map(r => (
                          <tr key={r.name} style={{ borderBottom: '1px solid rgba(255,255,255,.03)' }}>
                            <td style={{ padding: '3px 2px', fontWeight: 600, color: '#06b6d4' }}>{r.name}</td>
                            <td style={{ padding: '3px 2px', textAlign: 'right', fontFamily: 'monospace', color: 'var(--text-secondary)', fontSize: 9 }}>{r.bigO}</td>
                            <td style={{ padding: '3px 2px', textAlign: 'right', fontWeight: 600, color: 'var(--accent-color)' }}>{r.measured}ms</td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>
                )}
              </div>
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
                      Path: {Array.isArray(spResult.path) ? spResult.path.join(' → ') : spResult.path}
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

          {/* MST TAB */}
          {tab === 'mst' && (
            <div style={{ fontSize: 12 }}>
              <h3 style={{ fontSize: 12, marginBottom: 8 }}>Minimum Spanning Tree</h3>

              <div style={{ display: 'flex', gap: 4, marginBottom: 8 }}>
                <button className="btn primary" style={{ flex: 1, justifyContent: 'center', height: 30, fontSize: 11 }}
                  onClick={() => runMST('kruskal')} disabled={edges.length === 0}>
                  Kruskal
                </button>
                <button className="btn primary" style={{ flex: 1, justifyContent: 'center', height: 30, fontSize: 11 }}
                  onClick={() => runMST('prim')} disabled={edges.length === 0}>
                  Prim
                </button>
              </div>

              <div style={{ display: 'flex', gap: 4, marginBottom: 8 }}>
                <button className="btn" style={{ flex: 1, justifyContent: 'center', fontSize: 10, borderColor: '#f59e0b', color: '#f59e0b' }}
                  onClick={() => {
                    setMstShowOverlay(!mstShowOverlay);
                    if (!mstShowOverlay && mstResult) applyMstOverlay(mstResult.edges);
                    else clearEdgeStyles();
                  }}>
                  {mstShowOverlay ? 'Hide MST' : 'Show MST Overlay'}
                </button>
              </div>

              {mstResult && (
                <div style={{ background: 'rgba(245,158,11,0.08)', border: '1px solid #f59e0b', borderRadius: 6, padding: 8, marginBottom: 8 }}>
                  <div style={{ fontWeight: 700, color: '#f59e0b', marginBottom: 4, fontSize: 12 }}>
                    {mstResult.algorithm.toUpperCase()} Result
                  </div>
                  <div style={{ fontSize: 11, lineHeight: 1.6 }}>
                    <div>Total Cost: <b>{mstResult.totalCost}</b></div>
                    <div>MST Edges: <b>{mstResult.edgeCount}</b> / {edges.length} total</div>
                    <div>Compute: <b>{mstResult.time_ms}ms</b></div>
                  </div>
                </div>
              )}

              <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 8, fontSize: 11, borderColor: '#a855f7', color: '#a855f7' }}
                onClick={runMSTComparison} disabled={edges.length === 0}>
                <BarChart3 size={13} /> Compare Kruskal vs Prim
              </button>

              {mstCompare && (
                <div style={{ background: 'rgba(168,85,247,0.08)', border: '1px solid #a855f7', borderRadius: 6, padding: 8, marginBottom: 8 }}>
                  <div style={{ fontWeight: 700, color: '#a855f7', marginBottom: 6, fontSize: 11 }}>Kruskal vs Prim</div>
                  <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 11 }}>
                    <thead>
                      <tr>
                        <th style={{ textAlign: 'left', padding: '2px', color: 'var(--text-secondary)' }}>Algo</th>
                        <th style={{ textAlign: 'right', padding: '2px', color: 'var(--text-secondary)' }}>Cost</th>
                        <th style={{ textAlign: 'right', padding: '2px', color: 'var(--text-secondary)' }}>Edges</th>
                        <th style={{ textAlign: 'right', padding: '2px', color: 'var(--text-secondary)' }}>Time</th>
                      </tr>
                    </thead>
                    <tbody>
                      {mstCompare.map(r => (
                        <tr key={r.algorithm}>
                          <td style={{ padding: '3px 2px', fontWeight: 600, color: '#f59e0b' }}>{r.algorithm}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right' }}>{r.totalCost}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right' }}>{r.edgeCount}</td>
                          <td style={{ padding: '3px 2px', textAlign: 'right', color: 'var(--event-color)' }}>{r.time_ms}ms</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}

              <h4 style={{ fontSize: 11, margin: '10px 0 6px', color: '#10b981', borderTop: '1px solid var(--panel-border)', paddingTop: 8 }}>
                MST Path vs Shortest Path
              </h4>
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

              <button className="btn" style={{ width: '100%', justifyContent: 'center', marginBottom: 8, fontSize: 11, borderColor: '#10b981', color: '#10b981' }}
                onClick={runMSTvsShortestPath} disabled={edges.length === 0 || !sourceNode || !destNode}>
                <BarChart3 size={13} /> Compare MST vs Shortest Path
              </button>

              {mstVsSpResult && (
                <div style={{ background: 'rgba(16,185,129,0.06)', border: '1px solid rgba(16,185,129,0.4)', borderRadius: 6, padding: 8 }}>
                  <div style={{ fontWeight: 700, color: '#10b981', marginBottom: 6, fontSize: 11 }}>
                    Cost-Optimized (MST) vs Shortest Path
                  </div>
                  <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: 10 }}>
                    <thead>
                      <tr style={{ borderBottom: '1px solid rgba(16,185,129,0.2)' }}>
                        <th style={{ textAlign: 'left', padding: '3px 2px', color: 'var(--text-secondary)' }}>Metric</th>
                        <th style={{ textAlign: 'right', padding: '3px 2px', color: '#3b82f6' }}>Shortest</th>
                        <th style={{ textAlign: 'right', padding: '3px 2px', color: '#f59e0b' }}>MST Path</th>
                      </tr>
                    </thead>
                    <tbody>
                      <tr style={{ borderBottom: '1px solid rgba(255,255,255,.03)' }}>
                        <td style={{ padding: '3px 2px', color: 'var(--text-secondary)' }}>Cost</td>
                        <td style={{ padding: '3px 2px', textAlign: 'right', fontWeight: 700, color: '#3b82f6' }}>{mstVsSpResult.shortest.cost}</td>
                        <td style={{ padding: '3px 2px', textAlign: 'right', fontWeight: 700, color: '#f59e0b' }}>{mstVsSpResult.mst.cost}</td>
                      </tr>
                      <tr style={{ borderBottom: '1px solid rgba(255,255,255,.03)' }}>
                        <td style={{ padding: '3px 2px', color: 'var(--text-secondary)' }}>Hops</td>
                        <td style={{ padding: '3px 2px', textAlign: 'right', color: '#3b82f6' }}>{mstVsSpResult.shortest.hops}</td>
                        <td style={{ padding: '3px 2px', textAlign: 'right', color: '#f59e0b' }}>{mstVsSpResult.mst.hops}</td>
                      </tr>
                      <tr style={{ borderBottom: '1px solid rgba(255,255,255,.03)' }}>
                        <td style={{ padding: '3px 2px', color: 'var(--text-secondary)' }}>Compute Time</td>
                        <td style={{ padding: '3px 2px', textAlign: 'right', color: '#3b82f6' }}>{mstVsSpResult.shortest.time_ms}ms</td>
                        <td style={{ padding: '3px 2px', textAlign: 'right', color: '#f59e0b' }}>{mstVsSpResult.mst.time_ms}ms</td>
                      </tr>
                      <tr>
                        <td style={{ padding: '3px 2px', color: 'var(--text-secondary)' }}>Path Stretch</td>
                        <td colSpan={2} style={{ padding: '3px 2px', textAlign: 'right', fontWeight: 700, color: '#10b981', fontSize: 12 }}>{mstVsSpResult.pathStretch}x</td>
                      </tr>
                    </tbody>
                  </table>
                  <div style={{ fontSize: 9, color: 'var(--text-secondary)', marginTop: 6, lineHeight: 1.4 }}>
                    <div>Shortest: {mstVsSpResult.shortest.path}</div>
                    <div>MST Path: {mstVsSpResult.mst.path}</div>
                    <div style={{ marginTop: 3 }}>Total MST Network Cost: <b style={{ color: '#f59e0b' }}>{mstVsSpResult.mst.totalMstCost}</b></div>
                  </div>
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