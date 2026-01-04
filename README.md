# 🌊 UWSN Simulation Dashboard

**v0.7** | Underwater Wireless Sensor Network (UWSN) protocol comparison framework using NS-3/AquaSim-NG.  

Compare **11 routing protocols** including original (VBF, DBR, Flooding), energy-aware variants (EDBR, EDBRv2, EVBF, EVBFv2), and **SNR+Energy-aware variants** (EDBRv3, EDBRv4, EVBFv3, EVBFv4) with automated parameter sweeping, real-time visualization, and network lifetime analytics.

**Author:** Eng. Ahmed Amin  
**Website:** [amnxlab.site](https://www.amnxlab.site/)  
**GitHub:** [github.com/amnxlab](https://github.com/amnxlab)

---

## 🚀 Quick Start

```bash
python3 scripts/start_dashboard.py
```

Open **http://localhost:8000** in your browser.

> **Note**: On first run, the dashboard automatically installs all energy-aware protocols to AquaSim-NG.

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
| **EDBR** | DBR | Linear Energy | Energy-aware DBR with linear energy penalty |
| **EDBRv2** | DBR | Inverse Energy | Energy-aware DBR with inverse scaling |
| **EVBF** | VBF | Linear Energy | Energy-aware VBF with linear energy penalty |
| **EVBFv2** | VBF | Inverse Energy | Energy-aware VBF with inverse scaling |

### 🆕 SNR+Energy-Aware Protocols (v0.7)

| Protocol | Base | Formula Type | Description |
|----------|------|--------------|-------------|
| **EDBRv3** | DBR | Linear SNR+Energy | Considers both energy AND link quality (SNR) |
| **EDBRv4** | DBR | Inverse SNR+Energy | Inverse scaling with SNR factor |
| **EVBFv3** | VBF | Linear SNR+Energy | Considers both energy AND link quality (SNR) |
| **EVBFv4** | VBF | Inverse SNR+Energy | Inverse scaling with SNR factor |

---

## 📐 Protocol Formulas

### Normalization

```
QE = Eres / Einit                                           (Energy ratio)
Qsnr = 0                          if SNR < SNR_min
Qsnr = min(1, max(0, SNR/SNRmax)) otherwise                 (SNR quality)
```

### EDBR/EDBRv2 (Energy-only)

```
EDBR (Linear):    Th = Tmin + k × (1 - Δd/Dmax) × (1 - QE)
EDBRv2 (Inverse): Th = Tmin + k × (1 - Δd/Dmax) / (QE + ε)
```

### EDBRv3/EDBRv4 (SNR+Energy)

```
EDBRv3 (Linear):  Th = Tmin + k × (1 - Δd/Dmax) × (1 - QE) × (1 - Qsnr)
EDBRv4 (Inverse): Th = Tmin + k × (1 - Δd/Dmax) / ((QE + εE) × (Qsnr + εS))
```

### EVBF/EVBFv2 (Energy-only)

```
EVBF (Linear):    α' = α × (1 + β × (1 - QE))
EVBFv2 (Inverse): α' = α / (QE + ε)
```

### EVBFv3/EVBFv4 (SNR+Energy)

```
EVBFv3 (Linear):  α' = α × (1 + β × (1 - QE)) × (1 + δ × (1 - Qsnr))
EVBFv4 (Inverse): α' = α / ((QE + εE) × (Qsnr + εS))

Final delay:      Th = √α' × Tdelay + (range - d) / v0
```

### Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `Tmin` | 0.1s | Minimum backoff time |
| `k` | 4.0 | Scaling factor |
| `Dmax` | 300m | Maximum useful depth gain |
| `β` | 0.5 | Energy bias factor |
| `δ` | 0.5 | SNR bias factor |
| `εE`, `εS` | 0.01 | Division safeguards |
| `SNRmax` | 100.0 | Maximum SNR for normalization |
| `SNRmin` | 10.0 | Minimum acceptable SNR threshold |
| `v0` | 1500 m/s | Sound speed in water |

---

## ✨ Key Features

### 🖥️ Web Dashboard
- **Persistent Execution**: Simulations survive page reloads
- **Dynamic Parameter Sweeps**: Add multiple values using `[+]` buttons
- **Real-Time Console**: Unfiltered CLI output streamed to browser
- **Integrated Gallery**: Built-in lightbox for generated plots
- **Auto-Install**: All protocols automatically installed on first run

### 📊 Analytics & Visualization
- **Network Lifetime**: Tracks when first nodes die over time
- **PDR & Delay**: Packet Delivery Ratio and End-to-End Delay analysis
- **Energy Profiling**: Per-node and total energy consumption tracking
- **Energy Efficiency**: Bits delivered per Joule consumed
- **Comparative Plots**: All protocols on same graph for direct comparison

---

## ⚙️ Configuration

### Sweepable Parameters
Click `[+]` to add multiple values - runs all combinations automatically.

| Parameter | Default | Description |
|-----------|---------|-------------|
| **Nodes** | 100 | Number of relay nodes |
| **Sim Time** | 500s | Simulation duration |
| **Range** | 100m | Communication range |
| **Initial Energy** | 500J | Battery capacity per node |
| **Area X/Y** | 400m | Network dimensions |
| **Min/Max Z** | 0-400m | Depth range |
| **VBF Width** | 100m | VBF/EVBF routing pipe radius |

---

## 📁 Project Structure

```
NS3-Research/
├── scripts/
│   ├── start_dashboard.py      # 🚀 Main launcher
│   ├── combined_server.py      # Backend (Web + API)
│   ├── run_all.py              # Core simulation driver
│   ├── install_edbr_evbf.sh    # Protocol installer
│   ├── plot_comprehensive.py   # Visualization generator
│   └── public/index.html       # Frontend UI
│
├── models/                     # Routing protocol source files
│   ├── aqua-sim-routing-edbr.*     # EDBR (Linear Energy)
│   ├── aqua-sim-routing-edbrv2.*   # EDBRv2 (Inverse Energy)
│   ├── aqua-sim-routing-edbrv3.*   # EDBRv3 (Linear SNR+Energy)
│   ├── aqua-sim-routing-edbrv4.*   # EDBRv4 (Inverse SNR+Energy)
│   ├── aqua-sim-routing-evbf.*     # EVBF (Linear Energy)
│   ├── aqua-sim-routing-evbfv2.*   # EVBFv2 (Inverse Energy)
│   ├── aqua-sim-routing-evbfv3.*   # EVBFv3 (Linear SNR+Energy)
│   └── aqua-sim-routing-evbfv4.*   # EVBFv4 (Inverse SNR+Energy)
│
├── src/uswn-research.cc/
│   └── uwsn-research_v2.cc     # NS-3 simulation code
│
├── docs/                       # Additional documentation
├── results/                    # CSV simulation results
└── README.md
```

---

## 🛠️ CLI Reference

### Basic Usage

```bash
# Run with all 11 protocols
python3 scripts/run_all.py

# Compare SNR+Energy protocols
python3 scripts/run_all.py --protocols EDBRv3 EDBRv4 EVBFv3 EVBFv4

# Quick test
python3 scripts/run_all.py --protocols EDBRv3 EDBRv4 --nodes 50 --runs 3 --skip-build
```

### All CLI Options

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | All 11 protocols | Protocols to test |
| `--nodes` | 100 | Node count(s) |
| `--runs` | 30 | Iterations per configuration |
| `--simtime` | 1000 | Simulation time (seconds) |
| `--range` | 100 | Communication range (meters) |
| `--initial-energy` | 500.0 | Initial battery energy (Joules) |
| `--area-x`, `--area-y` | 400.0 | Network dimensions (meters) |
| `--min-z`, `--max-z` | 0.0, 400.0 | Depth range (meters) |
| `--vbf-width` | 100.0 | VBF routing pipe width (meters) |
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
cat scripts/server.log
pkill -f combined_server
python3 scripts/start_dashboard.py
```

### Protocols not available
```bash
bash scripts/install_edbr_evbf.sh
```

### Build errors after code changes
```bash
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
| **v0.7** | Added EDBRv3, EDBRv4, EVBFv3, EVBFv4 (SNR+Energy-aware protocols) |
| **v0.6** | Added EDBRv2, EVBFv2 (Inverse Scaling variants) |
| **v0.5** | Added EDBR, EVBF (Linear Penalty variants) |
| **v0.4** | Web dashboard with parameter sweeping |
| **v0.3** | Network lifetime analytics |
| **v0.2** | CLI automation and plotting |
| **v0.1** | Initial release with VBF, DBR, Flooding |

---

**Developed by Eng. Ahmed Amin**  
**Verified for NS-3.42 on Linux (Ubuntu 22.04+)**  
**AquaSim-NG compatibility: Tested with contrib/aqua-sim-ng**
