# UWSN Simulation Framework - Implementation Summary

> **Project**: Underwater Wireless Sensor Network (UWSN) Protocol Comparison
> **Base**: ns-3.42 + Aqua-Sim-NG
> **Date**: December 2024

---

## Table of Contents

- [[#Acronyms]]
- [[#What Was Implemented]]
- [[#Simulation Details]]
- [[#Automation Scripts]]
- [[#Usage Examples]]

---

## Acronyms

| Acronym | Full Name |
|---------|-----------|
| **UWSN** | Underwater Wireless Sensor Network |
| **PDR** | Packet Delivery Ratio |
| **CBR** | Constant Bit Rate |
| **VBF** | Vector-Based Forwarding |
| **DBR** | Depth-Based Routing |

---

## What Was Implemented

### Starting Point

Original `uwsn-research.cc` simulation with:
- Basic UWSN setup using Aqua-Sim-NG
- VBF routing protocol
- Energy consumption tracking
- OnOff traffic generation
- Manual PDR calculation from console output

### Enhancements Added

| Feature | Description |
|---------|-------------|
| **CBR Traffic** | `--cbr=true` flag for constant bit rate traffic |
| **Auto PDR** | End-to-end PDR calculated and saved to CSV |
| **Command Line** | Configurable nodes, simStop, range, dataRate, etc. |
| **Better Topology** | Adjusted area (200×200×150) for reliable connectivity |
| **Batch Automation** | Python script runs multiple experiments |
| **Statistics** | Mean, std, 95% confidence intervals |
| **Graphs** | 10 publication-quality plot types |

---

## Simulation Details

### Traffic Generation

**OnOff Mode (Original)**
```cpp
app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0066]"));
app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.9934]"));
```
- ~0.66% duty cycle
- Sporadic transmission

**CBR Mode (Added)**
```cpp
app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]"));
app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
```
- Continuous transmission
- Enabled with `--cbr=true`

### PDR Calculation

End-to-end PDR using device-specific counters:

```cpp
// Sender's transmitted packets
int senderSent = senderDev->TotalSentPkts();

// Sink's received packets (at routing layer)
int sinkRecv = sinkDev->GetRouting()->SendUpPktCount();

// PDR
double pdr = (double)sinkRecv / senderSent * 100.0;
```

### Topology

```
Before (connectivity issues):
  Area: 400×400×400m
  Sender: (10, 10, 50)
  Sink: (190, 190, 0)
  Distance: ~260m > Range (100m) ❌

After (fixed):
  Area: 200×200×150m
  Sender: (50, 50, 120)
  Sink: (100, 100, 0)
  Distance: ~139m with relay nodes ✓
```

### CSV Output Format

```csv
run,protocol,nodes,simStop,range,traffic_mode,data_rate,packet_size,
sender_sent,sink_recv,pdr,
total_initial_energy,total_consumed_energy,avg_energy_per_node
```

---

## Automation Scripts

### run_experiments.py

Runs multiple simulations with different parameters.

```bash
python3 run_experiments.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15 \
  --simtime 500 \
  --cbr
```

**What it does:**
1. Copies simulation to ns-3 scratch folder
2. Builds ns-3
3. Runs each experiment combination
4. Parses console output for PDR
5. Saves all results to single CSV

### analyze_results.py

Calculates statistics from results.

```bash
python3 analyze_results.py results.csv
```

**Output:**
```
Protocol: VBF (n=15 runs)
  PDR: 100.0% ± 0.0% (95% CI: [100.0%, 100.0%])
  Energy: 4.16 J/node ± 0.79 J
```

### plot_results.py

Generates publication-quality graphs.

```bash
python3 plot_results.py results.csv --format png
```

**Graphs generated:**
- `pdr_bar.png` - Bar chart comparing protocols
- `pdr_line.png` - PDR vs node count
- `pdr_boxplot.png` - PDR distribution
- `energy_line.png` - Energy consumption
- `summary_dashboard.png` - 4-panel overview

---

## Usage Examples

### Single Run

```bash
cd ~/ns3/ns-allinone-3.42/ns-3.42

# VBF with OnOff traffic
./ns3 run "uwsn-research --protocol=VBF --simStop=500"

# DBR with CBR traffic
./ns3 run "uwsn-research --protocol=DBR --simStop=500 --cbr=true"
```

### Batch Experiments

```bash
cd ~/NS3-Research/scripts

# Compare protocols
python3 run_experiments.py \
  --protocols VBF DBR flooding \
  --nodes 100 \
  --runs 15

# Analyze
python3 analyze_results.py ../results/uwsn_results_*.csv

# Plot
python3 plot_results.py ../results/uwsn_results_*.csv --format png
```

### Scalability Test

```bash
python3 run_experiments.py \
  --protocols VBF \
  --nodes 50 100 150 200 \
  --runs 10
```

---

## Summary

| Component | Status |
|-----------|--------|
| CBR traffic option | ✅ Working |
| End-to-end PDR | ✅ Working |
| Energy tracking | ✅ Working |
| Command line args | ✅ Working |
| Batch automation | ✅ Working |
| Statistical analysis | ✅ Working |
| Graph generation | ✅ Working |
| VBF protocol | ✅ Working |
| DBR protocol | ✅ Working |
| Flooding protocol | ✅ Working |

---

## Tags

#ns3 #uwsn #simulation #aqua-sim #research
