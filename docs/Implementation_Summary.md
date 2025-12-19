# UWSN Simulation Framework - Implementation Summary

> **Project**: Underwater Wireless Sensor Network (UWSN) Protocol Comparison
> **Base**: ns-3.42 + Aqua-Sim-NG
> **Date**: December 2024

---

## Table of Contents

- [[#Acronyms and Definitions]]
- [[#1. Original State]]
- [[#2. Enhanced Metrics Collection]]
- [[#3. CBR Traffic Generation]]
- [[#4. Command-Line Parameters]]
- [[#5. Automation Scripts]]
- [[#6. Aqua-Sim-NG Patches]]
- [[#7. Project Organization]]
- [[#8. Topology Configuration]]
- [[#Summary]]

---

## Acronyms and Definitions

| Acronym | Full Name | Description |
|---------|-----------|-------------|
| **UWSN** | Underwater Wireless Sensor Network | Network of underwater sensor nodes communicating via acoustic signals |
| **PDR** | Packet Delivery Ratio | Ratio of packets successfully received to packets sent |
| **E2E Delay** | End-to-End Delay | Time from packet transmission to reception at sink |
| **CBR** | Constant Bit Rate | Traffic pattern with fixed data rate |
| **VBF** | Vector-Based Forwarding | Routing protocol using virtual pipeline toward sink |
| **DBR** | Depth-Based Routing | Routing protocol forwarding packets toward surface |
| **MAC** | Medium Access Control | Protocol controlling access to shared medium |
| **PHY** | Physical Layer | Lowest layer handling signal transmission |
| **CI** | Confidence Interval | Statistical range indicating reliability of estimate |

---

## 1. Original State

### What Was Provided

The original `uwsn-research.cc` simulation:

- [x] Basic UWSN setup using Aqua-Sim-NG module
- [x] Random 3D node deployment
- [x] VBF routing protocol
- [x] Energy consumption tracking
- [x] Single run execution

### Limitations

- [ ] No PDR calculation
- [ ] No E2E delay measurement
- [ ] No throughput calculation
- [ ] No network lifetime tracking
- [ ] Manual execution required for each run
- [ ] No statistical analysis
- [ ] No automated graph generation

---

## 2. Enhanced Metrics Collection

### 2.1 PDR (Packet Delivery Ratio)

**Implementation**: Utilized `AquaSimChannel` internal counters for accurate network-wide PDR.

```cpp
uint64_t totalSent = channel->GetTotalSentPackets();
uint64_t totalReceived = channel->GetTotalReceivedPackets();
double pdr = (totalSent > 0) ? (double)totalReceived / totalSent : 0.0;
```

> [!info] Why Channel Counters?
> Application-level counters miss packets lost at MAC/PHY layers. Channel counters capture all transmissions and receptions in the network.

---

### 2.2 E2E Delay (End-to-End Delay)

**Implementation**: Custom packet tag `UwsnTimestampTag` to timestamp packets at transmission.

```cpp
class UwsnTimestampTag : public Tag {
private:
  Time m_timestamp;
  
public:
  void SetTimestamp(Time t) { m_timestamp = t; }
  Time GetTimestamp() const { return m_timestamp; }
  
  // Required Tag interface methods
  static TypeId GetTypeId();
  void Serialize(TagBuffer i) const;
  void Deserialize(TagBuffer i);
};
```

**Metrics Collected**:
- `avg_delay_ms` - Mean E2E delay
- `min_delay_ms` - Minimum delay observed
- `max_delay_ms` - Maximum delay observed  
- `std_delay_ms` - Standard deviation of delay

---

### 2.3 Throughput

**Implementation**: Bytes received divided by simulation time.

```cpp
double throughput_bps = (g_bytesReceived * 8.0) / simStop;
```

---

### 2.4 Network Lifetime

**Implementation**: Scheduled function checks node energy every second.

```cpp
void CheckNetworkLifetime(double currentTime) {
  for (uint32_t i = 0; i < g_devicesPtr->GetN(); i++) {
    Ptr<AquaSimNetDevice> device = 
      DynamicCast<AquaSimNetDevice>(g_devicesPtr->Get(i));
    
    if (device->EnergyModel()->GetEnergy() <= 0) {
      if (!g_firstNodeDied) {
        g_networkLifetime = currentTime;
        g_firstNodeDied = true;
      }
      g_deadNodes++;
    }
  }
}
```

**Metrics**:
- `network_lifetime_s` - Time until first node dies
- `dead_nodes` - Total nodes depleted at simulation end

---

### 2.5 Energy Efficiency

**Implementation**: Packets delivered per Joule of energy consumed.

```cpp
double energy_efficiency = packets_received / total_consumed_energy;
// Unit: packets/Joule
```

---

## 3. CBR Traffic Generation

### Problem with OnOff Application

Original OnOff application alternates between sending and idle states with configurable durations. This creates **variable rate traffic**, not suitable for controlled experiments.

### Solution: SetConstantRate()

```cpp
OnOffHelper appHelper("ns3::PacketSocketFactory", Address(socket));
appHelper.SetConstantRate(DataRate(m_dataRate), m_packetSize);
```

> [!tip] How SetConstantRate Works
> Internally sets:
> - `OnTime = ConstantRandomVariable(1000s)`
> - `OffTime = ConstantRandomVariable(0s)`
> 
> Result: Continuous transmission at specified rate.

### Configurable Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `dataRate` | uint32_t | 1000 bps | CBR transmission rate |
| `packetSize` | uint32_t | 50 bytes | Size of each packet |

---

## 4. Command-Line Parameters

All simulation parameters configurable without recompilation:

```bash
./ns3 run "uwsn-research \
  --protocol=VBF \
  --nodes=100 \
  --simStop=500 \
  --range=150 \
  --dataRate=1000 \
  --packetSize=50 \
  --runNumber=1 \
  --csvFile=results.csv"
```

### Parameter Reference

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `protocol` | string | VBF | Routing: VBF, DBR, flooding |
| `nodes` | int | 100 | Number of sensor nodes |
| `simStop` | double | 500 | Simulation duration (s) |
| `range` | double | 150 | Transmission range (m) |
| `dataRate` | uint32_t | 1000 | CBR rate (bps) |
| `packetSize` | uint32_t | 50 | Packet size (bytes) |
| `runNumber` | uint32_t | 1 | Random seed |
| `csvFile` | string | uwsn-results.csv | Output file |

---

## 5. Automation Scripts

### 5.1 run_experiments.py

**Purpose**: Batch execution of multiple simulation runs.

```bash
python3 run_experiments.py \
  --protocols VBF DBR flooding \
  --nodes 50 100 150 200 \
  --runs 30 \
  --simtime 500
```

**Features**:
- [x] Parallel execution via `multiprocessing`
- [x] Progress bar with ETA
- [x] Automatic CSV aggregation
- [x] Configurable parameters

**Calculation**:
$$\text{Total Experiments} = |\text{Protocols}| \times |\text{Node Counts}| \times \text{Runs}$$

Example: $3 \times 4 \times 30 = 360$ experiments

---

### 5.2 analyze_results.py

**Purpose**: Statistical analysis of simulation results.

```bash
python3 analyze_results.py results/uwsn_results.csv
```

**Statistics Computed**:

| Statistic | Formula |
|-----------|---------|
| Mean | $\bar{x} = \frac{1}{n}\sum_{i=1}^{n} x_i$ |
| Std Dev | $\sigma = \sqrt{\frac{1}{n-1}\sum_{i=1}^{n}(x_i - \bar{x})^2}$ |
| 95% CI | $\bar{x} \pm 1.96 \times \frac{\sigma}{\sqrt{n}}$ |

**Output**:
```
Protocol: VBF (n=30 runs)
  PDR: 78.5% ± 3.2% (95% CI: [75.3%, 81.7%])
  E2E Delay: 125.3 ms ± 15.7 ms
  Throughput: 856.2 bps ± 42.1 bps
```

---

### 5.3 plot_results.py

**Purpose**: Generate publication-quality graphs.

```bash
python3 plot_results.py results/uwsn_results.csv --format png
```

**Graph Types**:

| Graph | Description | Use Case |
|-------|-------------|----------|
| `pdr_bar.png` | PDR comparison bar chart | Protocol comparison |
| `pdr_line.png` | PDR vs node count | Scalability analysis |
| `pdr_boxplot.png` | PDR distribution | Variance visualization |
| `delay_cdf.png` | Delay CDF | Delay distribution |
| `energy_line.png` | Energy vs nodes | Energy scalability |
| `network_lifetime.png` | Lifetime comparison | Efficiency analysis |
| `summary_dashboard.png` | 4-panel summary | Paper figures |

---

## 6. Aqua-Sim-NG Patches

### 6.1 Channel Packet Counters

**File**: `aqua-sim-channel.h` / `aqua-sim-channel.cc`

Added getter methods for internal packet counters:

```cpp
// In aqua-sim-channel.h
uint64_t GetTotalSentPackets() const;
uint64_t GetTotalReceivedPackets() const;

// In aqua-sim-channel.cc
uint64_t AquaSimChannel::GetTotalSentPackets() const {
  uint64_t total = 0;
  for (auto& device : m_deviceList) {
    total += device->GetSentPackets();
  }
  return total;
}
```

---

### 6.2 Energy Model Namespace Fix

**File**: `aqua-sim-energy-model.h`

Fixed namespace conflict with ns-3.42's energy module:

```cpp
// Before (caused ambiguity)
Ptr<EnergySource> m_source;

// After (explicit namespace)
Ptr<ns3::energy::EnergySource> m_source;
```

---

## 7. Project Organization

### Directory Structure

```
~/NS3-Research/                 # Git Repository
├── src/
│   └── uwsn-research.cc        # Main simulation
├── examples/
│   └── VBF.cc                  # Reference example
├── scripts/
│   ├── run_experiments.py      # Batch runner
│   ├── analyze_results.py      # Statistics
│   ├── plot_results.py         # Graphs
│   └── requirements.txt        # Python deps
├── results/                    # CSV output
├── graphs/                     # PNG graphs
├── docs/
│   └── Implementation_Summary.md
├── setup.sh                    # Deployment script
├── .gitignore
└── README.md

~/ns3/                          # ns-3 Installation (separate)
└── ns-allinone-3.42/
    └── ns-3.42/
        ├── scratch/            # Simulation copied here
        └── contrib/
            └── aqua-sim-ng/    # Patched module
```

> [!note] Separation Benefits
> - Research code version-controlled independently
> - ns-3 installation not bloating repository
> - Easy to share/replicate on other machines

---

## 8. Topology Configuration

### Problem Identified

Original topology had poor connectivity:

| Issue | Value | Impact |
|-------|-------|--------|
| Sender-Sink Distance | ~260m | Exceeds range |
| Transmission Range | 100m | Too short |
| Volume | 400×400×400m | Sparse deployment |

$$d = \sqrt{(190-10)^2 + (190-10)^2 + (0-50)^2} \approx 259m > 100m$$

### Solution Applied

| Parameter | Before | After |
|-----------|--------|-------|
| Area | 400×400×400m | 200×200×100m |
| Range | 100m | 150m |
| Sender | (10, 10, 50) | (50, 50, 80) |
| Sink | (190, 190, 0) | (100, 100, 0) |
| Data Rate | 10,000 bps | 1,000 bps |

New distance:
$$d = \sqrt{(100-50)^2 + (100-50)^2 + (0-80)^2} \approx 106m < 150m$$

---

## Summary

### Features Comparison

| Feature | Before | After |
|---------|--------|-------|
| **Metrics** | Energy only | PDR, E2E Delay, Throughput, Lifetime, Energy |
| **Traffic** | OnOff (variable) | CBR (constant rate) |
| **Runs** | 1 (manual) | 30-50 (automated) |
| **Analysis** | None | Mean, SD, 95% CI |
| **Graphs** | None | 10+ publication-ready |
| **Protocols** | VBF only | VBF, DBR, Flooding |
| **Config** | Hardcoded | Command-line args |
| **Output** | Console | Structured CSV |

---

### Research Workflow

```mermaid
graph LR
    A[Configure Parameters] --> B[Run Experiments]
    B --> C[Collect CSV Results]
    C --> D[Statistical Analysis]
    D --> E[Generate Graphs]
    E --> F[Publication]
```

---

## Tags

#ns3 #uwsn #simulation #aqua-sim #networking #research

---

> [!quote] Citation
> This framework built on:
> - ns-3.42: https://www.nsnam.org/
> - Aqua-Sim-NG: https://gitlab.com/uw-networks/aqua-sim-ng

