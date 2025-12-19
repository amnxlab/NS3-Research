# UWSN Protocol Comparison Framework

A comprehensive simulation framework for comparing Underwater Wireless Sensor Network (UWSN) routing protocols using ns-3 and Aqua-Sim-NG.

## Table of Contents

- [Features](#features)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Simulation Parameters](#simulation-parameters)
- [Running Experiments](#running-experiments)
- [Analyzing Results](#analyzing-results)
- [Generating Graphs](#generating-graphs)
- [Output Files](#output-files)
- [Troubleshooting](#troubleshooting)

---

## Features

### Supported Protocols
- **VBF** (Vector-Based Forwarding) - Geographic routing using virtual pipeline
- **DBR** (Depth-Based Routing) - Pressure/depth-based greedy forwarding
- **Flooding** - Simple broadcast flooding for baseline comparison

### Metrics Collected
| Metric | Description |
|--------|-------------|
| **PDR** | Packet Delivery Ratio (received/sent) |
| **Throughput** | Network throughput in bits per second |
| **End-to-End Delay** | Average, min, max, std deviation (ms) |
| **Network Lifetime** | Time until first node energy depletion |
| **Energy Consumption** | Total, per-node, residual energy (Joules) |
| **Energy Efficiency** | Packets delivered per Joule |

### Traffic Generation
- **CBR (Constant Bit Rate)** traffic with configurable data rate
- Configurable packet size
- Multiple source/sink topology support

### Automation & Analysis
- Python automation for running 30+ experiments with different seeds
- Statistical analysis with 95% confidence intervals
- Publication-quality graphs (bar, line, box plots, CDF, heatmaps)
- LaTeX table generation for papers

---

## Prerequisites

### System Requirements
- Ubuntu 22.04/24.04 (or compatible Linux)
- 8GB+ RAM recommended
- Python 3.10+

### Required Packages

```bash
# Core build tools
sudo apt update
sudo apt install -y \
  build-essential g++ gcc \
  cmake ninja-build git pkg-config \
  python3 python3-dev python3-pip \
  python3-setuptools python3-wheel

# ns-3 dependencies
sudo apt install -y \
  qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
  libgtk-3-dev libxml2 libxml2-dev \
  libsqlite3-dev libboost-all-dev \
  gsl-bin libgsl-dev libfftw3-dev \
  libopenmpi-dev openmpi-bin libpcap-dev tcpdump
```

---

## Installation

### 1. Clone/Setup the Repository

```bash
cd ~/NS3-Research
```

### 2. ns-3 with Aqua-Sim-NG is Already Installed

The framework includes:
- ns-3.42 in `ns-allinone-3.42/ns-3.42/`
- Aqua-Sim-NG module in `ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/`

### 3. Install Python Dependencies

```bash
cd ~/NS3-Research/scripts
pip install -r requirements.txt
```

### 4. Build the Simulation

```bash
cd ~/NS3-Research

# Make setup script executable
chmod +x setup.sh

# Run setup (copies source to ns-3 scratch and builds)
./setup.sh
```

Or manually:

```bash
# Copy simulation to scratch folder
cp ~/NS3-Research/src/uwsn-research.cc ~/ns3/ns-allinone-3.42/ns-3.42/scratch/

# Build ns-3
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 build
```

---

## Quick Start

### Run a Single Simulation

```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42

# Basic run with VBF protocol
./ns3 run "uwsn-research --protocol=VBF --simStop=100"

# With custom parameters
./ns3 run "uwsn-research --protocol=DBR --nodes=150 --dataRate=5000 --simStop=200"
```

### Run Full Experiment Suite

```bash
cd ~/NS3-Research/scripts

# Run 30 experiments per configuration (default)
python run_experiments.py

# Quick test with fewer runs
python run_experiments.py --runs 5 --simtime 100

# With specific protocols and node counts
python run_experiments.py --protocols VBF DBR --nodes 50 100 --runs 10
```

### Analyze and Plot Results

```bash
cd ~/NS3-Research/scripts

# Analyze results
python analyze_results.py ~/NS3-Research/results/uwsn_results_*.csv

# Generate graphs
python plot_results.py ~/NS3-Research/results/uwsn_results_*.csv --output-dir graphs/
```

---

## Simulation Parameters

### Command-Line Options for `uwsn-research`

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--protocol` | VBF | Routing protocol: VBF, DBR, flooding |
| `--nodes` | 100 | Number of intermediate relay nodes |
| `--sinks` | 1 | Number of sink nodes |
| `--simStop` | 1000 | Simulation duration (seconds) |
| `--runNumber` | 1 | Random seed for reproducibility |
| `--dataRate` | 10000 | CBR traffic rate (bits/second) |
| `--packetSize` | 40 | Packet payload size (bytes) |
| `--range` | 100 | Transmission range (meters) |
| `--areaX` | 400 | Simulation area X dimension (meters) |
| `--areaY` | 400 | Simulation area Y dimension (meters) |
| `--maxDepth` | 400 | Maximum water depth (meters) |
| `--csvFile` | uwsn-results.csv | Output CSV file path |

### Example Commands

```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42

# Compare protocols at different node densities
./ns3 run "uwsn-research --protocol=VBF --nodes=50 --simStop=500"
./ns3 run "uwsn-research --protocol=VBF --nodes=100 --simStop=500"
./ns3 run "uwsn-research --protocol=VBF --nodes=200 --simStop=500"

# Test different CBR data rates
./ns3 run "uwsn-research --protocol=VBF --dataRate=1000 --simStop=100"
./ns3 run "uwsn-research --protocol=VBF --dataRate=5000 --simStop=100"
./ns3 run "uwsn-research --protocol=VBF --dataRate=10000 --simStop=100"

# Vary transmission range
./ns3 run "uwsn-research --protocol=DBR --range=50 --simStop=200"
./ns3 run "uwsn-research --protocol=DBR --range=100 --simStop=200"
./ns3 run "uwsn-research --protocol=DBR --range=150 --simStop=200"
```

---

## Running Experiments

### Automation Script: `run_experiments.py`

```bash
cd ~/NS3-Research/scripts
python run_experiments.py [OPTIONS]
```

#### Options

| Option | Default | Description |
|--------|---------|-------------|
| `--runs` | 30 | Number of runs per configuration |
| `--protocols` | VBF DBR flooding | Protocols to test |
| `--nodes` | 50 100 150 200 | Node counts to test |
| `--data-rates` | 10000 | CBR data rates (bps) |
| `--packet-size` | 40 | Packet size (bytes) |
| `--simtime` | 1000 | Simulation time (seconds) |
| `--parallel` | 0 | Parallel workers (0=sequential) |
| `--skip-build` | False | Skip rebuild step |
| `--output` | auto | Output CSV file path |

#### Examples

```bash
# Full experiment suite (3 protocols × 4 node counts × 30 runs = 360 experiments)
python run_experiments.py --runs 30

# Quick test
python run_experiments.py --protocols VBF --nodes 100 --runs 5 --simtime 100

# Test multiple data rates
python run_experiments.py --data-rates 1000 5000 10000 20000 --runs 10

# Parallel execution (faster on multi-core systems)
python run_experiments.py --parallel 4 --runs 30

# Skip rebuild if already built
python run_experiments.py --skip-build --runs 30
```

---

## Analyzing Results

### Statistical Analysis: `analyze_results.py`

```bash
cd ~/NS3-Research/scripts
python analyze_results.py <results.csv> [OPTIONS]
```

#### Features
- Computes mean, standard deviation, 95% confidence intervals
- Groups by protocol and node count
- Performs Kruskal-Wallis and Mann-Whitney U statistical tests
- Generates LaTeX tables for papers

#### Options

| Option | Description |
|--------|-------------|
| `--output` | Output summary CSV file |
| `--latex` | Generate LaTeX tables |

#### Example

```bash
# Analyze and generate summary
python analyze_results.py ~/NS3-Research/results/uwsn_results_20241219.csv

# With LaTeX tables
python analyze_results.py ~/NS3-Research/results/uwsn_results_20241219.csv --latex
```

#### Output
- Summary statistics table
- Statistical significance tests
- `*_summary.csv` file with aggregated results
- `*_latex.tex` file (if `--latex` specified)

---

## Generating Graphs

### Visualization: `plot_results.py`

```bash
cd ~/NS3-Research/scripts
python plot_results.py <results.csv> [OPTIONS]
```

#### Generated Graphs

| Graph | File | Description |
|-------|------|-------------|
| Summary Dashboard | `summary_dashboard.pdf` | 4-panel overview (PDR, delay, energy, throughput) |
| PDR Bar Chart | `pdr_bar.pdf` | Protocol comparison with error bars |
| PDR Line Graph | `pdr_line.pdf` | PDR vs number of nodes |
| PDR Box Plot | `pdr_boxplot.pdf` | Distribution across runs |
| Delay Line Graph | `delay_line.pdf` | End-to-end delay vs nodes |
| Delay Box Plot | `delay_boxplot.pdf` | Delay distribution |
| Delay CDF | `delay_cdf.pdf` | Cumulative distribution function |
| Energy Breakdown | `energy_breakdown.pdf` | Total consumption & efficiency |
| Energy Line Graph | `energy_line.pdf` | Energy vs nodes |
| Network Lifetime | `network_lifetime.pdf` | Lifetime comparison |
| Throughput Line | `throughput_line.pdf` | Throughput vs nodes |
| PDR Heatmap | `pdr_heatmap.pdf` | Protocol × nodes matrix |

#### Options

| Option | Default | Description |
|--------|---------|-------------|
| `--output-dir` | graphs/ | Output directory |
| `--format` | pdf | Output format: pdf, png, svg |

#### Example

```bash
# Generate all graphs as PDF
python plot_results.py ~/NS3-Research/results/uwsn_results_20241219.csv

# Generate PNG for presentations
python plot_results.py ~/NS3-Research/results/uwsn_results_20241219.csv --format png

# Custom output directory
python plot_results.py ~/NS3-Research/results/uwsn_results_20241219.csv --output-dir ~/paper/figures/
```

---

## Output Files

### CSV Output Format

The simulation generates a CSV file with the following columns:

```
run,protocol,nodes,sinks,simStop,range,areaX,areaY,minDepth,maxDepth,
data_rate_bps,packet_size_bytes,packets_sent,packets_received,pdr,
throughput_bps,avg_delay_ms,min_delay_ms,max_delay_ms,std_delay_ms,
network_lifetime_s,dead_nodes,initial_energy_per_node,tx_power,rx_power,
idle_power,total_initial_energy,total_consumed_energy,total_residual_energy,
avg_energy_per_node,energy_efficiency_pkts_per_j
```

### Example Row

```csv
1,VBF,100,1,100,100,400,400,0,400,10000,40,310,233,0.75,745.6,0,0,0,0,100,0,500,2,0.1,0.001,51000,94.04,50906,0.92,2.48
```

---

## Directory Structure

```
~/NS3-Research/                     # Your git repository (clean)
├── src/
│   └── uwsn-research.cc            # Main simulation source
├── examples/
│   └── VBF.cc                      # Original VBF example (reference)
├── scripts/
│   ├── run_experiments.py          # Automation runner
│   ├── analyze_results.py          # Statistical analysis
│   ├── plot_results.py             # Graph generation
│   └── requirements.txt            # Python dependencies
├── results/                        # Experiment results (gitignored)
│   └── uwsn_results_*.csv
├── graphs/                         # Generated graphs (gitignored)
│   ├── summary_dashboard.pdf
│   ├── pdr_bar.pdf
│   └── ...
├── setup.sh                        # Setup script (copy & build)
├── .gitignore                      # Git ignore rules
└── README.md                       # This file

~/ns3/                              # ns-3 installation (separate)
└── ns-allinone-3.42/
    └── ns-3.42/
        ├── scratch/                # Simulation copied here by setup.sh
        │   └── uwsn-research.cc
        ├── contrib/
        │   └── aqua-sim-ng/        # Aqua-Sim-NG module
        └── build/                  # Build output
```

---

## Troubleshooting

### Build Errors

```bash
# Clean and rebuild
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 clean
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### Missing Aqua-Sim-NG

```bash
# Clone if missing
cd ~/ns3/ns-allinone-3.42/ns-3.42/contrib
git clone https://github.com/rmartin5/aqua-sim-ng.git

# Rebuild
cd ..
./ns3 clean
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

### Python Import Errors

```bash
cd ~/NS3-Research/scripts
pip install -r requirements.txt
```

### Simulation Not Found

```bash
# Use the setup script
cd ~/NS3-Research
./setup.sh

# Or manually copy to scratch
cp ~/NS3-Research/src/uwsn-research.cc ~/ns3/ns-allinone-3.42/ns-3.42/scratch/

# Rebuild
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 build
```

### Low PDR or No Packets Received

- Check transmission range vs area size
- Verify sink position is reachable
- For VBF, ensure `TargetPos` matches sink location
- Try increasing simulation time

---

## Citation

If you use this framework in your research, please cite:

```bibtex
@software{uwsn_comparison_framework,
  title = {UWSN Protocol Comparison Framework},
  author = {Ahmed Amin},
  year = {2025},
  url = {https://github.com/void0x11/NS3-Research}
}
```

---

## License

This project uses ns-3 (GPLv2) and Aqua-Sim-NG (GPLv2). See respective licenses for details.