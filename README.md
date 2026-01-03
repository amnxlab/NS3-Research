# 🌊 UWSN Simulation Dashboard

**Underwater Wireless Sensor Network (UWSN) protocol comparison framework using NS-3/AquaSim-NG.**  
Compare **7 routing protocols** including original (VBF, DBR, Flooding) and energy-aware variants (EDBR, EDBRv2, EVBF, EVBFv2) with automated parameter sweeping, real-time visualization, and network lifetime analytics.

---

## 🚀 Quick Start

```bash
python3 scripts/start_dashboard.py
```

Open **http://localhost:8000** in your browser.

> **Note**: On first run, the dashboard automatically installs all energy-aware protocols (EDBR, EDBRv2, EVBF, EVBFv2) to AquaSim-NG.

### Running Your First Simulation
1. **Select Protocols**: Check any combination of 7 available protocols
2. **Configure Parameters**: Use default values or click `[+]` to add multiple values for sweeps
3. **Click "Initiate Simulation"**
4. **View Results**: 
   - Real-time console output streams automatically
   - Click **"📊 View Graphics Gallery"** to see generated plots

---

## 🔬 Protocols

### Base Protocols

| Protocol | Description |
|----------|-------------|
| **VBF** | Vector-Based Forwarding - geographic routing using virtual pipe |
| **DBR** | Depth-Based Routing - forwards packets toward surface using depth |
| **Flooding** | Simple broadcast flooding (baseline for comparison) |

### Energy-Aware Protocols

| Protocol | Base | Formula Type | Description |
|----------|------|--------------|-------------|
| **EDBR** | DBR | Linear Penalty | Energy-aware DBR with linear energy penalty |
| **EDBRv2** | DBR | Inverse Scaling | Energy-aware DBR with inverse scaling |
| **EVBF** | VBF | Linear Penalty | Energy-aware VBF with linear energy penalty |
| **EVBFv2** | VBF | Inverse Scaling | Energy-aware VBF with inverse scaling |

---

## 📐 Energy-Aware Formulas

### Formula Comparison

| Type | Formula | Behavior |
|------|---------|----------|
| **Linear Penalty** | `α' = α × [1 + β × (1 - E)]` | Delay increases linearly as energy depletes |
| **Inverse Scaling** | `α' = α / (E + ε)` | Delay increases exponentially as energy depletes |

Where:
- `α` = Original desirableness/priority factor
- `E` = `Eres/Einit` (energy ratio, 0 to 1)
- `β` = Energy bias factor (default: 0.5)
- `ε` = Small constant to prevent division by zero (default: 0.01)

### EDBR/EDBRv2 (DBR-based)

Modifies the **holding time** before forwarding:

```
EDBR (Linear):    Th = Tmin + k × (1 - Δd/Dmax) × (1 - Eres/Einit)
EDBRv2 (Inverse): Th = Tmin + k × (1 - Δd/Dmax) × (1 / (Eres/Einit + ε))
```

Parameters:
- `Tmin` = Minimum backoff time (default: 0.1s)
- `k` = Scaling factor (default: 4.0)
- `Dmax` = Maximum useful depth gain (default: 300m)

### EVBF/EVBFv2 (VBF-based)

Modifies the **desirableness factor** (α) that determines priority:

```
EVBF (Linear):    α' = α × [1 + β × (1 - Eres/Einit)]
EVBFv2 (Inverse): α' = α / (Eres/Einit + ε)
```

Parameters:
- `β` = Energy bias factor (default: 0.5)
- `Tdelay` = Maximum delay constant (default: 0.5s)

**Effect**: Higher-energy nodes get lower delay → forward packets first → balanced network energy consumption.

---

## ✨ Key Features

### 🖥️ Web Dashboard
- **Persistent Execution**: Simulations survive page reloads - the dashboard auto-reconnects
- **Dynamic Parameter Sweeps**: Add multiple values for any parameter using `[+]` buttons
- **Real-Time Console**: Unfiltered CLI output streamed directly to browser
- **Integrated Gallery**: Built-in lightbox for viewing all generated plots
- **Auto-Install**: All energy-aware protocols automatically installed on first run

### 📊 Analytics & Visualization
- **Network Lifetime**: Tracks when first nodes die over time
- **PDR & Delay**: Packet Delivery Ratio and End-to-End Delay analysis
- **Energy Profiling**: Per-node and total energy consumption tracking
- **Energy Efficiency**: Bits delivered per Joule consumed
- **Comparative Plots**: All protocols on same graph for direct comparison

---

## ⚙️ Configuration

### Sweepable Parameters
Click `[+]` to add multiple values - the system runs all combinations automatically.

| Parameter | Default | Description |
|-----------|---------|-------------|
| **Nodes** | 100 | Number of relay nodes |
| **Sim Time** | 500s | Simulation duration |
| **Range** | 100m | Communication range |
| **Initial Energy** | 500J | Battery capacity per node |
| **Area X/Y** | 400m | Network dimensions |
| **Min/Max Z** | 0-400m | Depth range |
| **VBF Width** | 100m | VBF/EVBF routing pipe radius |

### Single-Value Parameters
| Parameter | Options | Description |
|-----------|---------|-------------|
| **Runs** | 15 | Iterations per config (for averaging) |
| **Mobility** | random/fixed | Node movement model |

---

## 📁 Project Structure

```
NS3-Research/
├── scripts/
│   ├── start_dashboard.py      # 🚀 Main launcher (auto-installs protocols)
│   ├── combined_server.py      # Backend (Web + API on port 8000)
│   ├── run_all.py              # Core simulation driver
│   ├── install_edbr_evbf.sh    # Protocol installer for AquaSim-NG
│   ├── plot_comprehensive.py   # Visualization generator
│   ├── public/
│   │   └── index.html          # Frontend UI
│   ├── graphs/                 # Generated plots
│   └── traces/                 # Network lifetime traces
│
├── models/                     # Routing protocol source files
│   ├── aqua-sim-routing-edbr.*     # EDBR (Linear Penalty)
│   ├── aqua-sim-routing-edbrv2.*   # EDBRv2 (Inverse Scaling)
│   ├── aqua-sim-routing-evbf.*     # EVBF (Linear Penalty)
│   ├── aqua-sim-routing-evbfv2.*   # EVBFv2 (Inverse Scaling)
│   └── aqua-sim-routing-{vbf,dbr,flooding}.*  # Original protocols
│
├── src/uswn-research.cc/
│   └── uwsn-research_v2.cc     # NS-3 simulation code
│
├── results/                    # CSV simulation results
├── docs/                       # Additional documentation
└── README.md
```

---

## 🛠️ CLI Reference

All GUI functionality is available via command line:

### Basic Usage

```bash
# Run with default settings (all 7 protocols)
python3 scripts/run_all.py

# Specify protocols
python3 scripts/run_all.py --protocols VBF DBR EDBR EDBRv2 EVBF EVBFv2

# Quick test (fewer runs)
python3 scripts/run_all.py --protocols EDBR EDBRv2 EVBF EVBFv2 --nodes 50 --runs 3 --skip-build
```

### Parameter Sweeps

```bash
# Compare linear vs inverse scaling
python3 scripts/run_all.py --protocols EDBR EDBRv2 EVBF EVBFv2 --nodes 100 --runs 10

# Sweep node counts
python3 scripts/run_all.py --protocols VBF EVBF EVBFv2 --nodes 50 100 200 --runs 5

# Sweep communication range
python3 scripts/run_all.py --protocols EDBR EDBRv2 --range 100 150 200 --nodes 100
```

### All CLI Options

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | VBF DBR flooding EDBR EVBF EDBRv2 EVBFv2 | Protocols to test |
| `--nodes` | 100 | Node count(s) - supports multiple values |
| `--runs` | 30 | Iterations per configuration |
| `--simtime` | 1000 | Simulation time in seconds |
| `--range` | 100 | Communication range in meters |
| `--initial-energy` | 500.0 | Initial battery energy in Joules |
| `--area-x` | 400.0 | Network X dimension (meters) |
| `--area-y` | 400.0 | Network Y dimension (meters) |
| `--min-z` | 0.0 | Minimum depth (meters) |
| `--max-z` | 400.0 | Maximum depth (meters) |
| `--vbf-width` | 100.0 | VBF/EVBF routing pipe width (meters) |
| `--mobility` | random | Mobility model: `random` or `fixed` |
| `--skip-build` | false | Skip NS-3 rebuild |

---

## 📈 Metrics

| Metric | Description |
|--------|-------------|
| **PDR** | Packet Delivery Ratio = Unique Received / Sent |
| **Delay** | Average end-to-end latency (ms) |
| **Network Lifetime** | Time until first node battery depletion |
| **Energy Efficiency** | Bits delivered per Joule consumed |
| **Total Consumed Energy** | Sum of energy used by all nodes |
| **Residual Energy** | Remaining battery across network |

---

## 🔧 Troubleshooting

### Dashboard won't start
```bash
# Check logs
cat scripts/server.log

# Restart
pkill -f combined_server
python3 scripts/start_dashboard.py
```

### Protocols not available
```bash
# Manually run the installer
bash scripts/install_edbr_evbf.sh
```

### Low PDR (0%)
- Increase `--range` (e.g., 150m)
- Increase `--nodes` (e.g., 200)
- Check network connectivity in sparse configurations
- Try reducing `--max-z` to bring nodes closer together

### Build errors after code changes
```bash
# Force rebuild
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 clean
./ns3 configure --enable-examples
./ns3 build
```

---

## 📚 References

- **VBF**: Xie et al., "VBF: Vector-Based Forwarding Protocol for Underwater Sensor Networks"
- **DBR**: Yan et al., "DBR: Depth-Based Routing for Underwater Sensor Networks"
- **AquaSim-NG**: https://github.com/rmartin5/aqua-sim-ng
- **NS-3**: https://www.nsnam.org/

---

## 📝 Version History

| Version | Changes |
|---------|---------|
| **v2.0** | Added EDBRv2, EVBFv2 (Inverse Scaling variants) |
| **v1.5** | Added EDBR, EVBF (Linear Penalty variants) |
| **v1.0** | Initial release with VBF, DBR, Flooding |

---

**Verified for NS-3.42 on Linux (Ubuntu 22.04+)**  
**AquaSim-NG compatibility: Tested with contrib/aqua-sim-ng**
