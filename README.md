# UWSN Protocol Comparison Framework

Underwater Wireless Sensor Network (UWSN) simulation framework for comparing routing protocols using ns-3 and Aqua-Sim-NG.

## Features

- **Protocol Comparison**: VBF, DBR, and Flooding routing protocols
- **Traffic Modes**: OnOff (original) or CBR (constant bit rate)
- **Metrics**: PDR, Energy consumption, Packet statistics
- **Automation**: Python scripts for batch experiments
- **Analysis**: Statistical analysis with confidence intervals
- **Visualization**: Publication-quality graphs

## Directory Structure

```
~/NS3-Research/                 # This repository
├── src/
│   └── uwsn-research.cc        # Main simulation
├── scripts/
│   ├── run_experiments.py      # Batch runner
│   ├── analyze_results.py      # Statistics
│   ├── plot_results.py         # Graphs
│   └── requirements.txt        # Python dependencies
├── results/                    # CSV output (auto-created)
├── graphs/                     # PNG graphs (auto-created)
├── examples/
│   └── VBF.cc                  # Reference example
└── docs/
    └── Implementation_Summary.md

~/ns3/                          # ns-3 installation (separate)
└── ns-allinone-3.42/
    └── ns-3.42/
        ├── scratch/            # Simulation copied here
        └── contrib/
            └── aqua-sim-ng/    # Underwater module
```

## Quick Start

### 1. Setup

```bash
# Copy simulation to ns-3
cp ~/NS3-Research/src/uwsn-research.cc ~/ns3/ns-allinone-3.42/ns-3.42/scratch/

# Build
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 build
```

### 2. Run Single Simulation

```bash
# OnOff traffic (original)
./ns3 run "uwsn-research --protocol=VBF --simStop=500"

# CBR traffic
./ns3 run "uwsn-research --protocol=VBF --simStop=500 --cbr=true"

# Different protocol
./ns3 run "uwsn-research --protocol=DBR --simStop=500"
./ns3 run "uwsn-research --protocol=flooding --simStop=500"
```

### 3. Run Batch Experiments

```bash
cd ~/NS3-Research/scripts

# Install Python dependencies
pip install -r requirements.txt

# Run experiments (OnOff mode)
python3 run_experiments.py --protocols VBF DBR flooding --nodes 100 --runs 15

# Run experiments (CBR mode)
python3 run_experiments.py --protocols VBF DBR flooding --nodes 100 --runs 15 --cbr

# Analyze results
python3 analyze_results.py ../results/uwsn_results_*.csv

# Generate graphs
python3 plot_results.py ../results/uwsn_results_*.csv --format png
```

## Command Line Options

### Simulation (uwsn-research)

| Option | Default | Description |
|--------|---------|-------------|
| `--protocol` | VBF | Routing protocol: VBF, DBR, flooding |
| `--nodes` | 100 | Number of intermediate nodes |
| `--simStop` | 1000 | Simulation duration (seconds) |
| `--range` | 100 | Transmission range (meters) |
| `--dataRate` | 10000 | Data rate (bps) |
| `--packetSize` | 40 | Packet size (bytes) |
| `--cbr` | false | Use CBR traffic (true/false) |
| `--runNumber` | 1 | Random seed |
| `--csvFile` | uwsn-results.csv | Output file |

### Experiment Runner (run_experiments.py)

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | VBF DBR flooding | Protocols to test |
| `--nodes` | 50 100 150 200 | Node counts |
| `--runs` | 30 | Runs per configuration |
| `--simtime` | 1000 | Simulation time |
| `--cbr` | (flag) | Use CBR traffic |
| `--debug` | (flag) | Show debug output |

## Output Format

### CSV Columns

```
run, protocol, nodes, simStop, range, traffic_mode, data_rate, packet_size,
sender_sent, sink_recv, pdr,
total_initial_energy, total_consumed_energy, avg_energy_per_node
```

### Console Output

```
=== UWSN PROTOCOL COMPARISON ===
Protocol: VBF | Run: 1
...
--- End-to-End ---
Sender Sent: 202
Sink Received: 202
PDR: 100%

=== RESULTS ===
PDR: 100%
Energy Consumed: 405.2 J
Avg Energy/Node: 3.97 J
```

## Graphs Generated

| Graph | Description |
|-------|-------------|
| `pdr_bar.png` | PDR comparison by protocol |
| `pdr_line.png` | PDR vs node count |
| `pdr_boxplot.png` | PDR distribution |
| `energy_line.png` | Energy vs node count |
| `energy_boxplot.png` | Energy distribution |
| `network_lifetime.png` | Lifetime comparison |
| `dead_nodes.png` | Node failures |
| `total_energy.png` | Total energy consumption |
| `sent_vs_received.png` | Packet delivery scatter |
| `summary_dashboard.png` | 4-panel summary |

## Topology

```
Area: 200m × 200m × 150m (depth)
Range: 100m

     Surface (z=0)
     ┌─────────────────────┐
     │         ⬤ Sink      │  (100, 100, 0)
     │      (center)       │
     │                     │
     │    ○ ○ ○ ○ ○ ○     │  Relay nodes
     │   ○ ○ ○ ○ ○ ○ ○    │  (random positions)
     │    ○ ○ ○ ○ ○ ○     │
     │                     │
     │      ◆ Sender       │  (50, 50, 120)
     └─────────────────────┘
     Bottom (z=150)
```

## Protocols

| Protocol | Description | Best For |
|----------|-------------|----------|
| **VBF** | Vector-Based Forwarding - uses virtual pipeline | Dense networks |
| **DBR** | Depth-Based Routing - forwards toward surface | Simple deployment |
| **Flooding** | Broadcast to all neighbors | Baseline comparison |

## Requirements

- ns-3.42
- Aqua-Sim-NG module
- Python 3.8+
- pandas, matplotlib, seaborn, scipy

## Troubleshooting

### Build fails
```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42
rm -rf cmake-cache build
./ns3 configure --enable-examples
./ns3 build
```

### Low PDR
- Increase `--range` (e.g., 150)
- Decrease area size in source code
- Increase `--nodes` for denser deployment

### Simulation hangs
- Reduce `--simStop` time
- Check energy parameters (nodes may run out)

## License

Research use only. Based on Aqua-Sim-NG examples.
