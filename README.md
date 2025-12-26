# UWSN Protocol Comparison Framework V2

Underwater Wireless Sensor Network simulation framework for comparing VBF, DBR, and Flooding protocols using ns-3.42 and Aqua-Sim-NG.

---

## Quick Start

### Option 1: Web Dashboard (Recommended - Works Everywhere!)
```bash
cd ~/NS3-Research/scripts
./launch_dashboard.sh
```
Then open your browser to: **http://localhost:8000**

**Features:**
- 🎨 Professional Cyber-Terminal dark theme with glassmorphism
- ✅ All parameters in one interface with smart defaults
- 📊 Real-time console output streaming
- 💾 Parameter sweeps via comma-separated values
- 🌐 Works in any environment (no GUI display required)

### Option 2: Command Line
```bash
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt
python3 run_all.py --protocols VBF DBR flooding --nodes 100 --runs 15
```

**Output:** CSV results, console analysis, and publication-quality graphs automatically generated.

---

## Command Line Options

### Basic Usage

```bash
python3 run_all.py [OPTIONS]
```

### All Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--protocols` | list | VBF DBR flooding | Protocols to test (multiple supported) |
| `--nodes` | list | 100 | Node counts for sweep (e.g., `100 150 200`) |
| `--runs` | int | 15 | Runs per configuration (for statistical averaging) |
# 🌊 UWSN Simulation Dashboard & Analysis Framework

**A comprehensive GUI-based system for UnderWater Sensor Network (UWSN) simulation using NS-3.**
Compare **VBF**, **DBR**, and **Flooding** protocols with automated parameter sweeping, real-time visualization, and network lifetime analytics.

---

## 🚀 Quick Start

### 1. Start the Dashboard (Recommended)
The entire system is controlled via a unified web interface.
```bash
python3 scripts/start_dashboard.py
```
> Open **http://localhost:8000** in your browser.

### 2. Run a Simulation
1.  **Select Protocols**: Check VBF, DBR, or Flooding.
2.  **Configure Parameters**:
    *   **Sweep**: Click `[+]` to add multiple values for Nodes, Time, Range, or Energy.
    *   **Example**: Add Node counts `100` and `200` to run both scenarios automatically.
3.  **Click "Initiate Simulation"**.
4.  **View Results**:
    *   Watch the **Real-Time Console** for progress.
    *   Click **"📊 View Graphics Gallery"** to see generated plots instantly.

---

## 🌟 Key Features

### 🖥️ Cyber-Terminal Dashboard
*   **Detached Execution**: Simulations run in the background. Reloading the page **does not** kill the simulation. The dashboard auto-reconnects to the stream.
*   **Dynamic Sweeps**: Sweep **any** parameter (Nodes, Range, Sim Time, Energy, Dimensions) by adding multiple input fields. The system handles the combinatorial logic.
*   **Integrated Gallery**: Built-in modal to browse `alive_nodes_vs_time.png`, `network_lifetime.png`, and more without leaving the UI.

### 📊 Advanced Analytics
*   **Network Lifetime**: Tracks the exact moment first nodes die (`alive_nodes_vs_time.png`).
*   **PDR & Delay**: Automated calculation of Packet Delivery Ratio and End-to-End Delay.
*   **Energy Profiling**: Analysis of energy consumption per node.

---

## 🛠️ Deployment & Manual Usage

### Directory Structure
*   `scripts/public/`: Web frontend (HTML/JS).
*   `scripts/api_server.py`: Backend Process Manager (Handles execution & streaming).
*   `scripts/run_all.py`: Core simulation driver (can be run manually).
*   `scripts/results/`: Raw CSV data.
*   `scripts/graphs/`: Generated Plots.

### Manual CLI Execution
If you prefer the terminal, you can bypass the dashboard:
```bash
# Run VBF and DBR with specific parameters
python3 scripts/run_all.py --protocols VBF DBR --nodes 100 200 --simtime 500 --initial-energy 100

# Full Options
python3 scripts/run_all.py --help
```

---

## ⚙️ Configuration Reference

| Parameter | Flag | Description | Sweepable? |
| :--- | :--- | :--- | :--- |
| **Nodes** | `--nodes` | Number of nodes in the network | ✅ Yes |
| **Sim Time** | `--simtime` | Duration of simulation (seconds) | ✅ Yes |
| **Energy** | `--initial-energy` | Initial battery level (Joules) | ✅ Yes |
| **Range** | `--range` | Communication range (meters) | ✅ Yes |
| **Mobility** | `--mobility` | `random` (RandomWalk) or `fixed` | ❌ No |
| **Dimensions**| `--area-x`, `--area-y` | Network Area Size | ✅ Yes |

---

## 📈 Metric Definitions

1.  **PDR (Packet Delivery Ratio)**: $\frac{\text{Received Packets}}{\text{Generated Packets}}$
2.  **Average Delay**: Mean time for successful packet delivery.
3.  **Network Lifetime**: Time until the first node depletes its battery.
4.  **Energy Consumed**: Total energy used by all nodes / Total nodes.

---

*Verified for NS-3.42 on Linux.*

# Drop visible around 30000-50000s
python3 run_all.py --simtime 150000 --initial-energy 100

# Drop visible around 15000-25000s
python3 run_all.py --simtime 150000 --initial-energy 50

# Drop visible around 7500-12500s
python3 run_all.py --simtime 150000 --initial-energy 25
```

---

## Example Commands

### Protocol Comparison
```bash
python3 run_all.py --protocols VBF DBR flooding --nodes 100 --runs 15
```

### Scalability Study
```bash
python3 run_all.py --protocols VBF DBR --nodes 50 100 150 200 --runs 10
```

### VBF Width Optimization
```bash
python3 run_all.py --protocols VBF --vbf-width 50 100 150 --runs 20
```

### DBR Depth Analysis
```bash
python3 run_all.py --protocols DBR --min-z 0 --max-z 200 --runs 15
python3 run_all.py --protocols DBR --min-z 0 --max-z 400 --runs 15
```

### Mobility Impact
```bash
python3 run_all.py --protocols VBF DBR flooding --mobility random --runs 20
```

### Range vs Performance
```bash
python3 run_all.py --protocols VBF DBR --range 80 --runs 10
python3 run_all.py --protocols VBF DBR --range 150 --runs 10
```

### Complete Parameter Test
```bash
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 150 200 \
  --range 120 \
  --min-z 0 \
  --max-z 300 \
  --vbf-width 80 \
  --mobility random \
  --runs 15 \
  --simtime 500
```

---

## Output

### Real-time Display
```
[1/45] Running: VBF | Nodes: 100 | Run: 1 | Range: 100m | Mobility: static
  ✓ PDR: 85.3% (Sent: 1000, Recv: 853)
```

### Analysis Summary
```
VBF (n=15 runs)
----------------------------------------
  PDR: 85.23% ± 3.45%
  Total Energy: 1234.56 ± 45.67 J
  Residual Energy: 51265.44 J
  Total Packets: Sent=15000, Received=12785
```

### CSV File
```csv
run,protocol,nodes,sinks,simStop,range,minDepth,maxDepth,vbfWidth,mobility,
areaX,areaY,sender_sent,sink_recv,pdr,initial_energy_per_node,tx_power,
rx_power,idle_power,total_initial_energy,total_consumed_energy,total_residual_energy
```

### Graphs (PNG, 300 DPI)
- `v2_pdr_comparison.png` - PDR bar chart
- `v2_energy_comparison.png` - Energy consumption bar chart
- `v2_energy_distribution.png` - Box plot of energy distribution
- `v2_scalability_analysis.png` - Energy/residual vs node count (if multiple node counts)
- `alive_nodes_vs_time.png` - **Network Lifetime**: Number of alive nodes over time
  - **Full simulation timeline** displayed on X-axis
  - **Drop clearly visible** when nodes start dying
  - Shows flat line at max nodes if no deaths occur (default 500J energy)
  - Use lower `--initial-energy` to observe node death patterns


---

## Parameter Interaction Matrix

| Parameter | VBF | DBR | Flooding |
|-----------|-----|-----|----------|
| Nodes | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Range | ⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐ |
| Depth | ⭐⭐ | ⭐⭐⭐ CRITICAL | ⭐ |
| VBF Width | ⭐⭐⭐ CRITICAL | ❌ | ❌ |
| Mobility | ⭐⭐ | ⭐ | ⭐⭐ |

---

## Installation

### Prerequisites
- ns-3.4x (3.40-3.49) installed
- Aqua-Sim-NG module in ns-3 contrib folder
- Python 3.8+

### Python Dependencies
```bash
cd ~/NS3-Research/scripts
python3 -m pip install pandas numpy matplotlib seaborn scipy
```

Or:
```bash
python3 -m pip install -r requirements.txt
```

### Dynamic NS-3 Detection
The script automatically searches for ns-3.4x installations in:
- `~/ns3/`
- `~/`
- `/usr/local/ns3/`
- `/opt/ns3/`
- Parent directory of NS3-Research

Matches patterns: `ns-3.4x` or `ns-allinone-3.4x` (where x = 0-9).

If not found, prompts for manual path input.

---

## Project Structure

```
NS3-Research/
├── src/
│   └── uswn-research.cc/
│       └── uwsn-research_v2.cc       # Main simulation (5 parameters)
├── scripts/
│   ├── run_all.py                     # All-in-one script
│   ├── requirements.txt               # Python dependencies
│   └── graphs/                        # Generated plots
├── results/                           # CSV output files
├── models/                            # Local VBF/DBR copies (for reference)
└── README.md                          # This file
```

---

## Simulation Details

### Topology
- **Area:** 400m × 400m × depth_range(m)
- **Sink:** (190, 190, 0) - at surface
- **Sender:** (10, 10, 50) - 50m depth
- **Relay Nodes:** Random uniform distribution in 3D space

### Traffic
- **Type:** CBR (Constant Bit Rate)
- **Packet Size:** 50 bytes
- **Data Rate:** 1000 bps
- **Duration:** Configurable (default 500s)

### Energy Model
- **Initial:** 500J per node
- **TX Power:** 0.5W
- **RX Power:** 0.3W
- **Idle Power:** 0.01W

### Protocols
- **VBF:** Vector-Based Forwarding (pipeline-based)
- **DBR:** Depth-Based Routing (greedy depth forwarding)
- **Flooding:** Broadcast flooding

---

## Metrics Collected

| Metric | Description | CSV Column |
|--------|-------------|------------|
| **PDR** | Packet Delivery Ratio (%) | `pdr` |
| **Sent Packets** | Total sent by sender | `sender_sent` |
| **Received Packets** | Total received at sink | `sink_recv` |
| **Total Energy** | Total Consumed Energy (J) | `total_consumed_energy` |
| **Residual Energy** | Total Residual Energy (J) | `total_residual_energy` |
| **Efficiency** | Metric: `(E_useful / E_consumed) * 100` | `energy_efficiency_percent` |
| **Avg Energy** | Average Residual per Node (J) | `avg_residual_energy` (Computed) |

---

## Troubleshooting

### NS-3 Not Found
```bash
# Manually specify path when prompted, or ensure ns-3.4x is in:
~/ns3/ns-allinone-3.42/ns-3.42
```

### Build Errors
```bash
# Rebuild manually
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 clean
./ns3 configure --build-profile=release --enable-examples --enable-tests
./ns3 build
```

### Permission Denied
```bash
chmod +x ~/NS3-Research/scripts/run_all.py
```

### Low PDR (0%)
- Increase `--range` (e.g., 150)
- Increase `--nodes` (e.g., 200)
- Check connectivity in sparse networks

---

## V2 Changes from V1

| Feature | V1 | V2 |
|---------|----|----|
| **Parameters** | Hardcoded in C++ | 5 configurable from CLI |
| **Scripts** | 3 separate scripts | 1 all-in-one script |
| **NS-3 Path** | Manual setup | Auto-detection |
| **Documentation** | Multiple files | Single README |
| **PDR Display** | Analysis only | Real-time + analysis + plot |
| **Protocol Logic** | ❓ | ✅ Unchanged, clearly marked |

---

## Citation

If you use this framework, please cite:

```bibtex
@software{uwsn_v2_framework,
  title = {UWSN Protocol Comparison Framework V2},
  author = {Your Name},
  year = {2024},
  url = {https://github.com/void0x11/NS3-Research}
}
```

---

## License

MIT License - See LICENSE file for details.

---

## Quick Reference Card

```bash
# Minimum command
python3 run_all.py

# Common usage
python3 run_all.py --protocols VBF DBR --nodes 100 --runs 15

# All Parameters Test
python3 run_all.py \
  --nodes 100 150 \
  --range 120 150 \
  --min-z 0 \
  --max-z 300 500 \
  --area-x 500 1000 \
  --area-y 500 1000 \
  --vbf-width 80 100 \
  --mobility fixed random \
  --protocols VBF DBR flooding \
  --runs 5
```

**Done! Results in `results/`, graphs in `scripts/graphs/`**
