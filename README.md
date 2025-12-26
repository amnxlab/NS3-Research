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

## 🛠️ Manual CLI Usage

If you prefer the terminal:

```bash
# Basic run
python3 scripts/run_all.py --protocols VBF DBR --nodes 100 --runs 15

# Parameter sweep
python3 scripts/run_all.py --protocols VBF DBR \
  --nodes 50 100 200 \
  --simtime 500 1000 \
  --initial-energy 100 500

# Full options
python3 scripts/run_all.py --help
```

---

## 📈 Metrics

- **PDR**: Packet Delivery Ratio = Received / Sent
- **Delay**: Average end-to-end latency
- **Network Lifetime**: Time until first node battery depletion
- **Energy**: Total consumed / residual energy

---

## 📊 Generated Plots

- `alive_nodes_vs_time.png` - Network lifetime visualization
- `network_lifetime.png` - Comparative lifetime analysis
- `pdr_vs_nodes.png` - Scalability analysis
- `v2_energy_comparison.png` - Energy consumption comparison

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
