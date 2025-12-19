/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * UWSN Protocol Comparison - Research Version
 * Based on original working version with enhanced parameters
 * 
 * Metrics tracked:
 * - Energy consumption (per node, total)
 * - Network-level packet statistics via PrintCounters()
 * - Network lifetime (time until first node dies)
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/energy-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include <fstream>
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UwsnProtocolComparison");

// Global variables for network lifetime tracking
double g_networkLifetime = -1.0;
bool g_firstNodeDied = false;
NetDeviceContainer* g_devicesPtr = nullptr;

void CheckNetworkLifetime(double simStop)
{
  if (g_firstNodeDied || g_devicesPtr == nullptr)
    return;

  int deadCount = 0;
  for (uint32_t i = 0; i < g_devicesPtr->GetN(); i++)
  {
    Ptr<AquaSimNetDevice> dev = DynamicCast<AquaSimNetDevice>(g_devicesPtr->Get(i));
    if (dev && dev->EnergyModel())
    {
      double energy = dev->EnergyModel()->GetEnergy();
      if (energy <= 0.0)
      {
        if (!g_firstNodeDied)
        {
          g_networkLifetime = Simulator::Now().GetSeconds();
          g_firstNodeDied = true;
          std::cout << "[LIFETIME] First node died at " << g_networkLifetime << " s\n";
        }
        deadCount++;
      }
    }
  }

  // Schedule next check
  if (Simulator::Now().GetSeconds() < simStop - 1.0)
  {
    Simulator::Schedule(Seconds(1.0), &CheckNetworkLifetime, simStop);
  }
}

int
main(int argc, char *argv[])
{
  // ========================================
  // Default Parameters
  // ========================================
  double simStop = 1000;
  int nodes = 100;
  int sinks = 1;
  uint32_t m_dataRate = 10000;  // bits per second
  uint32_t m_packetSize = 40;   // bytes
  double range = 100;
  std::string protocol = "VBF";
  std::string csvFile = "uwsn-results.csv";
  
  // Energy parameters
  double initialEnergyJ = 500.0;
  double txPowerW = 2.0;
  double rxPowerW = 0.1;
  double idlePowerW = 0.001;
  
  // Topology parameters
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

  // ========================================
  // Parse Command Line
  // ========================================
  CommandLine cmd;
  cmd.AddValue("protocol", "Protocol: VBF, DBR, flooding", protocol);
  cmd.AddValue("runNumber", "Run number for random seed", runNumber);
  cmd.AddValue("nodes", "Number of intermediate nodes", nodes);
  cmd.AddValue("simStop", "Simulation duration in seconds", simStop);
  cmd.AddValue("csvFile", "Output CSV file name", csvFile);
  cmd.AddValue("range", "Transmission range in meters", range);
  cmd.AddValue("areaX", "Area X dimension", areaX);
  cmd.AddValue("areaY", "Area Y dimension", areaY);
  cmd.AddValue("maxDepth", "Maximum depth", maxDepth);
  cmd.AddValue("dataRate", "Data rate in bits per second", m_dataRate);
  cmd.AddValue("packetSize", "Packet size in bytes", m_packetSize);
  cmd.AddValue("sinkX", "Sink X position", sinkX);
  cmd.AddValue("sinkY", "Sink Y position", sinkY);
  cmd.AddValue("senderX", "Sender X position", senderX);
  cmd.AddValue("senderY", "Sender Y position", senderY);
  cmd.AddValue("senderZ", "Sender Z (depth) position", senderZ);
  cmd.Parse(argc, argv);

  // Set random seed
  RngSeedManager::SetRun(runNumber);

  // ========================================
  // Print Configuration
  // ========================================
  std::cout << "\n========================================\n";
  std::cout << "   UWSN PROTOCOL COMPARISON\n";
  std::cout << "========================================\n";
  std::cout << "Protocol: " << protocol << " | Run: " << runNumber << "\n";
  std::cout << "Nodes: " << nodes << ", Sinks: " << sinks << "\n";
  std::cout << "Area: " << areaX << "x" << areaY << "x[" << minDepth << "," << maxDepth << "]\n";
  std::cout << "Range: " << range << " m\n";
  std::cout << "Sink: (" << sinkX << "," << sinkY << "," << sinkZ << ")\n";
  std::cout << "Sender: (" << senderX << "," << senderY << "," << senderZ << ")\n";
  std::cout << "Sim Time: " << simStop << " s\n";
  std::cout << "Data Rate: " << m_dataRate << " bps, Packet: " << m_packetSize << " bytes\n";
  std::cout << "========================================\n";

  // Reset global variables
  g_networkLifetime = -1.0;
  g_firstNodeDied = false;

  // ========================================
  // Create Nodes
  // ========================================
  NodeContainer nodesCon, sinksCon, senderCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);
  senderCon.Create(1);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);
  socketHelper.Install(senderCon);

  // ========================================
  // Aqua-Sim Setup
  // ========================================
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetPhy("ns3::AquaSimPhyCmn");
  asHelper.SetMac("ns3::AquaSimBroadcastMac");

  // ========================================
  // Protocol Setup
  // ========================================
  if (protocol == "VBF") {
    asHelper.SetRouting("ns3::AquaSimVBF", 
                        "Width", DoubleValue(100),
                        "TargetPos", Vector3DValue(Vector(sinkX, sinkY, sinkZ)));
  } else if (protocol == "DBR") {
    asHelper.SetRouting("ns3::AquaSimDBR");
  } else if (protocol == "flooding") {
    asHelper.SetRouting("ns3::AquaSimFloodingRouting");
  } else {
    NS_FATAL_ERROR("Unsupported protocol: " << protocol);
  }

  // ========================================
  // Mobility Setup
  // ========================================
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
  std::mt19937 gen(12345 + runNumber);
  std::uniform_real_distribution<> disX(0.0, areaX);
  std::uniform_real_distribution<> disY(0.0, areaY);
  std::uniform_real_distribution<> disZ(minDepth, maxDepth);

  // Add intermediate nodes at random positions
  for (int i = 0; i < nodes; ++i) {
    position->Add(Vector(disX(gen), disY(gen), disZ(gen)));
  }
  // Add sink position
  position->Add(Vector(sinkX, sinkY, sinkZ));
  // Add sender position
  position->Add(Vector(senderX, senderY, senderZ));

  MobilityHelper mobility;
  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  // ========================================
  // Device + Energy Setup
  // ========================================
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

  // Store for lifetime checking
  g_devicesPtr = &devices;

  mobility.Install(nodesCon);
  mobility.Install(sinksCon);
  mobility.Install(senderCon);

  // ========================================
  // Application Setup (OnOff Traffic)
  // ========================================
  PacketSocketAddress socket;
  socket.SetAllDevices();
  socket.SetPhysicalAddress(devices.Get(nodes)->GetAddress());  // Sink address
  socket.SetProtocol(0);

  OnOffHelper app("ns3::PacketSocketFactory", Address(socket));
  // Original timing: ~0.66% duty cycle
  app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0066]"));
  app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.9934]"));
  app.SetAttribute("DataRate", DataRateValue(m_dataRate));
  app.SetAttribute("PacketSize", UintegerValue(m_packetSize));

  ApplicationContainer apps = app.Install(senderCon);
  apps.Start(Seconds(0.5));
  apps.Stop(Seconds(simStop));

  // Schedule network lifetime checking
  Simulator::Schedule(Seconds(1.0), &CheckNetworkLifetime, simStop);

  // ========================================
  // Run Simulation
  // ========================================
  std::cout << "\nRunning Simulation...\n";
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();

  // ========================================
  // Print Channel Statistics (PDR source)
  // ========================================
  std::cout << "\n--- Channel Statistics ---\n";
  asHelper.GetChannel()->PrintCounters();
  std::cout << "--- End Channel Statistics ---\n";

  // ========================================
  // Calculate Energy Metrics
  // ========================================
  double totalInitialEnergy = 0.0;
  double totalResidualEnergy = 0.0;
  int deadNodes = 0;

  for (uint32_t i = 0; i < devices.GetN(); i++) {
    Ptr<AquaSimNetDevice> dev = DynamicCast<AquaSimNetDevice>(devices.Get(i));
    if (dev && dev->EnergyModel()) {
      Ptr<AquaSimEnergyModel> em = dev->EnergyModel();
      totalInitialEnergy += em->GetInitialEnergy();
      totalResidualEnergy += em->GetEnergy();
      if (em->GetEnergy() <= 0) {
        deadNodes++;
      }
    }
  }
  double totalConsumedEnergy = totalInitialEnergy - totalResidualEnergy;
  double avgEnergyPerNode = totalConsumedEnergy / devices.GetN();

  // If no node died, set lifetime to simulation time
  if (g_networkLifetime < 0) {
    g_networkLifetime = simStop;
  }

  Simulator::Destroy();
  g_devicesPtr = nullptr;

  // ========================================
  // Print Results Summary
  // ========================================
  std::cout << "\n========================================\n";
  std::cout << "   SIMULATION RESULTS\n";
  std::cout << "========================================\n";
  std::cout << "Network Lifetime: " << g_networkLifetime << " s\n";
  std::cout << "Dead Nodes: " << deadNodes << " / " << devices.GetN() << "\n";
  std::cout << "Total Initial Energy: " << totalInitialEnergy << " J\n";
  std::cout << "Total Consumed Energy: " << totalConsumedEnergy << " J\n";
  std::cout << "Total Residual Energy: " << totalResidualEnergy << " J\n";
  std::cout << "Avg Energy/Node: " << avgEnergyPerNode << " J\n";
  std::cout << "========================================\n";
  std::cout << "NOTE: Use 'Sent Pkts' and 'Recv Pkts' from\n";
  std::cout << "      Channel Statistics above to compute PDR.\n";
  std::cout << "========================================\n";

  // ========================================
  // CSV Output
  // ========================================
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
        << "network_lifetime_s,dead_nodes,"
        << "initial_energy_per_node,tx_power,rx_power,idle_power,"
        << "total_initial_energy,total_consumed_energy,total_residual_energy,"
        << "avg_energy_per_node\n";
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
      << g_networkLifetime << ","
      << deadNodes << ","
      << initialEnergyJ << ","
      << txPowerW << ","
      << rxPowerW << ","
      << idlePowerW << ","
      << totalInitialEnergy << ","
      << totalConsumedEnergy << ","
      << totalResidualEnergy << ","
      << avgEnergyPerNode << "\n";
  csv.close();

  std::cout << "\nResults appended to: " << csvFile << "\n";
  std::cout << "fin.\n";

  return 0;
}
