# 🌊 UWSN Simulation Dashboard

**Underwater Wireless Sensor Network (UWSN) protocol comparison framework using NS-3.**  
Compare **VBF**, **DBR**, **Flooding**, **EDBR**, and **EVBF** protocols with automated parameter sweeping, real-time visualization, and network lifetime analytics.

---

## 🚀 Quick Start

```bash
python3 scripts/start_dashboard.py
```

Open **http://localhost:8000** in your browser.

> **Note**: On first run, the dashboard automatically installs EDBR/EVBF protocols to AquaSim-NG.

### Running Your First Simulation
1. **Select Protocols**: Check VBF, DBR, Flooding, EDBR, or EVBF
2. **Configure Parameters**: Use default values or click `[+]` to add multiple values for sweeps
3. **Click "Initiate Simulation"**
4. **View Results**: 
   - Real-time console output streams automatically
   - Click **"📊 View Graphics Gallery"** to see generated plots

---

## 🔬 Protocols

| Protocol | Description |
|----------|-------------|
| **VBF** | Vector-Based Forwarding - geographic routing using virtual pipe |
| **DBR** | Depth-Based Routing - forwards packets toward surface using depth |
| **Flooding** | Simple broadcast flooding (baseline) |
| **EDBR** | Energy-aware DBR - prioritizes high-energy nodes for forwarding |
| **EVBF** | Energy-aware VBF - incorporates residual energy into routing decision |

### EDBR/EVBF Energy-Aware Formulas

**EDBR** modifies the holding time:
```
Th = Tmin + k × (1 - Δd/Dmax) × (1 - Eres/Einit)
```

**EVBF** modifies the desirableness factor:
```
α' = α × [1 + β × (1 - Eres/Einit)]
```

Where `Eres` = residual energy, `Einit` = initial energy.  
Higher energy nodes forward packets earlier, balancing network energy consumption.

---

## ✨ Key Features

### 🖥️ Web Dashboard
- **Persistent Execution**: Simulations survive page reloads - the dashboard auto-reconnects
- **Dynamic Parameter Sweeps**: Add multiple values for any parameter using `[+]` buttons
- **Real-Time Console**: Unfiltered CLI output streamed directly to browser
- **Integrated Gallery**: Built-in lightbox for viewing all generated plots
- **Auto-Install**: EDBR/EVBF protocols are automatically installed on first run

### 📊 Analytics
- **Network Lifetime**: Tracks when first nodes die (`alive_nodes_vs_time.png`)
- **PDR & Delay**: Packet Delivery Ratio and End-to-End Delay analysis
- **Energy Profiling**: Per-node energy consumption tracking

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
│   ├── start_dashboard.py      # 🚀 Main launcher (auto-installs EDBR/EVBF)
│   ├── combined_server.py      # Backend (Web + API on port 8000)
│   ├── run_all.py              # Core simulation driver
│   ├── install_edbr_evbf.sh    # EDBR/EVBF protocol installer
│   ├── public/
│   │   └── index.html          # Frontend UI
│   ├── graphs/                 # Generated plots
│   └── results/                # CSV data
├── models/                     # Routing protocol source files
│   ├── aqua-sim-routing-edbr.* # Energy-aware DBR
│   ├── aqua-sim-routing-evbf.* # Energy-aware VBF
│   └── ...                     # Original DBR, VBF, Flooding
└── src/uwsn-research.cc/       # NS-3 simulation code
```

---

## 🛠️ CLI Reference

All GUI functionality is available via command line:

### Basic Usage

```bash
# Run with default settings (all 5 protocols)
python3 scripts/run_all.py

# Specify protocols
python3 scripts/run_all.py --protocols VBF DBR EDBR EVBF

# Quick test (fewer runs)
python3 scripts/run_all.py --protocols EDBR EVBF --nodes 50 --runs 3 --skip-build
```

### Parameter Sweeps

```bash
# Compare original vs energy-aware protocols
python3 scripts/run_all.py --protocols VBF EVBF DBR EDBR --nodes 100 200 --runs 10

# Sweep communication range
python3 scripts/run_all.py --protocols EDBR EVBF --range 100 150 200 --nodes 100
```

### All CLI Options

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | VBF DBR flooding EDBR EVBF | Protocols to test |
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

- **PDR**: Packet Delivery Ratio = Received / Sent
- **Delay**: Average end-to-end latency
- **Network Lifetime**: Time until first node battery depletion
- **Energy Efficiency**: Bits delivered per Joule consumed

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

### EDBR/EVBF not available
```bash
# Manually run the installer
bash scripts/install_edbr_evbf.sh
```

### Low PDR (0%)
- Increase `--range` (e.g., 150m)
- Increase `--nodes` (e.g., 200)
- Check network connectivity in sparse configurations

---

**Verified for NS-3.42 on Linux**
