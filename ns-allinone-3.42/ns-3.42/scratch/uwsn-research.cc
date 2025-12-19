/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * UWSN Protocol Comparison - Enhanced Version
 * - Automatic PDR tracking
 * - End-to-End delay measurement
 * - Throughput calculation
 * - Network lifetime monitoring
 * - Complete CSV output for analysis
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/tag.h"
#include <fstream>
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>
#include <limits>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UwsnProtocolComparison");

// ============================================================================
// Custom Timestamp Tag for End-to-End Delay Measurement
// ============================================================================
class UwsnTimestampTag : public Tag
{
public:
  static TypeId GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::UwsnTimestampTag")
      .SetParent<Tag>()
      .AddConstructor<UwsnTimestampTag>();
    return tid;
  }

  virtual TypeId GetInstanceTypeId(void) const { return GetTypeId(); }
  virtual uint32_t GetSerializedSize(void) const { return 8; }

  virtual void Serialize(TagBuffer i) const
  {
    i.WriteU64(m_timestamp.GetNanoSeconds());
  }

  virtual void Deserialize(TagBuffer i)
  {
    m_timestamp = NanoSeconds(i.ReadU64());
  }

  virtual void Print(std::ostream &os) const
  {
    os << "Timestamp=" << m_timestamp.GetSeconds();
  }

  void SetTimestamp(Time t) { m_timestamp = t; }
  Time GetTimestamp(void) const { return m_timestamp; }

private:
  Time m_timestamp;
};

// ============================================================================
// Global Metrics Tracking
// ============================================================================
uint64_t g_packetsSent = 0;
uint64_t g_packetsReceived = 0;
uint64_t g_bytesReceived = 0;
std::vector<double> g_delays;  // Store all delays for statistics
double g_networkLifetime = -1.0;  // Time when first node dies
bool g_firstNodeDied = false;

// Store device container globally for energy checking
NetDeviceContainer* g_devicesPtr = nullptr;
double g_initialEnergyPerNode = 0.0;

// ============================================================================
// Callbacks
// ============================================================================
void PacketSentCallback(Ptr<const Packet> packet)
{
  g_packetsSent++;
}

void PacketReceivedCallback(Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom(from)))
  {
    g_packetsReceived++;
    g_bytesReceived += packet->GetSize();

    // Extract timestamp and compute delay
    UwsnTimestampTag tag;
    if (packet->PeekPacketTag(tag))
    {
      Time sendTime = tag.GetTimestamp();
      Time delay = Simulator::Now() - sendTime;
      g_delays.push_back(delay.GetMilliSeconds());
    }
  }
}

void CheckNetworkLifetime(double simStop)
{
  if (g_firstNodeDied || g_devicesPtr == nullptr)
    return;

  for (uint32_t i = 0; i < g_devicesPtr->GetN(); i++)
  {
    Ptr<AquaSimNetDevice> dev = DynamicCast<AquaSimNetDevice>(g_devicesPtr->Get(i));
    if (dev && dev->EnergyModel())
    {
      double energy = dev->EnergyModel()->GetEnergy();
      if (energy <= 0.0)
      {
        g_networkLifetime = Simulator::Now().GetSeconds();
        g_firstNodeDied = true;
        NS_LOG_INFO("First node died at " << g_networkLifetime << " seconds");
        return;
      }
    }
  }

  // Schedule next check (every 1 second)
  if (Simulator::Now().GetSeconds() < simStop)
  {
    Simulator::Schedule(Seconds(1.0), &CheckNetworkLifetime, simStop);
  }
}

// ============================================================================
// Custom OnOff Application with Timestamp Tagging
// ============================================================================
class TimestampOnOffApplication : public Application
{
public:
  static TypeId GetTypeId(void)
  {
    static TypeId tid = TypeId("ns3::TimestampOnOffApplication")
      .SetParent<Application>()
      .AddConstructor<TimestampOnOffApplication>()
      .AddAttribute("DataRate", "Data rate",
                    DataRateValue(DataRate("500kb/s")),
                    MakeDataRateAccessor(&TimestampOnOffApplication::m_dataRate),
                    MakeDataRateChecker())
      .AddAttribute("PacketSize", "Packet size in bytes",
                    UintegerValue(512),
                    MakeUintegerAccessor(&TimestampOnOffApplication::m_packetSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Remote", "Remote address",
                    AddressValue(),
                    MakeAddressAccessor(&TimestampOnOffApplication::m_peer),
                    MakeAddressChecker())
      .AddAttribute("OnTime", "On time",
                    StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                    MakePointerAccessor(&TimestampOnOffApplication::m_onTime),
                    MakePointerChecker<RandomVariableStream>())
      .AddAttribute("OffTime", "Off time",
                    StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                    MakePointerAccessor(&TimestampOnOffApplication::m_offTime),
                    MakePointerChecker<RandomVariableStream>());
    return tid;
  }

  TimestampOnOffApplication() : m_socket(0), m_running(false) {}
  virtual ~TimestampOnOffApplication() { m_socket = 0; }

  void Setup(Address address, uint32_t packetSize, DataRate dataRate)
  {
    m_peer = address;
    m_packetSize = packetSize;
    m_dataRate = dataRate;
  }

protected:
  virtual void StartApplication(void)
  {
    m_running = true;
    if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName("ns3::PacketSocketFactory");
      m_socket = Socket::CreateSocket(GetNode(), tid);
      m_socket->Bind();
      m_socket->Connect(m_peer);
    }
    ScheduleNextTx();
  }

  virtual void StopApplication(void)
  {
    m_running = false;
    if (m_sendEvent.IsPending())
    {
      Simulator::Cancel(m_sendEvent);
    }
    if (m_socket)
    {
      m_socket->Close();
    }
  }

private:
  void SendPacket(void)
  {
    Ptr<Packet> packet = Create<Packet>(m_packetSize);

    // Add timestamp tag
    UwsnTimestampTag tag;
    tag.SetTimestamp(Simulator::Now());
    packet->AddPacketTag(tag);

    m_socket->Send(packet);
    g_packetsSent++;

    ScheduleNextTx();
  }

  void ScheduleNextTx(void)
  {
    if (m_running)
    {
      // Calculate time for next packet based on data rate
      Time nextTime = Seconds(m_packetSize * 8.0 / m_dataRate.GetBitRate());
      // Add some randomness based on on/off times
      double onTime = m_onTime->GetValue();
      double offTime = m_offTime->GetValue();
      if (onTime > 0)
      {
        nextTime = Seconds(onTime + offTime);
      }
      m_sendEvent = Simulator::Schedule(nextTime, &TimestampOnOffApplication::SendPacket, this);
    }
  }

  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  DataRate m_dataRate;
  EventId m_sendEvent;
  bool m_running;
  Ptr<RandomVariableStream> m_onTime;
  Ptr<RandomVariableStream> m_offTime;
};

// ============================================================================
// Main Simulation
// ============================================================================
int
main(int argc, char *argv[])
{
  // Parameters
  double simStop = 1000;
  int nodes = 100;
  int sinks = 1;
  uint32_t m_dataRate = 10000;
  uint32_t m_packetSize = 40;
  double range = 100;
  std::string protocol = "VBF";
  std::string csvFile = "uwsn-results.csv";
  double initialEnergyJ = 500.0;
  double txPowerW = 2.0;
  double rxPowerW = 0.1;
  double idlePowerW = 0.001;
  double areaX = 400.0;
  double areaY = 400.0;
  double minDepth = 0.0;
  double maxDepth = 400.0;
  double sinkX = 190.0;
  double sinkY = 190.0;
  double sinkZ = 0.0;
  double senderX = 10.0;
  double senderY = 10.0;
  double senderZ = 50.0;
  uint32_t runNumber = 1;

  // Parse command line
  CommandLine cmd;
  cmd.AddValue("protocol", "Protocol: VBF, DBR, flooding", protocol);
  cmd.AddValue("runNumber", "Run number for random seed", runNumber);
  cmd.AddValue("nodes", "Number of intermediate nodes", nodes);
  cmd.AddValue("simStop", "Simulation duration in seconds", simStop);
  cmd.AddValue("csvFile", "Output CSV file name", csvFile);
  cmd.AddValue("range", "Transmission range", range);
  cmd.AddValue("areaX", "Area X dimension", areaX);
  cmd.AddValue("areaY", "Area Y dimension", areaY);
  cmd.AddValue("maxDepth", "Maximum depth", maxDepth);
  cmd.AddValue("dataRate", "CBR data rate in bits per second", m_dataRate);
  cmd.AddValue("packetSize", "Packet size in bytes", m_packetSize);
  cmd.Parse(argc, argv);

  // Set random seed based on run number
  RngSeedManager::SetSeed(12345);
  RngSeedManager::SetRun(runNumber);

  // Store for energy checking
  g_initialEnergyPerNode = initialEnergyJ;

  std::cout << "\n========================================\n";
  std::cout << "   UWSN PROTOCOL COMPARISON\n";
  std::cout << "========================================\n";
  std::cout << "Protocol: " << protocol << " | Run: " << runNumber << "\n";
  std::cout << "Nodes: " << nodes << ", Sinks: " << sinks << "\n";
  std::cout << "Area: " << areaX << "x" << areaY << "x[" << minDepth << "," << maxDepth << "]\n";
  std::cout << "Sink: (" << sinkX << "," << sinkY << "," << sinkZ << ")\n";
  std::cout << "Sender: (" << senderX << "," << senderY << "," << senderZ << ")\n";
  std::cout << "Simulation Time: " << simStop << "s\n";
  std::cout << "CBR Data Rate: " << m_dataRate << " bps\n";
  std::cout << "Packet Size: " << m_packetSize << " bytes\n";
  std::cout << "========================================\n";

  // Reset global counters
  g_packetsSent = 0;
  g_packetsReceived = 0;
  g_bytesReceived = 0;
  g_delays.clear();
  g_networkLifetime = -1.0;
  g_firstNodeDied = false;

  // Create nodes
  NodeContainer nodesCon, sinksCon, senderCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);
  senderCon.Create(1);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);
  socketHelper.Install(senderCon);

  // Aqua-Sim setup
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetPhy("ns3::AquaSimPhyCmn");
  asHelper.SetMac("ns3::AquaSimBroadcastMac");

  // Protocol setup
  if (protocol == "VBF") {
    asHelper.SetRouting("ns3::AquaSimVBF", "Width", DoubleValue(100),
                        "TargetPos", Vector3DValue(Vector(sinkX, sinkY, sinkZ)));
  } else if (protocol == "DBR") {
    asHelper.SetRouting("ns3::AquaSimDBR");
  } else if (protocol == "flooding") {
    asHelper.SetRouting("ns3::AquaSimFloodingRouting");
  } else {
    NS_FATAL_ERROR("Unsupported protocol: " << protocol);
  }

  // Mobility setup with random positions
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
  std::mt19937 gen(12345 + runNumber);
  std::uniform_real_distribution<> disX(0.0, areaX);
  std::uniform_real_distribution<> disY(0.0, areaY);
  std::uniform_real_distribution<> disZ(minDepth, maxDepth);

  for (int i = 0; i < nodes; ++i) {
    position->Add(Vector(disX(gen), disY(gen), disZ(gen)));
  }
  position->Add(Vector(sinkX, sinkY, sinkZ));
  position->Add(Vector(senderX, senderY, senderZ));

  MobilityHelper mobility;
  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  // Device + Energy setup
  NetDeviceContainer devices;
  auto createDevice = [&](NodeContainer &nc) {
    for (NodeContainer::Iterator i = nc.Begin(); i != nc.End(); ++i) {
      Ptr<AquaSimNetDevice> dev = CreateObject<AquaSimNetDevice>();
      Ptr<AquaSimEnergyModel> em = CreateObject<AquaSimEnergyModel>();
      em->SetInitialEnergy(initialEnergyJ);
      em->SetTxPower(txPowerW);
      em->SetRxPower(rxPowerW);
      em->SetIdlePower(idlePowerW);
      em->SetDevice(dev);
      (*i)->AggregateObject(em);
      dev->SetEnergyModel(em);
      devices.Add(asHelper.Create(*i, dev));
      dev->GetPhy()->SetTransRange(range);
    }
  };
  createDevice(nodesCon);
  createDevice(sinksCon);
  createDevice(senderCon);

  // Store devices pointer for energy checking
  g_devicesPtr = &devices;

  mobility.Install(nodesCon);
  mobility.Install(sinksCon);
  mobility.Install(senderCon);

  // Application setup
  PacketSocketAddress socket;
  socket.SetAllDevices();
  socket.SetPhysicalAddress(devices.Get(nodes)->GetAddress());
  socket.SetProtocol(0);

  // Use CBR (Constant Bit Rate) traffic generator
  // SetConstantRate sets OnTime=1000s, OffTime=0s for continuous transmission
  OnOffHelper appHelper("ns3::PacketSocketFactory", Address(socket));
  appHelper.SetConstantRate(DataRate(m_dataRate), m_packetSize);

  ApplicationContainer apps = appHelper.Install(senderCon);
  apps.Start(Seconds(0.5));
  apps.Stop(Seconds(simStop));

  // Setup sink socket for receiving
  Ptr<Node> sinkNode = sinksCon.Get(0);
  TypeId psfid = TypeId::LookupByName("ns3::PacketSocketFactory");
  Ptr<Socket> sinkSocket = Socket::CreateSocket(sinkNode, psfid);
  sinkSocket->Bind(socket);
  sinkSocket->SetRecvCallback(MakeCallback(&PacketReceivedCallback));

  // Schedule network lifetime checking
  Simulator::Schedule(Seconds(1.0), &CheckNetworkLifetime, simStop);

  // Run simulation
  std::cout << "\nRunning Simulation...\n";
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();

  // Get channel counters for PDR
  Ptr<AquaSimChannel> asChannel = asHelper.GetChannel();
  asChannel->PrintCounters();

  // Get packet counts from channel (network-level statistics)
  int channelSentPkts = asChannel->GetTotalSentPackets();
  int channelRecvPkts = asChannel->GetTotalReceivedPackets();

  // If network lifetime was never set (no node died), set to simStop
  if (g_networkLifetime < 0) {
    g_networkLifetime = simStop;
  }

  // Energy calculation
  double totalInitialEnergy = 0.0;
  double totalResidualEnergy = 0.0;
  double minResidualEnergy = std::numeric_limits<double>::max();
  double maxResidualEnergy = 0.0;
  int deadNodes = 0;

  for (uint32_t i = 0; i < devices.GetN(); i++) {
    Ptr<AquaSimNetDevice> dev = DynamicCast<AquaSimNetDevice>(devices.Get(i));
    if (dev && dev->EnergyModel()) {
      Ptr<AquaSimEnergyModel> em = dev->EnergyModel();
      double initE = em->GetInitialEnergy();
      double resE = em->GetEnergy();
      totalInitialEnergy += initE;
      totalResidualEnergy += resE;
      if (resE < minResidualEnergy) minResidualEnergy = resE;
      if (resE > maxResidualEnergy) maxResidualEnergy = resE;
      if (resE <= 0) deadNodes++;
    }
  }
  double totalConsumedEnergy = totalInitialEnergy - totalResidualEnergy;
  double avgEnergyPerNode = totalConsumedEnergy / devices.GetN();

  // Compute delay statistics
  double avgDelay = 0.0, minDelay = 0.0, maxDelay = 0.0, stdDelay = 0.0;
  if (!g_delays.empty()) {
    avgDelay = std::accumulate(g_delays.begin(), g_delays.end(), 0.0) / g_delays.size();
    minDelay = *std::min_element(g_delays.begin(), g_delays.end());
    maxDelay = *std::max_element(g_delays.begin(), g_delays.end());

    // Standard deviation
    double sq_sum = 0.0;
    for (double d : g_delays) {
      sq_sum += (d - avgDelay) * (d - avgDelay);
    }
    stdDelay = std::sqrt(sq_sum / g_delays.size());
  }

  // Compute PDR and throughput using channel counters
  double pdr = (channelSentPkts > 0) ? (double)channelRecvPkts / channelSentPkts : 0.0;
  double throughput = (simStop > 0) ? (channelRecvPkts * m_packetSize * 8.0) / simStop : 0.0;  // bits per second
  double energyEfficiency = (totalConsumedEnergy > 0) ? channelRecvPkts / totalConsumedEnergy : 0.0;

  Simulator::Destroy();
  g_devicesPtr = nullptr;

  // Print results
  std::cout << "\n========================================\n";
  std::cout << "   SIMULATION RESULTS\n";
  std::cout << "========================================\n";
  std::cout << "Packets Sent: " << channelSentPkts << "\n";
  std::cout << "Packets Received: " << channelRecvPkts << "\n";
  std::cout << "PDR: " << (pdr * 100.0) << "%\n";
  std::cout << "Throughput: " << throughput << " bps\n";
  std::cout << "Avg Delay: " << avgDelay << " ms\n";
  std::cout << "Min Delay: " << minDelay << " ms\n";
  std::cout << "Max Delay: " << maxDelay << " ms\n";
  std::cout << "Std Delay: " << stdDelay << " ms\n";
  std::cout << "Network Lifetime: " << g_networkLifetime << " s\n";
  std::cout << "Dead Nodes: " << deadNodes << "\n";
  std::cout << "Total Initial Energy: " << totalInitialEnergy << " J\n";
  std::cout << "Total Consumed Energy: " << totalConsumedEnergy << " J\n";
  std::cout << "Total Residual Energy: " << totalResidualEnergy << " J\n";
  std::cout << "Avg Energy/Node: " << avgEnergyPerNode << " J\n";
  std::cout << "Energy Efficiency: " << energyEfficiency << " pkts/J\n";
  std::cout << "========================================\n";

  // CSV output
  bool writeHeader = false;
  {
    std::ifstream test(csvFile);
    if (!test.good()) writeHeader = true;
    test.close();
  }

  std::ofstream csv(csvFile, std::ios::app);
  if (writeHeader) {
    csv << "run,protocol,nodes,sinks,simStop,range,"
        << "areaX,areaY,minDepth,maxDepth,"
        << "data_rate_bps,packet_size_bytes,"
        << "packets_sent,packets_received,pdr,"
        << "throughput_bps,avg_delay_ms,min_delay_ms,max_delay_ms,std_delay_ms,"
        << "network_lifetime_s,dead_nodes,"
        << "initial_energy_per_node,tx_power,rx_power,idle_power,"
        << "total_initial_energy,total_consumed_energy,total_residual_energy,"
        << "avg_energy_per_node,energy_efficiency_pkts_per_j\n";
  }

  csv << runNumber << ","
      << protocol << ","
      << nodes << ","
      << sinks << ","
      << simStop << ","
      << range << ","
      << areaX << ","
      << areaY << ","
      << minDepth << ","
      << maxDepth << ","
      << m_dataRate << ","
      << m_packetSize << ","
      << channelSentPkts << ","
      << channelRecvPkts << ","
      << pdr << ","
      << throughput << ","
      << avgDelay << ","
      << minDelay << ","
      << maxDelay << ","
      << stdDelay << ","
      << g_networkLifetime << ","
      << deadNodes << ","
      << initialEnergyJ << ","
      << txPowerW << ","
      << rxPowerW << ","
      << idlePowerW << ","
      << totalInitialEnergy << ","
      << totalConsumedEnergy << ","
      << totalResidualEnergy << ","
      << avgEnergyPerNode << ","
      << energyEfficiency << "\n";
  csv.close();

  std::cout << "\nResults appended to: " << csvFile << "\n";
  std::cout << "fin.\n";

  return 0;
}
