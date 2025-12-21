# UWSN Protocol Comparison Framework V2

Underwater Wireless Sensor Network simulation framework for comparing VBF, DBR, and Flooding protocols using ns-3.42 and Aqua-Sim-NG.

---

## Quick Start

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
| `--protocols` | list | VBF DBR flooding | Protocols to test |
| `--nodes` | list | 100 | Node counts (e.g., `100 150 200`) |
| `--runs` | int | 15 | Runs per configuration |
| `--simtime` | int | 500 | Simulation duration (seconds) |
| `--range` | float | 100 | Communication range (meters) |
| `--min-depth` | float | 0.0 | Minimum node depth (meters) |
| `--max-depth` | float | 400.0 | Maximum node depth (meters) |
| `--vbf-width` | float | 100.0 | VBF routing pipe radius (meters) |
| `--mobility` | choice | static | `static` or `random` (2D horizontal) |
| `--skip-build` | flag | false | Skip copying/building simulation |
| `--skip-analysis` | flag | false | Skip analysis step |
| `--skip-plots` | flag | false | Skip plot generation |
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

### 3. **Depth Variation** (`--min-depth`, `--max-depth`)
- **What:** Vertical range (Z-axis) for node placement
- **Range:** 0-1000 meters
- **Impact:**
  - **DBR: CRITICAL** - Routing decisions based entirely on depth
  - VBF: Affects 3D pipeline geometry
  - Area: 400x400 (X,Y) fixed

```bash
# Shallow deployment
python3 run_all.py --min-depth 0 --max-depth 100 --protocols DBR --runs 10

# Deep deployment
python3 run_all.py --min-depth 0 --max-depth 600 --protocols DBR --runs 10
```

---

### 4. **VBF Width** (`--vbf-width`)
- **What:** Radius of VBF's virtual routing pipeline
- **Range:** 50-200 meters
- **Applies to:** VBF only (ignored by DBR/Flooding)
- **Trade-off:**
  - Wider: Higher PDR, more overhead
  - Narrower: Lower PDR, energy efficient

```bash
python3 run_all.py --protocol VBF --vbf-width 50 --runs 10   # Narrow
python3 run_all.py --protocol VBF --vbf-width 150 --runs 10  # Wide
```

---

### 5. **Mobility** (`--mobility`)
- **What:** Node movement pattern
- **Options:**
  - `static`: Fixed positions (anchored sensors)
  - `random`: 2D horizontal drift (water currents, depth constant)
- **Impact:**
  - VBF: Nodes may drift in/out of pipeline
  - DBR: Robust (depth unchanged)
  - Flooding: Neighbor set changes

```bash
python3 run_all.py --mobility static --protocols VBF DBR --runs 15
python3 run_all.py --mobility random --protocols VBF DBR --runs 15
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
python3 run_all.py --protocol VBF --vbf-width 50 100 150 --runs 20
```

### DBR Depth Analysis
```bash
python3 run_all.py --protocol DBR --min-depth 0 --max-depth 200 --runs 15
python3 run_all.py --protocol DBR --min-depth 0 --max-depth 400 --runs 15
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
  --min-depth 0 \
  --max-depth 300 \
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
| **Total Energy** | Energy consumed (J) | `total_consumed_energy` |
| **Residual Energy** | Remaining energy (J) | `total_residual_energy` |
| **Initial Energy** | Starting energy (J) | `total_initial_energy` |

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

# All 5 parameters
python3 run_all.py \
  --nodes 100 \
  --range 120 \
  --min-depth 0 \
  --max-depth 300 \
  --vbf-width 80 \
  --mobility random \
  --protocols VBF DBR flooding \
  --runs 15
```

**Done! Results in `results/`, graphs in `scripts/graphs/`**
