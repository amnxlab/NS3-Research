# UWSN Protocol Comparison Framework

Underwater Wireless Sensor Network (UWSN) simulation framework for comparing routing protocols using ns-3 and Aqua-Sim-NG.

---

## Table of Contents

- [Features](#features)
- [Directory Structure](#directory-structure)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Usage](#usage)
- [Command Line Options](#command-line-options)
- [Output Format](#output-format)
- [Graphs Generated](#graphs-generated)
- [Topology](#topology)
- [Protocols](#protocols)
- [Requirements](#requirements)
- [Troubleshooting](#troubleshooting)

---

## Features

- **Protocol Comparison**: VBF, DBR, and Flooding routing protocols
- **Traffic Modes**: OnOff (original) or CBR (constant bit rate)
- **Metrics**: PDR, Energy consumption, Packet statistics
- **Automation**: Python scripts for batch experiments
- **Analysis**: Statistical analysis with confidence intervals
- **Visualization**: Publication-quality graphs

---

## Directory Structure

```
~/NS3-Research/                 # This repository
├── src/
│   └── uwsn-research.cc        # Main simulation
├── scripts/
│   ├── run_experiments.py      # Batch runner
│   ├── analyze_results.py      # Statistics
│   ├── plot_results.py         # Graphs
│   ├── test_setup.py           # Setup verification
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

---

## Installation

### Prerequisites

1. **ns-3.42** installed with Aqua-Sim-NG module
2. **Python 3.8+**

### Step 1: Install Python Dependencies

```bash
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt
```

### Step 2: Verify ns-3 Installation

The scripts will **auto-detect** your ns-3 installation (supports versions **3.40-3.49**).

Search locations:
- `~/ns3/ns-allinone-3.4x/ns-3.4x`
- `~/ns-allinone-3.4x/ns-3.4x`
- `~/ns3/ns-3.4x`
- `/usr/local/ns3/ns-3.4x`

To test detection:
```bash
cd ~/NS3-Research/scripts
python3 run_experiments.py --help
```

You should see:
```
✓ Auto-detected ns-3 version: ns-3.42
Using paths:
  Research Dir: /home/username/NS3-Research
  NS-3 Path: /home/username/ns3/ns-allinone-3.42/ns-3.42
  Source Dir: /home/username/NS3-Research/src
  Results Dir: /home/username/NS3-Research/results
```

### Step 3: Verify Installation

```bash
cd ~/NS3-Research/scripts
python3 test_setup.py
```

This checks:
- ✓ Python packages installed
- ✓ Path detection working
- ✓ ns-3 build system accessible

---

## Quick Start

### Run First Experiment

```bash
cd ~/NS3-Research/scripts

# Quick test (1 run, 50 seconds)
python3 run_experiments.py --protocols VBF --nodes 100 --runs 1 --simtime 50

# Full experiment (15 runs)
python3 run_experiments.py --protocols VBF DBR flooding --nodes 100 --runs 15
```

### Generate Analysis and Graphs

```bash
# Analyze results
python3 analyze_results.py ../results/uwsn_results_*.csv

# Generate graphs
python3 plot_results.py ../results/uwsn_results_*.csv --format png
```

---

## Usage

### Single Simulation (Manual)

```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42

# OnOff traffic (original)
./ns3 run "uwsn-research --protocol=VBF --simStop=500"

# CBR traffic
./ns3 run "uwsn-research --protocol=VBF --simStop=500 --cbr=true"

# Different protocol
./ns3 run "uwsn-research --protocol=DBR --simStop=500"
./ns3 run "uwsn-research --protocol=flooding --simStop=500"
```

### Batch Experiments (Automated)

```bash
cd ~/NS3-Research/scripts

# Compare protocols (OnOff mode)
python3 run_experiments.py --protocols VBF DBR flooding --nodes 100 --runs 15

# Compare protocols (CBR mode)
python3 run_experiments.py --protocols VBF DBR flooding --nodes 100 --runs 15 --cbr

# Scalability test
python3 run_experiments.py --protocols VBF --nodes 50 100 150 200 --runs 10

# Quick test
python3 run_experiments.py --protocols VBF --nodes 100 --runs 1 --simtime 50

# Debug mode
python3 run_experiments.py --protocols VBF --nodes 100 --runs 1 --debug
```

### Analysis and Visualization

```bash
cd ~/NS3-Research/scripts

# Generate statistics (mean, std, 95% CI)
python3 analyze_results.py ../results/uwsn_results_*.csv

# Generate all graphs
python3 plot_results.py ../results/uwsn_results_*.csv --format png

# Specify output directory
python3 plot_results.py ../results/uwsn_results_*.csv --format png --output-dir ../graphs
```

---

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
| `--data-rate` | 10000 | Data rate (bps) |
| `--packet-size` | 40 | Packet size (bytes) |
| `--range` | 100 | Transmission range (m) |
| `--cbr` | (flag) | Use CBR traffic |
| `--skip-build` | (flag) | Skip copying and building |
| `--output` | auto | Output CSV path |
| `--debug` | (flag) | Show debug output |

---

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

---

## Graphs Generated

**5 Meaningful, Publication-Quality Graphs:**

| # | Graph | Description |
|---|-------|-------------|
| 1 | `1_protocol_comparison.png` | **Most Important** - PDR, Energy, Throughput side-by-side |
| 2 | `2_scalability_analysis.png` | PDR and Energy vs Network Size |
| 3 | `3_reliability_analysis.png` | Consistency box plots (variance analysis) |
| 4 | `4_energy_pdr_tradeoff.png` | Scatter plot showing efficiency trade-offs |
| 5 | `5_comprehensive_summary.png` | Complete 2×2 dashboard with performance table |

**Why only 5 graphs?**
- Focused on meaningful insights
- No redundant metrics
- Publication-ready quality
- Easy to understand

---

## Topology

```
Area: 400m × 400m × 400m (depth)
Range: 100m

     Surface (z=0)
     ┌─────────────────────┐
     │         ⬤ Sink      │  (200, 200, 0)
     │      (center)       │
     │                     │
     │    ○ ○ ○ ○ ○ ○     │  Relay nodes
     │   ○ ○ ○ ○ ○ ○ ○    │  (random positions)
     │    ○ ○ ○ ○ ○ ○     │
     │                     │
     │      ◆ Sender       │  (100, 100, 300)
     └─────────────────────┘
     Bottom (z=400)
```

---

## Protocols

| Protocol | Description | Best For |
|----------|-------------|----------|
| **VBF** | Vector-Based Forwarding - uses virtual pipeline | Dense networks |
| **DBR** | Depth-Based Routing - forwards toward surface | Simple deployment |
| **Flooding** | Broadcast to all neighbors | Baseline comparison |

---

## Requirements

### System
- ns-3 versions **3.40 through 3.49** (auto-detected)
- Aqua-Sim-NG module (in ns-3 contrib folder)
- Python 3.8+

### Python Packages
Install with: `python3 -m pip install -r scripts/requirements.txt`
- pandas
- matplotlib
- seaborn
- scipy

**Note**: The framework automatically detects any ns-3.4x version (3.40-3.49) installed on your system.

---

## Troubleshooting

### ns-3 Not Found

If you see:
```
ERROR: Could not auto-detect ns-3 installation (versions 3.40-3.49).
Please enter the full path to your ns-3.4x directory
```

**Solution 1**: Enter your ns-3 path when prompted:
```
> /home/myuser/workspace/ns-allinone-3.42/ns-3.42
```

**Solution 2**: Manual configuration in `scripts/run_experiments.py`:
```python
# Add your custom path to search_bases list (around line 38)
search_bases = [
    Path("/your/custom/path"),  # Add this line
    Path.home() / "ns3",
    Path.home(),
    # ... rest of paths
]
```

**Supported versions**: ns-3.40, ns-3.41, ns-3.42, ns-3.43, ... ns-3.49

### Python Package Errors

```bash
# Use virtual environment
python3 -m venv venv
source venv/bin/activate
python3 -m pip install -r requirements.txt
```

### Build Errors

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

### Simulation Hangs

- Reduce `--simStop` time
- Check energy parameters (nodes may run out)

### View Results

```bash
# CSV files
ls -lh ~/NS3-Research/results/

# Graphs
ls -lh ~/NS3-Research/scripts/graphs/
```

---

## Example Workflow

```bash
# 1. Setup
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt
python3 test_setup.py

# 2. Run experiments
python3 run_experiments.py --protocols VBF DBR flooding --nodes 100 --runs 15

# 3. Analyze
python3 analyze_results.py ../results/uwsn_results_*.csv

# 4. Visualize
python3 plot_results.py ../results/uwsn_results_*.csv --format png

# 5. View graphs
ls -lh graphs/
```

---

## Portable Setup

To run on different machines:

1. **Clone repository:**
   ```bash
   git clone <repo-url> ~/NS3-Research
   ```

2. **Install dependencies:**
   ```bash
   cd ~/NS3-Research/scripts
   python3 -m pip install -r requirements.txt
   ```

3. **Run experiments:**
   ```bash
   python3 run_experiments.py --protocols VBF --nodes 100 --runs 5
   ```

The scripts will automatically find your ns-3 installation!

---

## Tips

- **Fast testing**: Use `--runs 1 --simtime 50` for quick tests
- **Full comparison**: Use `--runs 30` for publication results
- **CBR vs OnOff**: Add `--cbr` flag to compare traffic types
- **Scalability**: Test with `--nodes 50 100 150 200`
- **Debug**: Use `--debug` flag to see simulation output

---

## License

Research use only. Based on Aqua-Sim-NG examples.
