# UWSN Research V2 - Complete Documentation

## Overview

UWSN Protocol Comparison Framework V2 adds **5 configurable parameters** to the simulation without modifying any protocol logic. All changes are clearly marked in the code.

---

## Quick Start

```bash
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt
python3 run_all.py --protocols VBF DBR flooding --nodes 100 --runs 15
```

**Output**: CSV results, console analysis, and publication-quality graphs automatically generated.

---

## The 5 Configurable Parameters

### 1. Number of Nodes (`--nodes`)

**What it controls**: The number of intermediate relay nodes in the network (between sender and sink)

**Default**: `100`

**Range**: 10 - 500 (recommended for UWSN)

**Impact on protocols**:
- **VBF**: More nodes inside the routing pipe → more forwarders → higher reliability
- **DBR**: More nodes at different depths → better depth-based routing options
- **Flooding**: More nodes → more broadcast duplicates → higher overhead

**Example**:
```bash
python3 run_all.py --nodes 50 100 150 --protocols VBF DBR --runs 10
```

---

### 2. Communication Range (`--range`)

**What it controls**: The transmission range of each node's radio (in meters)

**Default**: `100` meters

**Range**: 50 - 300 meters (realistic for underwater acoustic communication)

**How it works**:
- Sets the maximum distance a node can communicate
- Applied to **all nodes** (sender, relay, sink)
- Determines neighbor connectivity in the network

**Impact on protocols**:
- **VBF**: Uses range for delay calculation and pipeline participation
- **DBR**: Determines reachable neighbors for depth-based routing
- **Flooding**: Broadcast radius

**Example**:
```bash
python3 run_all.py --range 120 --protocols VBF --runs 10
```

---

### 3. Depth Variation (`--min-depth`, `--max-depth`)

**What it controls**: The vertical (Z-axis) range where relay nodes are randomly placed

**Default**: 
- `--min-depth`: `0.0` meters (surface)
- `--max-depth`: `400.0` meters (400m deep)

**Range**: 0 - 1000 meters (typical underwater sensor deployment depths)

**Impact on protocols**:
- **VBF**: Affects 3D pipeline geometry
- **DBR**: ⭐ **CRITICAL** - Routing decisions based entirely on depth
- **Flooding**: Affects vertical connectivity

**Example**:
```bash
# Shallow deployment
python3 run_all.py --min-depth 0 --max-depth 100 --protocols DBR --runs 10

# Deep deployment
python3 run_all.py --min-depth 0 --max-depth 600 --protocols DBR --runs 10
```

---

### 4. VBF Routing Pipe Radius (`--vbf-width`)

**What it controls**: The radius (width) of the virtual routing "pipeline" in VBF protocol

**Default**: `100` meters

**Range**: 50 - 200 meters (typical for UWSN)

**Applies to**: **VBF protocol ONLY** (ignored by DBR and Flooding)

**Trade-offs**:
- **Wider pipe**: More forwarders → higher reliability → better PDR
- **Narrower pipe**: Fewer forwarders → lower overhead → energy efficient

**Example**:
```bash
python3 run_all.py --protocol VBF --vbf-width 50 --runs 10   # Narrow pipe
python3 run_all.py --protocol VBF --vbf-width 150 --runs 10  # Wide pipe
```

---

### 5. Mobility Model (`--mobility`)

**What it controls**: Whether nodes move during simulation or stay stationary

**Default**: `static`

**Options**: 
- `static`: Nodes remain at fixed positions (anchored sensors)
- `random`: Nodes drift horizontally due to water currents

**How it works**:
- **Static Mode**: Nodes stay at initial random positions
- **Random Mode**: Nodes move **horizontally only** (X and Y axes), depth (Z) stays constant

**Impact on protocols**:
- **VBF**: Pipeline membership may change
- **DBR**: Robust (depth fixed, only horizontal position changes)
- **Flooding**: Neighbor set changes

**Example**:
```bash
python3 run_all.py --protocols VBF DBR flooding --mobility static --runs 15
python3 run_all.py --protocols VBF DBR flooding --mobility random --runs 15
```

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

## Usage Examples

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
├── docs/
│   └── V2_COMPLETE.md                 # This file
├── PDR_CALCULATION.md                 # PDR implementation details
└── README.md                          # Main project README
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

## Important Notes

### What Was Changed
- Added 5 command-line parameters
- All changes marked with `// ==== EDITED ====` comments
- Enhanced console output to show parameters
- Updated CSV format to include parameters
- Implemented unique packet UID tracking for accurate PDR

### What Was NOT Changed
- **No changes to VBF protocol logic** (in aqua-sim-ng library)
- **No changes to DBR protocol logic** (in aqua-sim-ng library)
- **No changes to Flooding protocol logic** (in aqua-sim-ng library)
- **No changes to energy model**
- **No changes to packet forwarding algorithms**

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
| **Documentation** | Multiple files | Consolidated |
| **PDR Display** | Analysis only | Real-time + analysis + plot |
| **PDR Accuracy** | Could exceed 100% | Guaranteed 0-100% (UID tracking) |
| **Protocol Logic** | ✅ Unchanged | ✅ Unchanged |

---

## Quick Reference

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

