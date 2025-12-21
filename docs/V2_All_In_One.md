# run_all.py - Complete Workflow Script

## What It Does

**All-in-one script** that:
1. ✅ Runs experiments
2. ✅ Analyzes results  
3. ✅ Generates plots

**No need to run 3 separate scripts!**

---

## Quick Start

```bash
cd ~/NS3-Research/scripts

# Basic usage (VBF, DBR, flooding with 100 nodes, 30 runs)
python3 run_all.py

# Custom protocols and nodes
python3 run_all.py \
  --protocols VBF DBR \
  --nodes 100 \
  --runs 15

# Quick test (1 run, 50 seconds)
python3 run_all.py \
  --protocols VBF \
  --nodes 100 \
  --runs 1 \
  --simtime 50
```

---

## Command Line Options

### Experiment Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | VBF DBR flooding | Protocols to test |
| `--nodes` | 100 | Node counts (space-separated) |
| `--runs` | 30 | Runs per configuration |
| `--simtime` | 1000 | Simulation time (seconds) |

### 5 Configurable Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--range` | 100 | Communication range (m) |
| `--min-depth` | 0.0 | Minimum depth (m) |
| `--max-depth` | 400.0 | Maximum depth (m) |
| `--vbf-width` | 100.0 | VBF pipe radius (m) |
| `--mobility` | static | static or random |

### Control Options

| Option | Description |
|--------|-------------|
| `--skip-build` | Skip building ns-3 (use existing) |
| `--skip-analysis` | Skip analysis step |
| `--skip-plots` | Skip plotting step |
| `--output` | Custom CSV output path |

---

## Usage Examples

### 1. Full Protocol Comparison

```bash
# Compare all 3 protocols
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15
```

**Output**:
- CSV: `results/uwsn_v2_results_TIMESTAMP.csv`
- Plots: `scripts/graphs/v2_*.png`

### 2. Scalability Study

```bash
# Test with different node counts
python3 run_all.py \
  --protocols VBF DBR \
  --nodes 50 100 150 200 \
  --runs 10
```

**Generates**:
- Energy vs Nodes plot
- Scalability analysis

### 3. Range Study

```bash
# Test different communication ranges
python3 run_all.py \
  --protocols VBF \
  --nodes 100 \
  --runs 10 \
  --range 80

python3 run_all.py \
  --protocols VBF \
  --nodes 100 \
  --runs 10 \
  --range 120
```

### 4. Mobility Study

```bash
# Compare static vs mobile
python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15 \
  --mobility static

python3 run_all.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15 \
  --mobility random
```

### 5. Quick Test

```bash
# Fast test (1 run, 50 seconds)
python3 run_all.py \
  --protocols VBF \
  --nodes 100 \
  --runs 1 \
  --simtime 50
```

---

## Output

### Console Output

```
=======================================================================
UWSN PROTOCOL COMPARISON V2 - COMPLETE WORKFLOW
=======================================================================
Output: /home/ahmed/NS3-Research/results/uwsn_v2_results_20241221_050000.csv
Graphs: /home/ahmed/NS3-Research/scripts/graphs
=======================================================================

=======================================================================
STEP 1: RUNNING EXPERIMENTS
=======================================================================
Total experiments: 45
Protocols: ['VBF', 'DBR', 'flooding']
Node counts: [100]
Runs per config: 15

[1/45] Running: VBF | Nodes: 100 | Run: 1 | Range: 100m | Mobility: static
  ✓ Completed: VBF | Nodes: 100 | Run: 1
...

=======================================================================
STEP 2: ANALYZING RESULTS
=======================================================================
Loaded 45 rows from results.csv

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

flooding (n=15 runs)
----------------------------------------
  Total Energy: 450.3 ± 18.2 J
  Residual Energy: 50049.7 J

=======================================================================
STEP 3: GENERATING PLOTS
=======================================================================
Generating energy comparison...
  ✓ Saved: scripts/graphs/v2_energy_comparison.png
Generating energy distribution...
  ✓ Saved: scripts/graphs/v2_energy_distribution.png
Generating scalability analysis...
  ✓ Saved: scripts/graphs/v2_scalability_analysis.png

=======================================================================
✅ COMPLETE WORKFLOW FINISHED!
=======================================================================
Results: /home/ahmed/NS3-Research/results/uwsn_v2_results_20241221_050000.csv
Graphs: /home/ahmed/NS3-Research/scripts/graphs
=======================================================================
```

### Generated Files

```
~/NS3-Research/
├── results/
│   └── uwsn_v2_results_TIMESTAMP.csv  ← Experiment results
└── scripts/
    └── graphs/
        ├── v2_energy_comparison.png      ← Bar chart
        ├── v2_energy_distribution.png    ← Box plot
        └── v2_scalability_analysis.png   ← Line graphs (if multiple node counts)
```

---

## Workflow Steps

### Step 1: Run Experiments
- Copies `uwsn-research_v2.cc` to ns-3 scratch
- Builds ns-3 (unless `--skip-build`)
- Runs all experiment combinations
- Saves results to CSV

### Step 2: Analyze Results
- Loads CSV data
- Calculates statistics by protocol
- Prints summary to console

### Step 3: Generate Plots
- Creates 2-3 publication-quality graphs
- Saves to `scripts/graphs/`
- PNG format, 300 DPI

---

## Skip Options

### Skip Building (If Already Built)
```bash
python3 run_all.py --skip-build
```

### Only Run Experiments (No Analysis/Plots)
```bash
python3 run_all.py --skip-analysis --skip-plots
```

### Only Generate Plots (From Existing CSV)
```bash
# Not supported directly, use separate scripts instead:
python3 plot_results.py results/uwsn_v2_results_*.csv
```

---

## Tips

1. **Quick testing**: Use `--runs 1 --simtime 50` for fast validation
2. **Full comparison**: Use `--runs 30` for publication results
3. **Scalability**: Test with `--nodes 50 100 150 200`
4. **Save time**: Use `--skip-build` after first run

---

## Comparison with Separate Scripts

| Task | Separate Scripts | All-in-One |
|------|------------------|------------|
| Run experiments | `run_experiments_v2.py` | ✅ Built-in |
| Analyze | `analyze_results.py` | ✅ Built-in |
| Plot | `plot_results.py` | ✅ Built-in |
| **Total commands** | **3** | **1** |

**Advantage**: One command does everything! 🎉

---

## Files

- **Script**: `/home/ahmed/NS3-Research/scripts/run_all.py`
- **Source**: `/home/ahmed/NS3-Research/src/uswn-research.cc/uwsn-research_v2.cc`
- **Results**: `/home/ahmed/NS3-Research/results/`
- **Graphs**: `/home/ahmed/NS3-Research/scripts/graphs/`

