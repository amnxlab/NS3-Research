# 🌊 UWSN Simulation Dashboard

**Underwater Wireless Sensor Network (UWSN) protocol comparison framework using NS-3.**  
Compare **VBF**, **DBR**, and **Flooding** protocols with automated parameter sweeping, real-time visualization, and network lifetime analytics.

---

## 🚀 Quick Start

```bash
python3 scripts/start_dashboard.py
```

Open **http://localhost:8000** in your browser.

### Running Your First Simulation
1. **Select Protocols**: Check VBF, DBR, or Flooding
2. **Configure Parameters**: Use default values or click `[+]` to add multiple values for sweeps
3. **Click "Initiate Simulation"**
4. **View Results**: 
   - Real-time console output streams automatically
   - Click **"📊 View Graphics Gallery"** to see generated plots

---

## ✨ Key Features

### 🖥️ Web Dashboard
- **Persistent Execution**: Simulations survive page reloads - the dashboard auto-reconnects
- **Dynamic Parameter Sweeps**: Add multiple values for any parameter using `[+]` buttons
- **Real-Time Console**: Unfiltered CLI output streamed directly to browser
- **Integrated Gallery**: Built-in lightbox for viewing all generated plots

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
| **VBF Width** | 100m | VBF routing pipe radius |

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
│   ├── start_dashboard.py      # 🚀 Main launcher
│   ├── api_server.py            # Backend (SSE streaming)
│   ├── run_all.py               # Core simulation driver
│   ├── public/
│   │   └── index.html           # Frontend UI
│   ├── graphs/                  # Generated plots
│   └── results/                 # CSV data
└── src/uwsn-research.cc/        # NS-3 simulation code
```

---

## 🛠️ CLI Reference

All GUI functionality is available via command line:

### Basic Usage

```bash
# Run with default settings (all 3 protocols, 100 nodes, 30 runs)
python3 scripts/run_all.py

# Specify protocols
python3 scripts/run_all.py --protocols VBF DBR flooding

# Quick test (fewer runs)
python3 scripts/run_all.py --protocols VBF --nodes 100 --runs 5 --skip-build
```

### Parameter Sweeps

Add multiple values to run all combinations automatically:

```bash
# Sweep node counts
python3 scripts/run_all.py --protocols VBF DBR --nodes 50 100 150 200

# Sweep multiple parameters
python3 scripts/run_all.py --protocols VBF DBR flooding \
  --nodes 100 200 \
  --range 100 150 \
  --initial-energy 100 500 1000

# Full parameter sweep example
python3 scripts/run_all.py \
  --protocols VBF DBR \
  --nodes 100 200 \
  --simtime 500 1000 \
  --range 100 150 \
  --area-x 400 600 \
  --area-y 400 600 \
  --min-z 0 \
  --max-z 400 500 \
  --vbf-width 100 150 \
  --initial-energy 500 \
  --mobility random \
  --runs 15
```

### All CLI Options

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | VBF DBR flooding | Protocols to test |
| `--nodes` | 100 | Node count(s) - supports multiple values |
| `--runs` | 30 | Iterations per configuration |
| `--simtime` | 1000 | Simulation time in seconds |
| `--range` | 100 | Communication range in meters |
| `--initial-energy` | 500.0 | Initial battery energy in Joules |
| `--area-x` | 400.0 | Network X dimension (meters) |
| `--area-y` | 400.0 | Network Y dimension (meters) |
| `--min-z` | 0.0 | Minimum depth (meters) |
| `--max-z` | 400.0 | Maximum depth (meters) |
| `--vbf-width` | 100.0 | VBF routing pipe width (meters) |
| `--mobility` | random | Mobility model: `random` or `fixed` |
| `--output` | auto | Custom output CSV file path |
| `--skip-build` | false | Skip NS-3 rebuild (use existing) |
| `--skip-analysis` | false | Skip result analysis step |
| `--skip-plots` | false | Skip graph generation |

### Output Files

Results are saved to:
- **CSV Data**: `results/uwsn_v2_results_YYYYMMDD_HHMMSS.csv`
- **Plots**: `scripts/graphs/uwsn_v2_results_YYYYMMDD_HHMMSS/`
  - `PDR_by_Node_Count.png`
  - `Average_Delay_by_Node_Count.png`
  - `Energy_Efficiency_by_Node_Count.png`
  - `Residual_Energy_by_Node_Count.png`
  - `Network_Lifetime_by_Node_Count.png`
  - `Network_Lifetime_Alive_Nodes.png`

---

## 📈 Metrics

- **PDR**: Packet Delivery Ratio = Received / Sent
- **Delay**: Average end-to-end latency
- **Network Lifetime**: Time until first node battery depletion
- **Energy**: Total consumed / residual energy

---

## 📊 Generated Plots

Each simulation run creates a folder with these graphs:

| File | Description |
|------|-------------|
| `PDR_by_Node_Count.png` | Packet Delivery Ratio comparison |
| `Average_Delay_by_Node_Count.png` | End-to-end latency analysis |
| `Energy_Efficiency_by_Node_Count.png` | Energy efficiency percentage |
| `Residual_Energy_by_Node_Count.png` | Average residual energy per node |
| `Network_Lifetime_by_Node_Count.png` | Network lifetime comparison |
| `Network_Lifetime_Alive_Nodes.png` | Alive nodes over time |

> **Note**: When sweeping parameters like `--range` or `--area-x`, files are named accordingly (e.g., `PDR_by_Range.png`).

---

## 🔧 Troubleshooting

### Dashboard won't start
```bash
# Check logs
cat scripts/api_server.log
cat scripts/public/web_server.log

# Restart
pkill -f api_server
pkill -f "http.server 8000"
python3 scripts/start_dashboard.py
```

### No output in console
- Ensure `api_server.py` is running: `ps aux | grep api_server`
- Check browser console for errors (F12)

### Low PDR (0%)
- Increase `--range` (e.g., 150m)
- Increase `--nodes` (e.g., 200)
- Check network connectivity in sparse configurations

---

**Verified for NS-3.42 on Linux**
