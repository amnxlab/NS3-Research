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

| # | Parameter | Command Line | Default | Description |
|---|-----------|--------------|---------|-------------|
| 1 | **Nodes** | `--nodes` | 100 | Number of relay nodes |
| 2 | **Range** | `--range` | 100m | Communication range |
| 3 | **Depth** | `--min-depth`, `--max-depth` | 0-400m | Vertical placement range |
| 4 | **VBF Width** | `--vbf-width` | 100m | Routing pipe radius (VBF only) |
| 5 | **Mobility** | `--mobility` | static | static or random (horizontal X,Y only) |

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
