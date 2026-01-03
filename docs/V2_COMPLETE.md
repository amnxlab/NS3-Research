# UWSN Research V2 - Complete Documentation

## Overview

UWSN Protocol Comparison Framework with **7 routing protocols** including energy-aware variants. Supports automated parameter sweeping, real-time visualization, and network lifetime analytics.

---

## Quick Start

```bash
# Start web dashboard
python3 scripts/start_dashboard.py

# Or run via CLI
python3 scripts/run_all.py --protocols VBF DBR EDBR EDBRv2 EVBF EVBFv2 --nodes 100 --runs 5
```

---

## Protocols

### Base Protocols
| Protocol | Description |
|----------|-------------|
| **VBF** | Vector-Based Forwarding (pipeline-based) |
| **DBR** | Depth-Based Routing (greedy depth forwarding) |
| **Flooding** | Broadcast flooding (baseline) |

### Energy-Aware Protocols
| Protocol | Base | Formula | Description |
|----------|------|---------|-------------|
| **EDBR** | DBR | Linear Penalty | `Th = Tmin + k × (1 - Δd/Dmax) × (1 - E)` |
| **EDBRv2** | DBR | Inverse Scaling | `Th = Tmin + k × (1 - Δd/Dmax) × (1/(E + ε))` |
| **EVBF** | VBF | Linear Penalty | `α' = α × [1 + β × (1 - E)]` |
| **EVBFv2** | VBF | Inverse Scaling | `α' = α / (E + ε)` |

Where `E = Eres/Einit` (energy ratio, 0 to 1).

---

## Configurable Parameters

| Option | Default | Description |
|--------|---------|-------------|
| `--protocols` | All 7 | Protocols to test |
| `--nodes` | 100 | Node count(s) |
| `--runs` | 30 | Iterations per config |
| `--simtime` | 1000 | Simulation duration (s) |
| `--range` | 100 | Communication range (m) |
| `--min-z` | 0.0 | Minimum depth (m) |
| `--max-z` | 400.0 | Maximum depth (m) |
| `--vbf-width` | 100.0 | VBF/EVBF pipe width (m) |
| `--mobility` | random | `static` or `random` |
| `--initial-energy` | 500.0 | Initial energy (J) |

---

## Usage Examples

### Protocol Comparison
```bash
python3 run_all.py --protocols VBF DBR EDBR EVBF --nodes 100 --runs 10
```

### Linear vs Inverse Scaling
```bash
python3 run_all.py --protocols EDBR EDBRv2 EVBF EVBFv2 --nodes 100 --runs 15
```

### Scalability Study
```bash
python3 run_all.py --protocols VBF EVBF EVBFv2 --nodes 50 100 200 --runs 5
```

---

## Output

### Metrics Collected
| Metric | Description |
|--------|-------------|
| **PDR** | Packet Delivery Ratio (%) |
| **Delay** | End-to-end latency (ms) |
| **Network Lifetime** | Time until first node dies |
| **Energy Efficiency** | Bits/Joule |
| **Total Energy** | Energy consumed (J) |

### Generated Files
- `results/*.csv` - Simulation data
- `scripts/graphs/*.png` - Visualization plots
- `scripts/traces/*.csv` - Network lifetime traces

---

## Project Structure

```
NS3-Research/
├── scripts/
│   ├── start_dashboard.py      # Web dashboard launcher
│   ├── run_all.py              # Core simulation driver
│   ├── install_edbr_evbf.sh    # Protocol installer
│   └── public/index.html       # Frontend UI
├── models/                     # Protocol source files
│   ├── aqua-sim-routing-edbr.*     # EDBR
│   ├── aqua-sim-routing-edbrv2.*   # EDBRv2
│   ├── aqua-sim-routing-evbf.*     # EVBF
│   └── aqua-sim-routing-evbfv2.*   # EVBFv2
├── src/uwsn-research.cc/       # NS-3 simulation
├── results/                    # CSV output
└── docs/                       # Documentation
```

---

## Energy-Aware Formula Details

### Linear Penalty (`1 - E`)
- Delay increases **linearly** as energy depletes
- At E=1 (full): minimal penalty
- At E=0 (empty): maximum penalty

### Inverse Scaling (`1/(E + ε)`)
- Delay increases **exponentially** as energy depletes
- More aggressive penalty for low-energy nodes
- Uses ε=0.01 to prevent division by zero

### Effect
Higher-energy nodes forward first → balanced network energy → extended lifetime

---

## Troubleshooting

### Protocols Not Available
```bash
bash scripts/install_edbr_evbf.sh
```

### Low PDR
- Increase `--range` (e.g., 150)
- Increase `--nodes` (e.g., 200)
- Reduce `--max-z` to decrease network sparsity

### Build Errors
```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42
./ns3 clean && ./ns3 configure --enable-examples && ./ns3 build
```

---

**Verified for NS-3.42 on Linux**
