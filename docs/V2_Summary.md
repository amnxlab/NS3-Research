# UWSN Research V2 - Summary

## ✅ Completed Tasks

### 1. Created `uwsn-research_v2.cc`
**Location**: `/home/ahmed/NS3-Research/src/uswn-research.cc/uwsn-research_v2.cc`

**What was added**:
- 5 configurable parameters from command line
- All changes marked with `// ==== EDITED ====` comments
- **NO changes to protocol logic** (VBF, DBR, Flooding)

### 2. Created `run_experiments_v2.py`
**Location**: `/home/ahmed/NS3-Research/scripts/run_experiments_v2.py`

**What it does**:
- Runs batch experiments with V2 simulation
- Passes all 5 parameters from Python to C++
- Auto-detects ns-3 installation
- Generates timestamped CSV files

### 3. Created Documentation
**Location**: `/home/ahmed/NS3-Research/docs/V2_Parameters.md`

**Contents**:
- Explanation of each parameter
- Usage examples
- Research scenarios
- CSV format

---

## The 5 Parameters

| # | Parameter | Command Line | Python Arg | Default | Description |
|---|-----------|--------------|------------|---------|-------------|
| 1 | **Nodes** | `--nodes` | `--nodes` | 100 | Number of relay nodes |
| 2 | **Range** | `--range` | `--range` | 100m | Communication range |
| 3 | **Depth** | `--minDepth`, `--maxDepth` | `--min-depth`, `--max-depth` | 0-400m | Vertical placement range |
| 4 | **VBF Width** | `--width` | `--vbf-width` | 100m | Routing pipe radius (VBF only) |
| 5 | **Mobility** | `--mobility` | `--mobility` | static | static or random movement |

---

## File Structure

```
~/NS3-Research/
├── src/
│   └── uswn-research.cc/
│       ├── uwsn-research_v2.cc  ← NEW: V2 simulation with 5 parameters
│       └── VBF.cc               ← Your reference file
├── scripts/
│   ├── run_experiments_v2.py    ← NEW: Python script for V2
│   ├── run_experiments.py       ← OLD: Original script (still works)
│   ├── analyze_results.py       ← Will need update for V2 CSV
│   └── plot_results.py          ← Will need update for V2 CSV
├── docs/
│   ├── V2_Parameters.md         ← NEW: Parameter documentation
│   └── Implementation_Summary.md ← Existing docs
└── models/
    └── aqua-sim-routing-vbf.h   ← Your VBF header copy
```

---

## Usage

### Quick Test (Command Line)

```bash
# Copy to ns-3
cp ~/NS3-Research/src/uswn-research.cc/uwsn-research_v2.cc \
   ~/ns3/ns-allinone-3.42/ns-3.42/scratch/

# Build
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 build

# Test with custom parameters
./ns3 run "uwsn-research_v2 \
  --protocol=VBF \
  --nodes=150 \
  --range=120 \
  --minDepth=0 \
  --maxDepth=300 \
  --width=150 \
  --mobility=static"
```

### Batch Experiments (Python)

```bash
cd ~/NS3-Research/scripts

# Single run test
python3 run_experiments_v2.py \
  --protocols VBF \
  --nodes 100 \
  --runs 1 \
  --simtime 100

# Full comparison
python3 run_experiments_v2.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15 \
  --range 150 \
  --vbf-width 120 \
  --mobility static
```

---

## What's Next

### Immediate:
1. ✅ Test V2 simulation works
2. ✅ Verify 5 parameters are being passed correctly
3. ✅ Check CSV output format

### Soon:
1. Update `analyze_results.py` for V2 CSV format
2. Update `plot_results.py` for V2 CSV format
3. Run parametric studies (range, depth, width, mobility)

### Later:
1. If you want to modify VBF/DBR protocols:
   - Copy from `/home/ahmed/ns3/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/`
   - Put in `/home/ahmed/NS3-Research/models/`
   - Make modifications there
   - Document changes

---

## Important Guarantees

### ✅ Protocol Logic NOT Changed
- VBF forwarding algorithm: **UNTOUCHED**
- DBR depth-based routing: **UNTOUCHED**
- Flooding broadcast logic: **UNTOUCHED**
- Energy model: **UNTOUCHED**

### ✅ All Changes Documented
Every modification in `uwsn-research_v2.cc` is marked:
```cpp
// ==== EDITED BY ASSISTANT ====
// Description of change
// ...code...
// ==== END OF EDIT ====
```

### ✅ Modular Design
- V2 is separate from original
- Can run both V1 and V2
- No interference between versions

---

## Testing Checklist

```bash
# 1. Copy and build
cp ~/NS3-Research/src/uswn-research.cc/uwsn-research_v2.cc \
   ~/ns3/ns-allinone-3.42/ns-3.42/scratch/
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 build

# 2. Test parameter 1 (nodes)
./ns3 run "uwsn-research_v2 --nodes=50 --simStop=50"
./ns3 run "uwsn-research_v2 --nodes=150 --simStop=50"

# 3. Test parameter 2 (range)
./ns3 run "uwsn-research_v2 --range=80 --simStop=50"
./ns3 run "uwsn-research_v2 --range=150 --simStop=50"

# 4. Test parameter 3 (depth)
./ns3 run "uwsn-research_v2 --maxDepth=200 --simStop=50"
./ns3 run "uwsn-research_v2 --maxDepth=600 --simStop=50"

# 5. Test parameter 4 (VBF width)
./ns3 run "uwsn-research_v2 --protocol=VBF --width=50 --simStop=50"
./ns3 run "uwsn-research_v2 --protocol=VBF --width=200 --simStop=50"

# 6. Test parameter 5 (mobility)
./ns3 run "uwsn-research_v2 --mobility=static --simStop=50"
./ns3 run "uwsn-research_v2 --mobility=random --simStop=50"

# 7. Test Python script
cd ~/NS3-Research/scripts
python3 run_experiments_v2.py --protocols VBF --nodes 100 --runs 1 --simtime 50
```

---

## Questions?

1. **Where are protocol files?**
   - Original: `/home/ahmed/ns3/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/`
   - Your copies: `/home/ahmed/NS3-Research/models/` (when you make them)

2. **How to modify protocols?**
   - Copy from contrib/aqua-sim-ng/model/
   - Edit in ~/NS3-Research/models/
   - Update CMakeLists.txt
   - Rebuild ns-3

3. **CSV format changed?**
   - Yes, V2 CSV has 5 extra columns for the parameters
   - Need to update analyze/plot scripts

4. **Can I run both V1 and V2?**
   - Yes! They're separate programs
   - Use `uwsn-research` for V1
   - Use `uwsn-research_v2` for V2

