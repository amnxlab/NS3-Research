# UWSN Research V2 - 5 Configurable Parameters

## Overview

`uwsn-research_v2.cc` adds **5 configurable parameters** that can be controlled from Python scripts or command line, **WITHOUT changing any protocol logic**.

---

## The 5 Parameters

### 1. **Number of Nodes** (`--nodes`)
**What it does**: Sets the number of intermediate relay nodes in the network

**Default**: `100`

**Range**: 10 - 500 (recommended)

**Impact**:
- More nodes = denser network = better connectivity = higher PDR
- More nodes = more energy consumption
- Affects all protocols (VBF, DBR, Flooding)

**Example**:
```bash
./ns3 run "uwsn-research_v2 --nodes=150"
```

---

### 2. **Communication Range** (`--range`)
**What it does**: Sets the transmission range for all nodes (in meters)

**Default**: `100` meters

**Range**: 50 - 300 meters (realistic for underwater)

**Impact**:
- Larger range = better connectivity = higher PDR
- Larger range = more energy per transmission
- Affects all protocols equally
- **Used by**: All protocols (VBF, DBR, Flooding)

**Location in code**:
```cpp
// Line 129 in uwsn-research_v2.cc
dev->GetPhy()->SetTransRange(range);  // Parameter 2
```

**Example**:
```bash
./ns3 run "uwsn-research_v2 --range=150"
```

---

### 3. **Depth Variation** (`--minDepth`, `--maxDepth`)
**What it does**: Sets the depth range where nodes are randomly placed (in meters)

**Default**: 
- `minDepth = 0.0` (surface)
- `maxDepth = 400.0` (400m deep)

**Range**: 0 - 1000 meters (typical underwater deployment)

**Impact**:
- Affects vertical distribution of nodes
- **Critical for DBR**: DBR uses depth for routing decisions
- Larger depth range = more 3D spread = affects connectivity
- **Used by**:
  - VBF: Indirect (affects node positions)
  - DBR: Direct (routing based on depth)
  - Flooding: Indirect (affects topology)

**Location in code**:
```cpp
// Line 146 in uwsn-research_v2.cc
std::uniform_real_distribution<> disZ(minDepth, maxDepth);  // Parameter 3
```

**Example**:
```bash
./ns3 run "uwsn-research_v2 --minDepth=0 --maxDepth=200"
```

---

### 4. **VBF Routing Pipe Radius/Width** (`--width`)
**What it does**: Sets the width of the virtual routing pipe in VBF protocol (in meters)

**Default**: `100` meters

**Range**: 50 - 200 meters (typical)

**Impact**:
- **Only affects VBF protocol** (ignored by DBR and Flooding)
- Wider pipe = more nodes can forward = higher PDR but more duplicates
- Narrower pipe = fewer forwarders = lower overhead but lower PDR
- Trade-off between reliability and efficiency

**VBF Protocol Logic** (NOT changed, just parameter passed):
```cpp
// Line 89 in uwsn-research_v2.cc
asHelper.SetRouting("ns3::AquaSimVBF", 
                    "Width", DoubleValue(vbfWidth),  // Parameter 4
                    "TargetPos", Vector3DValue(Vector(sinkX, sinkY, sinkZ)));
```

**Example**:
```bash
./ns3 run "uwsn-research_v2 --protocol=VBF --width=150"
```

---

### 5. **Mobility Model** (`--mobility`)
**What it does**: Sets whether nodes are static or mobile

**Default**: `static`

**Options**:
- `static`: Nodes stay in fixed positions (ConstantPositionMobilityModel)
- `random`: Nodes move **horizontally only (X, Y)** due to water currents (RandomWalk2dMobilityModel)

**Important**: 
- **Movement is in X,Y plane ONLY** - depth (Z) stays constant for each node
- This is realistic for UWSN where nodes float at fixed depth and drift with currents
- Vertical movement (Z-axis) is typically not modeled for regular sensor nodes

**Impact**:
- Static: Predictable topology, easier to analyze
- Random: More realistic for underwater currents/drift
- Mobile nodes = topology changes = routing must adapt
- Affects all protocols differently:
  - **VBF**: Pipeline to sink may break with mobility (horizontal drift)
  - **DBR**: Depth-based routing robust to horizontal mobility (depth doesn't change)
  - **Flooding**: Works with mobility but high overhead

**Location in code**:
```cpp
// Line 160-177 in uwsn-research_v2.cc
if (mobilityModel == "static") {
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
} else if (mobilityModel == "random") {
  // Random walk in X,Y plane only (horizontal movement)
  // Simulates water currents - depth stays constant
  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(0, areaX, 0, areaY)),
                            "Speed", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                            "Distance", DoubleValue(50.0));
}
```

**Example**:
```bash
./ns3 run "uwsn-research_v2 --mobility=random"
```

---

## Usage Examples

### From Command Line (ns-3 directly)

```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42

# Test VBF with custom parameters
./ns3 run "uwsn-research_v2 \
  --protocol=VBF \
  --nodes=150 \
  --range=120 \
  --minDepth=0 \
  --maxDepth=300 \
  --width=150 \
  --mobility=static"

# Test DBR with mobile nodes
./ns3 run "uwsn-research_v2 \
  --protocol=DBR \
  --nodes=100 \
  --range=100 \
  --minDepth=0 \
  --maxDepth=400 \
  --mobility=random"
```

### From Python Script

```bash
cd ~/NS3-Research/scripts

# Compare protocols with different ranges
python3 run_experiments_v2.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 10 \
  --range 150 \
  --min-depth 0 \
  --max-depth 300 \
  --vbf-width 120 \
  --mobility static

# Test scalability (different node counts)
python3 run_experiments_v2.py \
  --protocols VBF \
  --nodes 50 100 150 200 \
  --runs 5 \
  --range 100

# Test mobility impact
python3 run_experiments_v2.py \
  --protocols VBF DBR \
  --nodes 100 \
  --runs 10 \
  --mobility random
```

---

## CSV Output Format

The V2 simulation saves results with all 5 parameters:

```csv
run,protocol,nodes,sinks,simStop,range,minDepth,maxDepth,vbfWidth,mobility,areaX,areaY,initial_energy_per_node,tx_power,rx_power,idle_power,total_initial_energy,total_consumed_energy,total_residual_energy
1,VBF,100,1,1000,100,0.0,400.0,100.0,static,400.0,400.0,500.0,2.0,0.1,0.001,50500.0,405.2,50094.8
```

---

## Parameter Combinations for Research

### 1. **Range Study** (Impact of communication range)
```bash
python3 run_experiments_v2.py --protocols VBF DBR --nodes 100 --range 80 --runs 15
python3 run_experiments_v2.py --protocols VBF DBR --nodes 100 --range 100 --runs 15
python3 run_experiments_v2.py --protocols VBF DBR --nodes 100 --range 120 --runs 15
python3 run_experiments_v2.py --protocols VBF DBR --nodes 100 --range 150 --runs 15
```

### 2. **Depth Study** (Impact of depth variation)
```bash
python3 run_experiments_v2.py --protocols DBR --nodes 100 --max-depth 200 --runs 15
python3 run_experiments_v2.py --protocols DBR --nodes 100 --max-depth 400 --runs 15
python3 run_experiments_v2.py --protocols DBR --nodes 100 --max-depth 600 --runs 15
```

### 3. **VBF Width Study** (Impact of routing pipe)
```bash
python3 run_experiments_v2.py --protocols VBF --nodes 100 --vbf-width 50 --runs 15
python3 run_experiments_v2.py --protocols VBF --nodes 100 --vbf-width 100 --runs 15
python3 run_experiments_v2.py --protocols VBF --nodes 100 --vbf-width 150 --runs 15
python3 run_experiments_v2.py --protocols VBF --nodes 100 --vbf-width 200 --runs 15
```

### 4. **Mobility Study** (Static vs Mobile)
```bash
python3 run_experiments_v2.py --protocols VBF DBR flooding --nodes 100 --mobility static --runs 15
python3 run_experiments_v2.py --protocols VBF DBR flooding --nodes 100 --mobility random --runs 15
```

### 5. **Scalability Study** (Node count)
```bash
python3 run_experiments_v2.py --protocols VBF DBR --nodes 50 100 150 200 --runs 10
```

---

## Important Notes

### ✅ What Was Changed
- Added 5 command-line parameters
- All changes marked with `// ==== EDITED ====` comments
- Enhanced console output to show parameters
- Updated CSV format to include parameters

### ❌ What Was NOT Changed
- **No changes to VBF protocol logic** (in aqua-sim-ng library)
- **No changes to DBR protocol logic** (in aqua-sim-ng library)
- **No changes to Flooding protocol logic** (in aqua-sim-ng library)
- **No changes to energy model**
- **No changes to packet forwarding algorithms**

### 📍 Where Parameters Are Used

| Parameter | Line in Code | How It's Used |
|-----------|--------------|---------------|
| `--nodes` | Line 68 | `nodesCon.Create(nodes)` |
| `--range` | Line 209 | `dev->GetPhy()->SetTransRange(range)` |
| `--minDepth`, `--maxDepth` | Line 146 | `disZ(minDepth, maxDepth)` for node placement |
| `--width` | Line 89 | `"Width", DoubleValue(vbfWidth)` for VBF |
| `--mobility` | Line 157 | Mobility model selection |

---

## Files Modified

1. ✅ `/home/ahmed/NS3-Research/src/uswn-research.cc/uwsn-research_v2.cc`
   - Added 5 parameters
   - All changes marked with comments

2. ✅ `/home/ahmed/NS3-Research/scripts/run_experiments_v2.py`
   - Python script to run batch experiments
   - Passes 5 parameters to simulation

3. ✅ `/home/ahmed/NS3-Research/docs/V2_Parameters.md`
   - This documentation file

---

## Next Steps

1. **Test the simulation**:
   ```bash
   cd ~/NS3-Research/scripts
   python3 run_experiments_v2.py --protocols VBF --nodes 100 --runs 1 --simtime 100
   ```

2. **Run full experiments**:
   ```bash
   python3 run_experiments_v2.py --protocols VBF DBR flooding --nodes 100 --runs 15
   ```

3. **Analyze results** (will need updated analyze script for V2 CSV format)

4. **Generate graphs** (will need updated plot script for V2 CSV format)

