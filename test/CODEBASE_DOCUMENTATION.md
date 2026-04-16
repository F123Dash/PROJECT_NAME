# Network Simulator - Complete Codebase Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture & Design Philosophy](#architecture--design-philosophy)
3. [Directory Structure](#directory-structure)
4. [Core Components](#core-components)
5. [Data Models](#data-models)
6. [Network Stack](#network-stack)
7. [Simulation Engine](#simulation-engine)
8. [Algorithms](#algorithms)
9. [Metrics Collection](#metrics-collection)
10. [Configuration System](#configuration-system)
11. [Data Flow & Simulation Lifecycle](#data-flow--simulation-lifecycle)
12. [Building & Running](#building--running)

---

## Project Overview

This is a **Discrete Event Network Simulator** inspired by NS-3, designed to simulate network behavior and performance. It combines:

- **Graph algorithms** (Dijkstra, MST, Union-Find) for topology analysis
- **Discrete event simulation engine** for timing and event scheduling
- **Network protocol stack** (TCP/UDP) for transport layer functionality
- **Performance metrics collection** for analyzing network behavior
- **Linux-inspired networking concepts** for realistic simulation

### Key Features
- Support for multiple protocols (TCP, UDP, custom)
- Traffic generation with configurable flows
- Packet loss and link failure simulation
- Real-time visualization support
- Comprehensive performance metrics
- Configurable topology and parameters

---

## Architecture & Design Philosophy

```
┌─────────────────────────────────────────┐
│    Graph Simulator (DSA - Algorithms)   │
├─────────────────────────────────────────┤
│   Discrete Event Engine (NS-3 style)    │
├─────────────────────────────────────────┤
│      Protocol Stack (Networking)        │
├─────────────────────────────────────────┤
│   Linux-inspired Behavior & Policies    │
├─────────────────────────────────────────┤
│  Visualization/Analysis (OMNeT++ style) │
└─────────────────────────────────────────┘
```

### Design Principles

1. **Layered Architecture**: Separation of concerns into distinct layers
2. **Event-Driven**: Everything is driven by events in a priority queue
3. **Modular Components**: Independent modules for easy testing and extension
4. **Metrics-First**: Comprehensive tracking integrated throughout
5. **Configuration-Driven**: External config files for easy parameter tuning

---

## Directory Structure

```
PROJECT_NAME/
├── algorithms/          # Core algorithms (Dijkstra, MST, Union-Find, etc.)
│   ├── shortest_path.*  # Dijkstra & Bellman-Ford
│   ├── mst.*            # Prim & Kruskal algorithms
│   ├── routing_table.*  # Build routing tables from paths
│   ├── metrics.*        # Path metrics computation
│   └── union_find.*     # Union-Find data structure
├── engine/              # Discrete event simulation core
│   ├── simulator.*      # Main simulator class
│   ├── event_queue.*    # Event scheduling & processing
│   ├── node.*           # Node abstraction
│   ├── packet.*         # Packet data structure
│   ├── integration.*    # System initialization
│   ├── config.*         # Configuration management
│   ├── pipeline.*       # Processing pipeline
│   └── main*.cpp        # Entry points
├── network/             # Network layer & protocols
│   ├── network_layer.*  # IP routing & forwarding
│   ├── tcp.*            # TCP protocol
│   ├── udp.*            # UDP protocol
│   ├── queue.*          # Packet queuing
│   ├── traffic_generator.* # Flow generation
│   ├── metrics.*        # Performance metrics
│   ├── logger.*         # Logging facilities
│   ├── congestion.*     # Congestion control
│   ├── failure.*        # Link failure simulation
│   └── flow.h           # Flow definition
├── graphs/              # Graph data structures
│   ├── graphs.*         # Graph class
│   ├── graph_generator.*# Topology generation
│   ├── graph_analysis.* # Graph analysis tools
│   └── link.h           # Link properties
├── config/              # Configuration files
│   ├── config.txt       # Standard config
│   ├── config_fast.txt  # Fast mode
│   └── config_notrealtime.txt # Non-realtime mode
├── test/                # Unit tests
└── build/               # Compiled binaries
```

---

## Core Components

### 1. **Simulator** (`engine/simulator.h`)

The main orchestrator of the simulation.

```cpp
class Simulator {
    Config config;                      // Configuration parameters
    Graph* graph;                       // Network topology
    Integration* integration;           // System integration layer
    TrafficGenerator traffic;           // Traffic/flow generation
    std::map<int, Node*> node_map;     // All nodes in simulation
    double simulation_time;             // Current simulation time
    
    void load_config(const string& filename);
    void load_topology();
    void init_system();                 // Initialize all components
    void init_traffic();                // Setup traffic flows
    void run();                         // Execute simulation
    void finalize();                    // Cleanup & reporting
};
```

### 2. **Node** (`engine/node.h`)

Represents a network node (router/host).

```cpp
struct Node {
    int id;                             // Unique identifier
    vector<int> routing_table;          // destination → next hop
    queue<Packet> pkt_queue;            // Packet buffer
    vector<int> interfaces;             // Connected links
    NetworkLayer* network_layer;        // Routing logic
    Transport* transport;               // TCP/UDP handling
    Graph* graph;                       // Reference to topology
    
    int get_next_hop(int destination);
    void send_packet(Packet& pkt);
    void receive_packet(Packet& pkt);
};
```

### 3. **Packet** (`engine/packet.h`)

Represents a data packet in the network.

```cpp
struct Packet {
    int packet_id;                      // Unique ID for tracking
    int source;                         // Source node
    int destination;                    // Destination node
    int size;                           // Size in bytes
    string protocol;                    // "TCP", "UDP", etc.
    int timestamp;                      // Creation time
    int ttl;                            // Time to live (hops)
    vector<int> path_history;           // Nodes visited
    
    bool is_expired() const;
    void record_hop(int node_id);
};
```

### 4. **Event** (`engine/event.h`)

Represents a discrete event in the simulation.

```cpp
struct Event {
    double time;                        // When event occurs
    string type;                        // Event type identifier
    function<void()> callback;          // Function to execute
    
    // Priority queue ordering (min-heap by time)
    bool operator>(const Event& other) const;
};
```

### 5. **Graph** (`graphs/graphs.h`)

Represents the network topology.

```cpp
class Graph {
    int V;                              // Number of vertices
    vector<vector<pair<int, int>>> adj; // Adjacency list with weights
    map<pair<int,int>, Link> links;     // Link properties
    
    void add_edge(int u, int v, int w);
    vector<pair<int,int>> get_neighbors(int u) const;
    Link* getLinkProperties(int u, int v);
};
```

### 6. **Link** (`graphs/link.h`)

Represents link properties between nodes.

```cpp
struct Link {
    double bandwidth;                   // bps
    double latency;                     // ms
    double packet_loss_probability;     // 0.0-1.0
    int queue_capacity;                 // max packets
};
```

---

## Data Models

### Packet Lifecycle

```
1. Creation        → Packet created with source/dest
2. Queuing         → Enqueued at source node
3. Transmission    → Awaiting transmission slot
4. In-Flight       → Traveling on link
5. Reception       → Received at intermediate node
6. Routing/Forwarding → Lookup next hop
7. Repeat 3-6      → Until destination reached
8. Delivery        → Received at destination
9. Metrics Recording → Latency, hops, etc. recorded
```

### Flow Definition

A **Flow** represents a stream of packets between two nodes with specified characteristics:
- Source and destination nodes
- Packet size
- Interval between packets
- Duration/number of packets
- Protocol type (TCP/UDP, ICMP)

---

## Network Stack

The simulator implements a simplified OSI model:

### Layer 4: Transport Layer (`network/tcp.h`, `network/udp.h`)

**Transport** is the abstract base class for protocols.

- **TCP**: Connection-oriented, reliable, ordered delivery
- **UDP**: Connectionless, unreliable, datagram-based

### Layer 3: Network Layer (`network/network_layer.h`)

Handles **routing and forwarding** of packets.

```cpp
class NetworkLayer {
    Node* node;
    
    void send(Packet p);                // Send from this node
    void receive(Packet p);             // Process received packet
    void schedulePacketArrival(Packet p, int next_hop, double travel_time);
    void forward(Packet p);             // Core forwarding logic
};
```

### Layer 2 & 1: Link Layer & Physical

- **Link properties**: Bandwidth, latency, loss probability
- **Queue management**: FIFO queuing with capacity limits
- **Congestion**: Simulated drop on buffer overflow

---

## Simulation Engine

### Event Queue Architecture

The **Event Queue** is the heart of the simulator.

```cpp
// Global variables
extern double current_time;             // Current simulation time
extern std::vector<Event> event_queue;  // Priority queue (min-heap)

// Functions
void schedule_event(Event e);           // Add event
void run_event_loop(double end_time);   // Execute all events
```

### Real-Time Mode

The simulator supports real-time synchronization:

```cpp
extern bool REAL_TIME_MODE;             // Enable real-time
extern double TIME_SCALE;               // Simulation speed multiplier
extern int MAX_SLEEP_MS;                // Max sleep per iteration
```

### Simulator Clock (`engine/simulator_clock.h`)

Manages simulation timing with precision.

```cpp
class SimulatorClock {
    double current_time;
    void advance(double delta);
    double get_time() const;
};
```

---

## Algorithms

### 1. Shortest Path Algorithms

**File**: `algorithms/shortest_path.h/cpp`

#### Dijkstra's Algorithm
- **Time Complexity**: O((V+E) log V) with binary heap
- **Use Case**: Single-source shortest paths
- **Suitable for**: Weighted graphs with non-negative weights

```cpp
void dijkstra(Graph &g, int source, 
              vector<int> &dist, vector<int> &parent);
```

#### Bellman-Ford Algorithm
- **Time Complexity**: O(VE)
- **Use Case**: Handles negative weights
- **Suitable for**: More general graphs

```cpp
bool bellmanFord(Graph &g, int source,
                 vector<int> &dist, vector<int> &parent);
```

### 2. Minimum Spanning Tree (MST)

**File**: `algorithms/mst.h/cpp`

#### Kruskal's Algorithm
- **Approach**: Greedy, edge-based
- **Uses**: Union-Find for cycle detection
- **Time Complexity**: O(E log E)

```cpp
vector<Edge> kruskal(int n, vector<Edge>& edges);
```

#### Prim's Algorithm
- **Approach**: Greedy, vertex-based
- **Time Complexity**: O((V+E) log V)

```cpp
vector<Edge> prim(Graph& g);
```

### 3. Union-Find Data Structure

**File**: `algorithms/union_find.h/cpp`

Efficient disjoint set operations with path compression and union by rank.

```cpp
class UnionFind {
    void unite(int a, int b);
    int find(int a);
    bool connected(int a, int b);
};
```

### 4. Routing Table Generation

**File**: `algorithms/routing_table.h/cpp`

Builds routing tables from shortest path trees.

```cpp
vector<int> buildRoutingTable(const vector<int>& parent,
                              const vector<int>& dist,
                              int source, int n);
```

---

## Metrics Collection

### MetricsManager (Singleton Pattern)

**File**: `network/metrics.h/cpp`

Tracks comprehensive performance metrics throughout simulation.

#### Per-Packet Metrics

```cpp
struct PerPacketMetrics {
    int packet_id;
    int source, destination;
    int size;
    string protocol;
    double send_time;
    double receive_time;
    double latency;
    int hop_count;
    bool dropped;
    vector<int> path;
};
```

#### Collected Metrics

| Metric | Definition | Usage |
|--------|-----------|-------|
| **Latency** | `arrival_time - send_time` | End-to-end delay |
| **Throughput** | `total_bits_received / time` | Network capacity |
| **Packet Loss Rate** | `dropped_packets / sent_packets` | Reliability |
| **Hop Count** | Number of hops | Path efficiency |
| **Bandwidth Utilization** | `used / max_bandwidth` | Link efficiency |
| **Jitter** | Variance in packet delay | QoS indicator |
| **Queue Metrics** | Queue length, wait time | Buffer analysis |

#### Hook Points

The MetricsManager is called at 5 key locations:

1. **Packet Creation**: Record initial state and timestamp
2. **Packet Sent**: Log departure from node
3. **Packet Received**: Log arrival at node
4. **Packet Dropped**: Record drop reason
5. **Simulation End**: Generate final statistics

#### CSV Export

```cpp
MetricsManager::getInstance()->initialize(0, "results.csv");
// ... simulation runs ...
MetricsManager::getInstance()->finalize(simulation_end_time);
```

---

## Configuration System

### Config Files

Configuration is loaded from text files with key-value pairs.

**File**: `engine/config.h/cpp`

```cpp
class Config {
    unordered_map<string, string> data;
    
    int get_int(const string& key);
    double get_double(const string& key);
    string get_string(const string& key);
    bool get_bool(const string& key);
};
```

### Example Configuration (`config/config.txt`)

```
# Network Parameters
num_nodes=10
num_edges=25
topology_type=random

# Simulation Parameters
simulation_time=1000
traffic_flows=5
packet_size=1024

# Protocol Settings
default_protocol=TCP
enable_real_time=false
time_scale=1.0

# Link Parameters
default_bandwidth=1000
default_latency=10
packet_loss_rate=0.01

# Routing
routing_algorithm=dijkstra
enable_multipath=false

# Logging
log_level=INFO
metrics_output=results.csv
```

### Loading Configuration

```cpp
Simulator sim;
sim.load_config("config/config.txt");
sim.load_topology();
sim.init_system();
sim.run();
sim.finalize();
```

---

## Data Flow & Simulation Lifecycle

### Initialization Flow

```
1. main.cpp
   ├─> Simulator::load_config(filename)
   ├─> Simulator::load_topology()
   │   ├─> Graph generation
   │   └─> Link properties setup
   ├─> Simulator::init_system()
   │   ├─> Integration::init_nodes()
   │   ├─> Integration::build_routing_tables()
   │   │   └─> Dijkstra for each node
   │   └─> Integration::attach_layers()
   │       ├─> Attach NetworkLayer to each node
   │       └─> Attach Transport layer (TCP/UDP)
   └─> Simulator::init_traffic()
       └─> TrafficGenerator::schedule_flows()
```

### Simulation Execution Flow

```
2. Simulator::run()
   └─> run_event_loop(end_time)
       ├─> While event_queue not empty:
       │   ├─> Pop minimum-time event
       │   ├─> Update current_time
       │   ├─> Execute event callback
       │   │   ├─> May schedule new events
       │   │   └─> May generate packets
       │   └─> Real-time sync (if enabled)
       └─> When queue empty or time exceeded
           └─> Jump to finalization
```

### Packet Transmission Flow

```
3. Packet Transmission
   
   Source Node:
   └─> App creates packet
       └─> Transport::send(pkt)
           └─> NetworkLayer::send(pkt)
               ├─> Get next_hop from routing table
               ├─> Schedule transmission start event
               └─> Schedule transmission end event
                   └─> Calculate travel_time = distance/bandwidth

   Intermediate Nodes:
   └─> Link transmission completes
       └─> NetworkLayer::receive(pkt)
           ├─> Record hop in packet
           ├─> Lookup next_hop
           └─> Schedule next transmission

   Destination Node:
   └─> Link transmission completes
       └─> NetworkLayer::receive(pkt)
           ├─> Recognize as destination
           ├─> Pass to Transport layer
           ├─> Record metrics
           └─> Deliver to application
```

### Routing Decision

```
4. Routing & Forwarding
   
   At each node:
   ├─> Get destination from packet
   ├─> Lookup routing_table[destination]
   ├─> if next_hop found:
   │   └─> Calculate travel_time on link
   │       ├─> distance = 1 (unit hop)
   │       ├─> bandwidth = link_properties[next_hop]
   │       └─> travel_time = size / bandwidth + latency
   └─> else (no route):
       └─> Drop packet (record drop)
```

### Finalization Flow

```
5. Simulator::finalize()
   ├─> MetricsManager::getInstance()->finalize(end_time)
   │   ├─> Aggregate statistics
   │   ├─> Write CSV results
   │   └─> Generate summary report
   ├─> Cleanup all nodes
   ├─> Cleanup graph
   └─> Print final statistics
```

---

## Building & Running

### Prerequisites
- C++11 or later compiler (g++, clang)
- Standard library support for <queue>, <map>, <vector>

### Build Commands

```bash
# Build everything
make

# Build specific target
make simulator
make main_integration

# Clean build artifacts
make clean
```

### Running Simulations

```bash
# Run with default config
./build/simulator

# Run with specific config
./build/simulator --config config/config_fast.txt

# Run integration tests
./build/main_integration

# Run specific protocol test
./build/test/transport_protocol_test
```

### Configuration Examples

**Fast Mode** (`config/config_fast.txt`):
```
simulation_time=100
time_scale=10.0
enable_real_time=false
```

**Non-Realtime Mode** (`config/config_notrealtime.txt`):
```
enable_real_time=false
time_scale=1.0
```

**Standard Mode** (`config/config.txt`):
```
enable_real_time=true
time_scale=1.0
```

---

## Integration Points & Extension Guide

### Adding a New Protocol

1. Create protocol class inheriting from `Transport`
2. Implement `send()` and `receive()` methods
3. Register in `Integration::attach_layers()`

```cpp
class CustomProtocol : public Transport {
    void send(Packet p) override;
    void receive(Packet p) override;
};
```

### Adding Custom Metrics

1. Extend `PerPacketMetrics` struct
2. Add tracking calls in `NetworkLayer`
3. Update CSV export in `MetricsManager`

### Adding Network Features

- **Link Failure**: Hook into simulator clock
- **Congestion**: Extend queue management in `NetworkLayer`
- **Retransmission**: Schedule events in `Transport` layer
- **QoS Policies**: Add priority queues in nodes

---

## Debugging & Troubleshooting

### Enable Debug Output

Most classes have `print_info()` methods:

```cpp
node->print_info();
packet.print_info();
```

### Logging

**File**: `network/logger.h/cpp`

```cpp
Logger::getInstance()->log(LogLevel::INFO, "Message");
Logger::getInstance()->log(LogLevel::DEBUG, "Packet: " + to_string(id));
Logger::getInstance()->log(LogLevel::ERROR, "Transmission failed");
```

### Metrics Analysis

Generate results:
```bash
./build/simulator > simulation.log
# Check results.csv for detailed packet metrics
```

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| No packets delivered | No routing path | Verify graph connectivity |
| High packet loss | Buffer overflow | Increase queue capacity |
| Incorrect latency | Wrong link properties | Check bandwidth/latency config |
| Routing loops | MST issues | Use Dijkstra instead |

---

## Performance Characteristics

### Complexity Analysis

- **Routing Setup**: O(V log V + E log V) - Dijkstra for each node
- **Event Processing**: O(n log n) where n = number of events
- **Per-Packet Operations**: O(log V) for routing table lookup
- **Metrics Recording**: O(1) per packet event

### Memory Usage

- **Graph Storage**: O(V + E)
- **Routing Tables**: O(V²) in worst case
- **Event Queue**: O(n_events)
- **Metrics**: O(n_packets)

### Optimization Tips

1. Use adjacency list for sparse graphs
2. Enable batch processing for large flows
3. Configure TIME_SCALE for faster simulation
4. Use non-realtime mode for batch processing
5. Reduce metric granularity for large simulations

---

## References & Further Reading

- Feature list: See `Features.txt`
- NS-3 Architecture: https://www.nsnam.org/docs/
- Graph Algorithms: CLRS "Introduction to Algorithms"
- Discrete Event Simulation: Banks et al.
- Linux Networking: Stevens & Fenner "Unix Network Programming"

---

## Summary

This network simulator provides a complete, extensible platform for:
- ✅ Analyzing network topologies and routing efficiency
- ✅ Simulating realistic packet flows and protocols
- ✅ Collecting comprehensive performance metrics
- ✅ Testing network algorithms and policies
- ✅ Visualizing network behavior

The modular architecture allows easy extension and customization for specific research or educational needs.

