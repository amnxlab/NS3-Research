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
| `--simtime` | int | 500 | Simulation duration (seconds) |
| `--range` | list | 100 | Communication range sweep (meters) |
| `--initial-energy` | float | 500 | Initial battery energy (Joules). **Lower values enable Network Lifetime graph truncation** |
| `--min-z` | list | 0.0 | Minimum node depth sweep (meters) [Alias: `--min-depth`] |
| `--max-z` | list | 400.0 | Maximum node depth sweep (meters) [Alias: `--max-depth`] |
| `--area-x` | list | 400.0 | Area Width (X-axis) sweep (meters) |
| `--area-y` | list | 400.0 | Area Length (Y-axis) sweep (meters) |
| `--vbf-width` | list | 100.0 | VBF routing pipe radius sweep (meters) |
| `--mobility` | list | random | Mobility models: `fixed` or `random` |
| `--skip-build` | flag | false | Skip copying/building C++ source |
| `--skip-analysis` | flag | false | Skip result analysis step |
| `--skip-plots` | flag | false | Skip graph generation |
| `--output` | path | auto | Custom CSV output path |



---

## The 5 Configurable Parameters

### 1. **Nodes** (`--nodes`)
- **What:** Number of relay nodes between sender and sink
- **Range:** 10-500
- **Impact:** More nodes = denser network = more routing options

```bash
python3 run_all.py --nodes 50 100 150 200 --protocols VBF --runs 10
```

---

### 2. **Communication Range** (`--range`)
- **What:** Transmission range of each node (meters)
- **Range:** 50-300
- **Impact:** 
  - VBF: Used in delay calculation and pipeline participation
  - DBR: Determines reachable neighbors for depth-based routing
  - Flooding: Broadcast radius

```bash
python3 run_all.py --range 120 --protocols VBF DBR --runs 10
```

---

### 3. **Depth Variation** (`--min-z`, `--max-z`)
- **What:** Vertical range (Z-axis) for node placement
- **Default:** 0 - 400 meters
- **Impact:**
  - **DBR: CRITICAL** - Routing decisions based entirely on depth
  - VBF: Affects 3D pipeline geometry

```bash
# Shallow deployment
python3 run_all.py --min-z 0 --max-z 100 --protocols DBR --runs 10

# Deep deployment
python3 run_all.py --min-z 0 --max-z 600 --protocols DBR --runs 10
```

---

### 4. **Area Dimensions** (`--area-x`, `--area-y`)
- **What:** Horizontal dimensions of the simulation field
- **Default:** 400x400 meters
- **Impact:**
  - Larger area = lower node density (if node count constant)
  - Affects connectivity and path length

```bash
python3 run_all.py --area-x 1000 --area-y 1000 --nodes 200
```

---

### 5. **VBF Width** (`--vbf-width`)
- **What:** Radius of VBF's virtual routing pipeline
- **Range:** 50-200 meters
- **Applies to:** VBF only (ignored by DBR/Flooding)
- **Trade-off:**
  - Wider: Higher PDR, more overhead
  - Narrower: Lower PDR, energy efficient

```bash
python3 run_all.py --protocols VBF --vbf-width 50 --runs 10   # Narrow
python3 run_all.py --protocols VBF --vbf-width 150 --runs 10  # Wide
```

---

### 6. **Mobility** (`--mobility`)
- **What:** Node movement pattern
- **Options:**
  - `fixed`: Fixed positions (renamed from `static`)
  - `random`: Random Walk 2D (Variable speed 1.0-5.0 m/s, dist 30m)
- **Impact:**
  - VBF: Nodes may drift in/out of## Quick Start

1.  **Launch the Dashboard** (Recommended)
    Use the included Python launcher for a one-click setup.
    ```bash
    python3 scripts/start_dashboard.py
    ```
    Open **http://localhost:8000** in your browser.

2.  **Command Line** (Manual)
    ```bash
    python3 scripts/run_all.py --protocols VBF DBR --nodes 100 200 --simtime 500
    ```

---

## 🖥️ Web Dashboard Features

The new cyber-terminal dashboard provides a complete control center for your simulations:

### 1. Unified Control Panel
*   **Protocol Selection**: Toggle VBF, DBR, and Flooding.
*   **Dynamic Parameter Sweeps**: Add multiple values for Nodes, Range, Dimensions, Time, and Energy using the `[+]` button. The system automatically runs all combinations.
*   **Real-Time Console**: View the unfiltered simulation output stream directly in the browser.

### 2. Persistent & Robust
*   **Detached Execution**: Simulations run in the background. You can close the tab or reload the page without killing the process.
*   **Auto-Reconnect**: On reload, the dashboard reconnects to the running simulation and restores the log history.

### 3. Visualization Gallery
*   Built-in **lightbox gallery** for viewing generated plots (`graphs/`).
*   Auto-updates as new simulations complete.
*   Supports `alive_nodes_vs_time.png`, `network_lifetime.png`, `pdr_vs_nodes.png`, etc.

### Notes
*   **Logs**: Simulates standard terminal output with identical formatting.
*   **Data**: Results are saved to `results/` (CSVs) and `scripts/graphs/` (Plots).

**Example Energy Values for Long Simulations:**
```bash
# All nodes survive (flat line at max)
python3 run_all.py --simtime 15000 --initial-energy 500

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
