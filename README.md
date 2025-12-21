# UWSN Protocol Comparison Framework - Version 2

Underwater Wireless Sensor Network (UWSN) simulation framework for comparing routing protocols using ns-3 and Aqua-Sim-NG.

**Version 2 Features:**
- 🎯 5 configurable parameters (nodes, range, depth, VBF width, mobility)
- 🚀 All-in-one script (experiments + analysis + plots)
- 📊 Publication-quality graphs
- 🔧 No protocol logic changed

---

## Quick Start

### 1. Install Python Dependencies

```bash
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt
```

### 2. Run Complete Workflow

```bash
# One command does everything!
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15
```

**Output:**
- ✅ Results saved to `results/uwsn_v2_results_TIMESTAMP.csv`
- ✅ Analysis printed to console
- ✅ Graphs saved to `scripts/graphs/`

---

## The 5 Configurable Parameters

### Overview

Version 2 allows you to control **5 critical parameters** from the command line without modifying any code. These parameters affect network topology, protocol behavior, and node mobility.

---

### Parameter 1: Number of Nodes (`--nodes`)

**What it controls:** The number of intermediate relay nodes in the network (between sender and sink)

**Default:** `100`

**Range:** 10 - 500 (recommended for UWSN)

**How it works:**
- Creates relay nodes randomly distributed in the 3D underwater space
- More nodes = denser network = more routing paths
- Fewer nodes = sparse network = may cause connectivity issues

**Impact on protocols:**
- **VBF**: More nodes inside the routing pipe → more forwarders → higher reliability
- **DBR**: More nodes at different depths → better depth-based routing options
- **Flooding**: More nodes → more broadcast duplicates → higher overhead

**Example:**
```bash
python3 run_all.py --nodes 50 100 150 --protocols VBF DBR --runs 10
```

**Use cases:**
- Scalability studies: Test how protocols perform as network size grows
- Density analysis: Compare sparse vs dense network performance
- Connectivity studies: Find minimum nodes needed for good PDR

---

### Parameter 2: Communication Range (`--range`)

**What it controls:** The transmission range of each node's radio (in meters)

**Default:** `100` meters

**Range:** 50 - 300 meters (realistic for underwater acoustic communication)

**How it works:**
- Sets the maximum distance a node can communicate
- Applied to **all nodes** (sender, relay, sink)
- Determines neighbor connectivity in the network

**Implementation:**
```cpp
// Line 200 in uwsn-research_v2.cc
dev->GetPhy()->SetTransRange(range);  // Applied to each device
```

**Impact on protocols:**
- **VBF**: 
  - Uses range for delay calculation: `delay = (p/width) + ((range - d*cos_theta)/range)`
  - Larger range → more nodes can participate in forwarding
  - Affects which nodes are "close enough" to the routing pipeline
  
- **DBR**: 
  - Uses range to determine reachable neighbors
  - Larger range → more depth-based routing options
  - Critical for multi-hop depth-based forwarding
  
- **Flooding**: 
  - Larger range → more neighbors receive broadcast
  - Directly affects network coverage

**Example:**
```bash
# Test range impact
python3 run_all.py --range 80 --protocols VBF --runs 10   # Short range
python3 run_all.py --range 150 --protocols VBF --runs 10  # Long range
```

**Use cases:**
- Energy-range trade-off studies
- Optimal range determination
- Coverage analysis

**Note:** Larger range = more energy per transmission (not modeled in current version)

---

### Parameter 3: Depth Variation (`--min-depth`, `--max-depth`)

**What it controls:** The vertical (Z-axis) range where relay nodes are randomly placed

**Default:** 
- `--min-depth`: `0.0` meters (surface)
- `--max-depth`: `400.0` meters (400m deep)

**Range:** 0 - 1000 meters (typical underwater sensor deployment depths)

**How it works:**
- Relay nodes are randomly distributed between min and max depth
- Sink is typically at surface (z=0)
- Sender can be at any depth (default: 50m)

**Implementation:**
```cpp
// Lines 146-149 in uwsn-research_v2.cc
std::uniform_real_distribution<> disZ(minDepth, maxDepth);
for (int i = 0; i < nodes; ++i) {
    position->Add(Vector(disX(gen), disY(gen), disZ(gen)));  // Z from depth range
}
```

**Impact on protocols:**
- **VBF**: 
  - Depth affects 3D pipeline geometry
  - Vertical spread impacts which nodes are "inside" the pipeline
  - Larger depth range → more 3D routing complexity
  
- **DBR**: ⭐ **CRITICAL for DBR**
  - DBR routing decisions are **based entirely on depth**
  - Nodes at shallower depth forward packets toward surface (sink)
  - Larger depth range → more depth "layers" → more routing hops
  - Formula: `holding_time = f(current_depth - previous_hop_depth)`
  
- **Flooding**: 
  - Affects vertical connectivity
  - Deeper range → more 3D spread → potentially more hops

**Example:**
```bash
# Shallow deployment (0-100m)
python3 run_all.py --min-depth 0 --max-depth 100 --protocols DBR --runs 10

# Deep deployment (0-600m)
python3 run_all.py --min-depth 0 --max-depth 600 --protocols DBR --runs 10

# Mid-water deployment (200-400m, no surface/bottom)
python3 run_all.py --min-depth 200 --max-depth 400 --protocols VBF --runs 10
```

**Use cases:**
- DBR depth-routing analysis (most important use case)
- Deployment depth optimization
- Pressure/depth effects simulation
- Stratified water layer studies

**Important for DBR:** Since DBR forwards based on depth, varying this parameter is essential for studying DBR's depth-based greedy forwarding mechanism.

---

### Parameter 4: VBF Routing Pipe Radius (`--vbf-width`)

**What it controls:** The radius (width) of the virtual routing "pipeline" in VBF protocol

**Default:** `100` meters

**Range:** 50 - 200 meters (typical for UWSN)

**Applies to:** **VBF protocol ONLY** (ignored by DBR and Flooding)

**How it works:**
- VBF creates a virtual "pipe" from sender to sink
- Only nodes within this pipe's radius participate in forwarding
- Width determines how many nodes can be forwarders

**Implementation:**
```cpp
// Lines 131-133 in uwsn-research_v2.cc
if (protocol == "VBF") {
  asHelper.SetRouting("ns3::AquaSimVBF", 
                      "Width", DoubleValue(vbfWidth),  // Sets m_width in VBF
                      "TargetPos", Vector3DValue(Vector(sinkX, sinkY, sinkZ)));
}
```

**VBF Internal Usage:**
```cpp
// In aqua-sim-routing-vbf.cc
double delay = (p/m_width) + ((range - d*cos_theta)/range);
// p = perpendicular distance from node to pipeline axis
// If p < m_width → node is inside pipe → can forward
// If p > m_width → node is outside pipe → drops packet
```

**Impact:**

| Width | Nodes Inside Pipe | PDR | Overhead | Best For |
|-------|------------------|-----|----------|----------|
| **Narrow (50m)** | Few | Lower | Low | Sparse networks, energy-constrained |
| **Medium (100m)** | Moderate | Good | Medium | Balanced performance |
| **Wide (200m)** | Many | Higher | High | Dense networks, reliability-critical |

**Trade-offs:**
- ✅ **Wider pipe**: More forwarders → higher reliability → better PDR
- ❌ **Wider pipe**: More duplicates → higher energy consumption → more collisions
- ✅ **Narrower pipe**: Fewer forwarders → lower overhead → energy efficient
- ❌ **Narrower pipe**: Fewer paths → lower reliability → may fail if nodes die

**Example:**
```bash
# Test VBF width impact (only affects VBF)
python3 run_all.py --protocol VBF --vbf-width 50 --runs 10   # Narrow pipe
python3 run_all.py --protocol VBF --vbf-width 100 --runs 10  # Medium pipe
python3 run_all.py --protocol VBF --vbf-width 150 --runs 10  # Wide pipe

# Width has NO effect on DBR or Flooding
python3 run_all.py --protocol DBR --vbf-width 200 --runs 10  # Width ignored
```

**Use cases:**
- VBF optimization studies
- Reliability vs overhead trade-off analysis
- Pipeline geometry research
- Void avoidance studies (narrow pipe may avoid voids)

**Important:** This is the **key parameter** for VBF protocol tuning!

---

### Parameter 5: Mobility Model (`--mobility`)

**What it controls:** Whether nodes move during simulation or stay stationary

**Default:** `static`

**Options:** 
- `static`: Nodes remain at fixed positions (anchored sensors)
- `random`: Nodes drift horizontally due to water currents

**Applies to:** All protocols (VBF, DBR, Flooding)

**How it works:**

**Static Mode:**
```cpp
mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
```
- Nodes stay at initial random positions
- Topology never changes
- Most common for anchored underwater sensors

**Random Mode:**
```cpp
mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue(Rectangle(0, areaX, 0, areaY)),  // X,Y only
    "Speed", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),  // 2 m/s
    "Distance", DoubleValue(50.0));  // Move 50m then change direction
```
- Nodes move **horizontally only** (X and Y axes)
- **Depth (Z) stays constant** - realistic for buoyant sensors
- Speed: 2 m/s (typical ocean current speed)
- Random walk pattern: move 50m, pick new random direction, repeat

**Why X,Y only (horizontal)?**
- Real underwater sensors float at fixed depth (buoyancy control)
- Ocean currents are primarily horizontal
- Vertical movement is rare (except AUVs)
- **DBR requires constant depth** for depth-based routing to work

**Impact on protocols:**

| Protocol | Static Behavior | Random (Mobile) Behavior |
|----------|----------------|-------------------------|
| **VBF** | Pipeline geometry fixed → consistent routing | Pipeline to sink stays valid (depth fixed) but nodes may drift in/out of pipe radius → PDR may vary |
| **DBR** | Depth hierarchy fixed → optimal routing | ✅ **Still works!** Depth fixed, only horizontal position changes → depth-based routing unaffected |
| **Flooding** | Neighbor set fixed → predictable overhead | Neighbors change → broadcast reaches different nodes → may improve/reduce coverage |

**Example:**
```bash
# Compare static vs mobile
python3 run_all.py --protocols VBF DBR flooding --mobility static --runs 15
python3 run_all.py --protocols VBF DBR flooding --mobility random --runs 15

# Test which protocol is most robust to mobility
python3 run_all.py --protocols VBF --mobility random --runs 20
python3 run_all.py --protocols DBR --mobility random --runs 20
```

**Use cases:**
- Mobility robustness studies
- Comparison of static vs dynamic topology performance
- Water current impact simulation
- Protocol adaptability analysis

**Expected Results:**
- **VBF**: May see PDR drop with mobility (nodes move out of pipeline)
- **DBR**: Should handle mobility well (depth-based routing still valid)
- **Flooding**: May see increased/decreased coverage depending on movement

---

## Parameter Interaction Matrix

How parameters affect each protocol:

| Parameter | VBF | DBR | Flooding |
|-----------|-----|-----|----------|
| **1. Nodes** | ⭐⭐⭐ Affects density in pipeline | ⭐⭐⭐ Affects depth-layer density | ⭐⭐⭐ Affects broadcast coverage |
| **2. Range** | ⭐⭐⭐ Used in delay calculation | ⭐⭐⭐ Determines reachable neighbors | ⭐⭐⭐ Determines broadcast radius |
| **3. Depth** | ⭐⭐ Affects 3D pipeline geometry | ⭐⭐⭐ **CRITICAL** - routing based on depth | ⭐ Affects vertical connectivity |
| **4. VBF Width** | ⭐⭐⭐ **CRITICAL** - defines routing pipe | ❌ Not used | ❌ Not used |
| **5. Mobility** | ⭐⭐ Pipeline membership may change | ⭐ Robust (depth fixed) | ⭐⭐ Neighbor set changes |

---

## Complete Parameter Example

Test all 5 parameters together:

```bash
cd ~/NS3-Research/scripts

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

# This will run: 3 protocols × 3 node counts × 15 runs = 135 experiments
# Results will show impact of all 5 parameters on each protocol
```

---

## Parameter Selection Guidelines

### For VBF Studies:
```bash
# Focus on width and range
python3 run_all.py --protocol VBF \
  --vbf-width 50 100 150 \
  --range 100 150 \
  --nodes 100 \
  --runs 20
```

### For DBR Studies:
```bash
# Focus on depth variation
python3 run_all.py --protocol DBR \
  --min-depth 0 --max-depth 200 \
  --runs 15

python3 run_all.py --protocol DBR \
  --min-depth 0 --max-depth 400 \
  --runs 15
```

### For Mobility Studies:
```bash
# Compare all protocols with/without mobility
python3 run_all.py --protocols VBF DBR flooding \
  --mobility static --runs 15

python3 run_all.py --protocols VBF DBR flooding \
  --mobility random --runs 15
```

### For Scalability Studies:
```bash
# Test multiple node counts
python3 run_all.py --protocols VBF DBR \
  --nodes 50 100 150 200 250 \
  --runs 10
```

---

## Directory Structure

```
~/NS3-Research/
├── src/
│   └── uswn-research.cc/
│       └── uwsn-research_v2.cc        # Main simulation
├── scripts/
│   ├── run_all.py                     # ⭐ USE THIS (all-in-one)
│   ├── analyze_results.py             # Optional: standalone analysis
│   ├── plot_results.py                # Optional: standalone plotting
│   └── requirements.txt               # Python dependencies
├── docs/
│   ├── V2_FINAL.md                    # Complete guide
│   ├── V2_Summary.md                  # Quick reference
│   ├── V2_Parameters.md               # Parameter details
│   └── V2_All_In_One.md               # run_all.py usage
├── models/                            # For custom protocol modifications
├── results/                           # CSV outputs (auto-created)
└── graphs/                            # PNG plots (auto-created)
```

---

## Usage Examples

### Basic Protocol Comparison

```bash
cd ~/NS3-Research/scripts

python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15
```

### Quick Test (Fast)

```bash
python3 run_all.py \
  --protocols VBF \
  --nodes 100 \
  --runs 1 \
  --simtime 50
```

### Scalability Study

```bash
python3 run_all.py \
  --protocols VBF DBR \
  --nodes 50 100 150 200 \
  --runs 10
```

### Range Impact Study

```bash
# Test different communication ranges
python3 run_all.py --protocols VBF --nodes 100 --runs 10 --range 80
python3 run_all.py --protocols VBF --nodes 100 --runs 10 --range 120
python3 run_all.py --protocols VBF --nodes 100 --runs 10 --range 150
```

### Mobility Comparison

```bash
# Static nodes
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15 \
  --mobility static

# Mobile nodes (horizontal drift)
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15 \
  --mobility random
```

---

## Command Line Options

### Experiment Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | VBF DBR flooding | Protocols to test (space-separated) |
| `--nodes` | 100 | Node counts to test (space-separated) |
| `--runs` | 30 | Number of runs per configuration |
| `--simtime` | 1000 | Simulation duration (seconds) |

### 5 Configurable Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--range` | 100 | Communication range (meters) |
| `--min-depth` | 0.0 | Minimum depth (meters) |
| `--max-depth` | 400.0 | Maximum depth (meters) |
| `--vbf-width` | 100.0 | VBF routing pipe radius (meters) |
| `--mobility` | static | Mobility model: static or random |

### Control Options

| Option | Description |
|--------|-------------|
| `--skip-build` | Skip building ns-3 (use existing build) |
| `--skip-analysis` | Skip analysis step |
| `--skip-plots` | Skip plotting step |
| `--output` | Custom output CSV path |

---

## What Gets Generated

### 1. CSV Results

**Location**: `results/uwsn_v2_results_TIMESTAMP.csv`

**Columns**:
```
run, protocol, nodes, sinks, simStop,
range, minDepth, maxDepth, vbfWidth, mobility,
areaX, areaY, initial_energy_per_node, tx_power, rx_power, idle_power,
total_initial_energy, total_consumed_energy, total_residual_energy
```

### 2. Console Analysis

```
=======================================================================
   ANALYSIS BY PROTOCOL
=======================================================================

VBF (n=15 runs)
----------------------------------------
  Total Energy: 405.2 ± 12.3 J
  Residual Energy: 50094.8 J

DBR (n=15 runs)
----------------------------------------
  Total Energy: 412.5 ± 15.1 J
  Residual Energy: 50087.5 J
```

### 3. Graphs

**Location**: `scripts/graphs/`

| File | Description |
|------|-------------|
| `v2_energy_comparison.png` | Bar chart comparing total energy |
| `v2_energy_distribution.png` | Box plot showing energy variance |
| `v2_scalability_analysis.png` | Energy vs node count (if multiple node counts tested) |

---

## Topology

```
Area: 400m × 400m × 400m (depth)
Default Range: 100m

     Surface (z=0)
     ┌─────────────────────┐
     │    ⬤ Sink (190,190,0)│
     │                     │
     │    ○ ○ ○ ○ ○ ○     │  Relay nodes
     │   ○ ○ ○ ○ ○ ○ ○    │  (random positions)
     │    ○ ○ ○ ○ ○ ○     │
     │                     │
     │  ◆ Sender (10,10,50) │
     └─────────────────────┘
     Bottom (z=400)
```

**Mobility:**
- `static`: Nodes stay at fixed positions
- `random`: Nodes drift horizontally (X,Y only) at 2 m/s, depth stays constant

---

## Protocols

| Protocol | Description | How Parameters Affect It |
|----------|-------------|-------------------------|
| **VBF** | Vector-Based Forwarding | `--vbf-width` sets routing pipe radius |
| **DBR** | Depth-Based Routing | `--min-depth`, `--max-depth` affect node placement |
| **Flooding** | Broadcast to all neighbors | `--range` affects number of neighbors |

**Note**: Protocol logic is NOT modified. Parameters only affect deployment and configuration.

---

## Requirements

### System
- ns-3 versions 3.40-3.49 (auto-detected in home directory)
- Aqua-Sim-NG module (installed in ns-3 contrib folder)
- Python 3.8+

### Python Packages
```bash
python3 -m pip install -r scripts/requirements.txt
```
- pandas
- matplotlib
- seaborn
- scipy
- numpy

---

## Troubleshooting

### ns-3 Not Found

The script auto-searches your home directory for ns-3.4x installations.

If it can't find ns-3, you'll be prompted to enter the path manually.

To add a custom search location, edit `scripts/run_all.py` and add your path to the search list.

### Python Packages Missing

```bash
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt
```

### Build Errors

```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42  # or your ns-3 path
rm -rf cmake-cache build
./ns3 configure --enable-examples
./ns3 build
```

Then run with `--skip-build`:
```bash
python3 run_all.py --skip-build --protocols VBF --runs 5
```

---

## Documentation

### Start Here:
- 📖 **`docs/V2_FINAL.md`** - Complete overview and file structure

### Reference:
- 📖 **`docs/V2_All_In_One.md`** - How to use run_all.py
- 📖 **`docs/V2_Parameters.md`** - Detailed parameter explanations
- 📖 **`docs/V2_Summary.md`** - Quick reference card

---

## Advanced Usage

### Run Only Specific Steps

```bash
# Only experiments (no analysis/plots)
python3 run_all.py --skip-analysis --skip-plots

# Use existing build
python3 run_all.py --skip-build

# Custom output file
python3 run_all.py --output my_results.csv
```

### Separate Scripts (Optional)

If you prefer to run steps separately:

```bash
# 1. Experiments only
python3 run_all.py --skip-analysis --skip-plots

# 2. Analyze existing CSV
python3 analyze_results.py results/uwsn_v2_results_*.csv

# 3. Generate plots from CSV
python3 plot_results.py results/uwsn_v2_results_*.csv --format png
```

---

## Protocol Modifications

If you want to modify protocol behavior:

1. Copy protocol files from ns-3:
   ```bash
   cp ~/ns3/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-vbf.* \
      ~/NS3-Research/models/
   ```

2. Edit the files in `~/NS3-Research/models/`

3. Update ns-3 to use your modified versions

4. Rebuild ns-3

**Current V2 does NOT modify any protocol logic** - it only adds configurable parameters.

---

## Example Workflow

```bash
# 1. Install dependencies (first time)
cd ~/NS3-Research/scripts
python3 -m pip install -r requirements.txt

# 2. Quick test to verify everything works
python3 run_all.py \
  --protocols VBF \
  --nodes 100 \
  --runs 1 \
  --simtime 50

# 3. Full protocol comparison
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 30

# 4. Check results
ls -lh ../results/
ls -lh graphs/

# 5. View graphs
# Open PNG files in graphs/ directory
```

---

## What Changed from V1

| Feature | V1 | V2 |
|---------|----|----|
| **Parameters** | Fixed in code | 5 configurable from command line |
| **Scripts** | 3 separate scripts | 1 all-in-one script |
| **Mobility** | Static only | Static or Random (X,Y) |
| **VBF Width** | Fixed (100m) | Configurable |
| **Range** | Fixed (100m) | Configurable |
| **Depth** | Fixed (0-400m) | Configurable |

---

## Tips

- 💡 **Quick testing**: Use `--runs 1 --simtime 50`
- 💡 **Publication results**: Use `--runs 30` or more
- 💡 **Save time**: Use `--skip-build` after first run
- 💡 **Multiple experiments**: Change parameters and compare results
- 💡 **Mobility**: Test both `static` and `random` to see protocol robustness

---

## Citation

If you use this framework in your research, please cite:

```
[Your citation information here]
```

---

## License

Research use only. Based on ns-3 and Aqua-Sim-NG.

---

## Support

For issues or questions:
1. Check `docs/V2_FINAL.md` for complete documentation
2. Review troubleshooting section above
3. Ensure all requirements are installed

---

**Ready to run your UWSN experiments!** 🚀
