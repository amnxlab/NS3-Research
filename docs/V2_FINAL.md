# UWSN Research V2 - Final File Structure

## ✅ Cleaned Up - Only V2 Files Remain

---

## Project Structure

```
~/NS3-Research/
├── src/
│   └── uswn-research.cc/
│       ├── uwsn-research_v2.cc  ✅ Main simulation (5 parameters)
│       └── VBF.cc               ✅ Reference file
│
├── scripts/
│   ├── run_all.py               ✅ ALL-IN-ONE (experiments + analysis + plots)
│   ├── analyze_results.py       ✅ Standalone analysis (optional)
│   ├── plot_results.py          ✅ Standalone plotting (optional)
│   └── requirements.txt         ✅ Python dependencies
│
├── docs/
│   ├── V2_Summary.md            ✅ Quick overview
│   ├── V2_Parameters.md         ✅ Detailed parameter docs
│   └── V2_All_In_One.md         ✅ run_all.py usage guide
│
├── models/
│   └── aqua-sim-routing-vbf.h   ✅ Your VBF header copy
│
├── examples/
│   └── (empty - for future reference files)
│
├── results/                      ✅ Auto-created (CSV outputs)
├── graphs/                       ✅ Auto-created (PNG plots)
├── README.md                     ✅ Main project README
└── .gitignore                    ✅ Git configuration
```

---

## Deleted Files (Cleanup)

### Scripts (Old V1):
- ❌ `run_experiments.py` - Old V1 script
- ❌ `run_experiments_v2.py` - Separate V2 script (merged into run_all.py)
- ❌ `test_setup.py` - Not needed

### Docs (Old):
- ❌ `Implementation_Summary.md` - Old V1 docs
- ❌ `Enhancement_Suggestions.md` - Not needed

---

## Files You Actually Use

### For Running Experiments:

**Main Script** (Use This!):
```bash
scripts/run_all.py
```
- ✅ Runs experiments
- ✅ Analyzes results
- ✅ Generates plots
- ✅ Everything in one command!

**Optional** (if you want separate steps):
```bash
scripts/analyze_results.py  # Just analysis
scripts/plot_results.py     # Just plotting
```

### For Understanding:

**Read These**:
1. `docs/V2_Summary.md` - Start here (overview)
2. `docs/V2_Parameters.md` - Details on 5 parameters
3. `docs/V2_All_In_One.md` - How to use run_all.py

---

## Quick Start Workflow

```bash
# 1. Navigate to scripts
cd ~/NS3-Research/scripts

# 2. Install dependencies (first time only)
python3 -m pip install -r requirements.txt

# 3. Run everything (experiments + analysis + plots)
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15

# Done! Check results:
# - CSV: ../results/uwsn_v2_results_TIMESTAMP.csv
# - Plots: graphs/*.png
```

---

## The 5 Configurable Parameters

| # | Parameter | Command Line Flag | What It Does |
|---|-----------|------------------|--------------|
| 1 | **Nodes** | `--nodes` | Number of relay nodes |
| 2 | **Range** | `--range` | Communication range (meters) |
| 3 | **Depth** | `--min-depth`, `--max-depth` | Vertical node placement range |
| 4 | **VBF Width** | `--vbf-width` | Routing pipe radius (VBF only) |
| 5 | **Mobility** | `--mobility` | static or random (X,Y only) |

---

## What We Changed in uwsn-research_v2.cc

### Mobility Section (Your Question)

**Original** (Lines 110-112 in uwsn-research_old.cc):
```cpp
MobilityHelper mobility;
mobility.SetPositionAllocator(position);
mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");  // Always static
```

**V2** (Lines 167-181 in uwsn-research_v2.cc):
```cpp
MobilityHelper mobility;
mobility.SetPositionAllocator(position);

if (mobilityModel == "static") {
  // SAME AS ORIGINAL - No change when static
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
} else if (mobilityModel == "random") {
  // NEW OPTION - Horizontal movement only (X,Y)
  mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(0, areaX, 0, areaY)),
                            "Speed", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                            "Distance", DoubleValue(50.0));
}
```

**Summary**:
- ✅ **NO LOGIC CHANGED** when using `--mobility=static` (default)
- ✅ **Original behavior = PRESERVED** (just wrapped in if statement)
- ✅ **New feature added** = Optional horizontal mobility with `--mobility=random`
- ✅ **Depth stays constant** = Nodes move in X,Y plane only (realistic for UWSN)

---

## Protocol Logic Status

| Protocol | Status | Files |
|----------|--------|-------|
| **VBF** | ✅ UNCHANGED | Uses ns-3 library (contrib/aqua-sim-ng/model/) |
| **DBR** | ✅ UNCHANGED | Uses ns-3 library (contrib/aqua-sim-ng/model/) |
| **Flooding** | ✅ UNCHANGED | Uses ns-3 library (contrib/aqua-sim-ng/model/) |

**All changes are in uwsn-research_v2.cc ONLY:**
- Added command-line parameters
- Added parameter selection logic (if/else)
- NO changes to protocol algorithms

---

## Usage Examples

### Basic Comparison
```bash
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15
```

### Test Different Ranges
```bash
python3 run_all.py --protocols VBF --nodes 100 --runs 10 --range 80
python3 run_all.py --protocols VBF --nodes 100 --runs 10 --range 120
python3 run_all.py --protocols VBF --nodes 100 --runs 10 --range 150
```

### Test Static vs Mobile
```bash
python3 run_all.py --protocols VBF DBR --nodes 100 --runs 15 --mobility static
python3 run_all.py --protocols VBF DBR --nodes 100 --runs 15 --mobility random
```

### Quick Test
```bash
python3 run_all.py --protocols VBF --nodes 100 --runs 1 --simtime 50
```

---

## Summary

✅ **Cleaned up to V2 only**
✅ **One main script: run_all.py**
✅ **Clear documentation (3 files)**
✅ **No protocol logic changed**
✅ **5 parameters configurable**
✅ **Ready for research!**

---

## Next Steps

1. ✅ Copy `uwsn-research_v2.cc` to ns-3 scratch folder
2. ✅ Build ns-3
3. ✅ Run `python3 run_all.py` with your desired parameters
4. ✅ Analyze results and plots
5. ✅ Modify protocol files (in models/) if needed later

