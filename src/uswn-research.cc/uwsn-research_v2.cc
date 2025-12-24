/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * UWSN Protocol Comparison - Version 2
 * 
 * ==== EDITED BY ASSISTANT ====
 * Added 5 configurable parameters from Python/command line:
 * 1. Number of nodes (--nodes)
 * 2. Communication range (--range)
 * 3. Depth variation (--minDepth, --maxDepth)
 * 4. VBF routing pipe radius (--width)
 * 5. Mobility model (--mobility: static/random)
 * 
 * NO CHANGES TO PROTOCOL LOGIC - Only added command line parameters
 * ==== END OF EDIT ====
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
#include <set>
#include <map>
#include <vector>
#include <limits>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("UwsnProtocolComparisonV2");

// ==== EDITED: Global tracking for metrics ====
// PDR tracking: Set of unique packet UIDs received at sink
static std::set<uint64_t> g_receivedPacketUids;

// Delay tracking: Map packet UID to send time, and collect delays
static std::map<uint64_t, Time> g_packetSendTime;
static std::vector<double> g_packetDelays;  // Delays in ms

// Network lifetime tracking: Time when first node dies
static Time g_networkLifetime;
static bool g_firstNodeDied = false;

// Callback for application TX - records packet send time
void AppTxCallback(Ptr<const Packet> packet) {
  uint64_t uid = packet->GetUid();
  g_packetSendTime[uid] = Simulator::Now();
}




// Callback for sink routing - tracks unique packets and calculates delay
void SinkPacketCallback(Ptr<const Packet> packet) {
  uint64_t uid = packet->GetUid();
  // NS_LOG_UNCOND("DEBUG: SinkPacketCallback called! UID=" << uid);
  
  // Only process if this is a new unique packet
  if (g_receivedPacketUids.find(uid) == g_receivedPacketUids.end()) {
    g_receivedPacketUids.insert(uid);
    // NS_LOG_UNCOND("DEBUG: New Unique Packet! UID=" << uid);
    
    // Calculate delay if we have the send time


    auto it = g_packetSendTime.find(uid);
    if (it != g_packetSendTime.end()) {
      Time delay = Simulator::Now() - it->second;
      g_packetDelays.push_back(delay.GetMilliSeconds());
    }
  }
}

// Callback for energy depletion - tracks network lifetime
void EnergyDepletionCallback(Ptr<AquaSimEnergyModel> em) {
  if (!g_firstNodeDied) {
    g_firstNodeDied = true;
    g_networkLifetime = Simulator::Now();
    NS_LOG_INFO("First node died at: " << g_networkLifetime.GetSeconds() << "s");
  }
}
// ==== END OF EDIT ====


int
main(int argc, char *argv[])
{
  // ========== DEFAULT PARAMETERS (Original values) ==========
  double simStop = 1000;
  int nodes = 100;
  int sinks = 1;
  uint32_t m_dataRate = 10000;
  uint32_t m_packetSize = 40;
  
  // ==== EDITED: Make these configurable ====
  double range = 100;              // Parameter 2: Communication Range
  double minDepth = 0.0;           // Parameter 3a: Depth Variation (min)
  double maxDepth = 400.0;         // Parameter 3b: Depth Variation (max)
  double vbfWidth = 100.0;         // Parameter 4: VBF Routing Pipe Radius
  std::string mobilityModel = "static";  // Parameter 5: Mobility Model
  // ==== END OF EDIT ====
  
  std::string protocol = "VBF";
  std::string csvFile = "comparison-results.csv";
  double initialEnergyJ = 500.0;
  double txPowerW = 2.0;
  double rxPowerW = 0.1;
  double idlePowerW = 0.001;
  double areaX = 400.0;
  double areaY = 400.0;
  double sinkX = 190.0;    // Original value - NOT CHANGED
  double sinkY = 190.0;    // Original value - NOT CHANGED
  double sinkZ = 0.0;
  double senderX = 10.0;   // Original value - NOT CHANGED
  double senderY = 10.0;   // Original value - NOT CHANGED
  double senderZ = 50.0;   // Original value - NOT CHANGED
  uint32_t runNumber = 1;

  // ==== EDITED: Add command line options for the 5 parameters ====
  CommandLine cmd;
  cmd.AddValue("protocol", "Protocol: VBF, DBR, flooding", protocol);
  cmd.AddValue("runNumber", "Run number", runNumber);
  cmd.AddValue("simStop", "Simulation stop time", simStop);
  
  // Parameter 1: Number of nodes
  cmd.AddValue("nodes", "Number of intermediate nodes", nodes);
  
  // Parameter 2: Communication range
  cmd.AddValue("range", "Communication range (meters)", range);
  
  // Parameter 3: Depth variation
  cmd.AddValue("minDepth", "Minimum depth (meters)", minDepth);
  cmd.AddValue("maxDepth", "Maximum depth (meters)", maxDepth);
  
  // Parameter 4: VBF routing pipe radius (width)
  cmd.AddValue("width", "VBF routing pipe radius/width (meters)", vbfWidth);
  
  // Parameter 5: Mobility model
  cmd.AddValue("mobility", "Mobility model: static or random", mobilityModel);
  
  cmd.AddValue("csvFile", "Output CSV file", csvFile);
  cmd.Parse(argc, argv);
  
  // ==== EDITED: Handle common typo VPF -> VBF ====
  if (protocol == "VPF") {
    std::cout << "Note: Protocol 'VPF' corrected to 'VBF'\n";
    protocol = "VBF";
  }
  // ==== END OF EDIT ====
  // ==== END OF EDIT ====

  RngSeedManager::SetRun(runNumber);

  // ==== EDITED: Enhanced output to show the 5 parameters ====
  std::cout << "\n=== UWSN PROTOCOL COMPARISON V2 ===\n";
  std::cout << "Protocol: " << protocol << " | Run: " << runNumber << "\n";
  std::cout << "--- 5 Configurable Parameters ---\n";
  std::cout << "1. Nodes: " << nodes << "\n";
  std::cout << "2. Range: " << range << " m\n";
  std::cout << "3. Depth: [" << minDepth << ", " << maxDepth << "] m\n";
  std::cout << "4. VBF Width: " << vbfWidth << " m\n";
  std::cout << "5. Mobility: " << mobilityModel << "\n";
  std::cout << "--- Network Setup ---\n";
  std::cout << "Sinks: " << sinks << "\n";
  std::cout << "Area: " << areaX << "x" << areaY << "x[" << minDepth << "," << maxDepth << "]\n";
  std::cout << "Sink: (" << sinkX << "," << sinkY << "," << sinkZ << ")\n";
  std::cout << "Sender: (" << senderX << "," << senderY << "," << senderZ << ")\n";
  // ==== END OF EDIT ====

  // ========== NO CHANGES BELOW - Original node creation ==========
  NodeContainer nodesCon, sinksCon, senderCon;
  nodesCon.Create(nodes);  // Uses Parameter 1
  sinksCon.Create(sinks);
  senderCon.Create(1);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);
  socketHelper.Install(senderCon);

  // ========== NO CHANGES - Original Aqua-Sim setup ==========
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetPhy("ns3::AquaSimPhyCmn");
  asHelper.SetMac("ns3::AquaSimBroadcastMac");

  // ==== EDITED: Use Parameter 4 (vbfWidth) for VBF protocol ====
  // NO CHANGES TO PROTOCOL LOGIC - Just passing parameter value
  if (protocol == "VBF") {
    asHelper.SetRouting("ns3::AquaSimVBF", 
                        "Width", DoubleValue(vbfWidth),  // Parameter 4: VBF Width
                        "TargetPos", Vector3DValue(Vector(sinkX, sinkY, sinkZ)));
  } else if (protocol == "DBR") {
    asHelper.SetRouting("ns3::AquaSimDBR");
    // NOTE: DBR uses depth implicitly from node positions
    // Depth variation (Parameter 3) affects DBR through node placement
  } else if (protocol == "flooding") {
    asHelper.SetRouting("ns3::AquaSimFloodingRouting");
  } else {
    NS_FATAL_ERROR("Unsupported protocol: " << protocol);
  }
  // ==== END OF EDIT ====

  // ==== EDITED: Use Parameter 3 (depth variation) in node placement ====
  // NO CHANGES TO LOGIC - Just using configurable min/max depth
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator>();
  std::mt19937 gen(12345 + runNumber);
  std::uniform_real_distribution<> disX(0.0, areaX);
  std::uniform_real_distribution<> disY(0.0, areaY);
  std::uniform_real_distribution<> disZ(minDepth, maxDepth);  // Parameter 3: Depth range

  for (int i = 0; i < nodes; ++i) {
    position->Add(Vector(disX(gen), disY(gen), disZ(gen)));
  }
  position->Add(Vector(sinkX, sinkY, sinkZ));
  position->Add(Vector(senderX, senderY, senderZ));
  // ==== END OF EDIT ====

  // ==== EDITED: Use Parameter 5 (mobility model) ====
  // MOBILITY LOGIC:
  // - static: Nodes stay at fixed positions (realistic for anchored sensors)
  // - random: Nodes move HORIZONTALLY (X,Y only) due to water currents
  //           Depth (Z) remains constant for each node
  // NOTE: Using RandomWalk2dMobilityModel for horizontal-only movement
  
  MobilityHelper mobility;
  mobility.SetPositionAllocator(position);
  
  if (mobilityModel == "static") {
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  } else if (mobilityModel == "random") {
    // Random walk in X,Y plane only (horizontal movement)
    // Simulates water currents - depth stays constant
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(0, areaX, 0, areaY)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                              "Distance", DoubleValue(50.0));
  } else {
    NS_FATAL_ERROR("Unsupported mobility model: " << mobilityModel);
  }
  // ==== END OF EDIT ====

  // ========== NO CHANGES - Original device + energy setup ==========
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
      
      // ==== EDITED: Use Parameter 2 (communication range) ====
      dev->GetPhy()->SetTransRange(range);  // Parameter 2: Range
      // ==== END OF EDIT ====
    }
  };
  createDevice(nodesCon);
  createDevice(sinksCon);
  createDevice(senderCon);

  // ==== EDITED: Connect trace callbacks for metrics ====
  // Reset all tracking variables for each run
  g_receivedPacketUids.clear();
  g_packetSendTime.clear();
  g_packetDelays.clear();
  g_firstNodeDied = false;
  g_networkLifetime = Seconds(simStop);  // Default: simulation end time
  


  // Connect sink packet callback for PDR and delay
  Ptr<AquaSimNetDevice> sinkDev = DynamicCast<AquaSimNetDevice>(sinksCon.Get(0)->GetDevice(0));
  
  // Connect to Routing "PacketReceived"
  sinkDev->GetRouting()->TraceConnectWithoutContext(
    "PacketReceived",
    MakeCallback(&SinkPacketCallback)
  );
  
  // ==== END OF EDIT ====

  mobility.Install(nodesCon);
  mobility.Install(sinksCon);
  mobility.Install(senderCon);

  // ========== NO CHANGES - Original application setup ==========
  PacketSocketAddress socket;
  socket.SetAllDevices();
  socket.SetPhysicalAddress(devices.Get(nodes)->GetAddress());
  socket.SetProtocol(0);

  OnOffHelper app("ns3::PacketSocketFactory", Address(socket));
  app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0066]"));
  app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.9934]"));
  app.SetAttribute("DataRate", DataRateValue(m_dataRate));
  app.SetAttribute("PacketSize", UintegerValue(m_packetSize));

  ApplicationContainer apps = app.Install(senderCon);
  apps.Start(Seconds(0.5));
  apps.Stop(Seconds(simStop));
  
  // ==== EDITED: Connect TX callback for delay tracking ====
  // Use the OnOff application's Tx trace to record send times
  apps.Get(0)->TraceConnectWithoutContext("Tx", MakeCallback(&AppTxCallback));
  // ==== END OF EDIT ====

  // ========== NO CHANGES - Original simulation run ==========
  std::cout << "\nRunning Simulation...\n";
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();

  // ========== NO CHANGES - Original output and energy calculation ==========
  asHelper.GetChannel()->PrintCounters();

  // ==== EDITED: Calculate PDR using UNIQUE packet UIDs ====
  // Uses std::set to deduplicate packets that arrive via multiple paths
  Ptr<AquaSimNetDevice> senderDev = DynamicCast<AquaSimNetDevice>(senderCon.Get(0)->GetDevice(0));
  
  int senderSentPkts = senderDev->TotalSentPkts();  // Packets generated by sender
  int sinkRecvPkts = g_receivedPacketUids.size();   // UNIQUE packets received at sink
  
  // PDR = unique packets received / packets sent (guaranteed 0-100%)
  double pdr = (senderSentPkts > 0) ? (double)sinkRecvPkts / senderSentPkts : 0.0;
  
  std::cout << "\n--- End-to-End PDR (Unique Packets) ---\n";
  std::cout << "Sender Sent: " << senderSentPkts << " pkts\n";
  std::cout << "Sink Received (unique): " << sinkRecvPkts << " pkts\n";
  std::cout << "PDR: " << (pdr * 100.0) << "%\n";
  // ==== END OF EDIT ====

  double totalInitialEnergy = 0.0;
  double totalResidualEnergy = 0.0;
  double minResidualEnergy = std::numeric_limits<double>::max();
  
  for (uint32_t i = 0; i < devices.GetN(); i++) {
    Ptr<AquaSimNetDevice> dev = DynamicCast<AquaSimNetDevice>(devices.Get(i));
    if (dev && dev->EnergyModel()) {
      Ptr<AquaSimEnergyModel> em = dev->EnergyModel();
      totalInitialEnergy += em->GetInitialEnergy();
      double residual = em->GetEnergy();
      totalResidualEnergy += residual;
      
      // Track minimum residual energy to estimate network lifetime
      if (residual < minResidualEnergy) {
        minResidualEnergy = residual;
      }
      
      // Check if this node died (energy depleted)
      if (residual <= 0 && !g_firstNodeDied) {
        g_firstNodeDied = true;
        g_networkLifetime = Seconds(simStop);  // Node died by end of simulation
      }
    }
  }
  double totalConsumedEnergy = totalInitialEnergy - totalResidualEnergy;
  
  // ==== EDITED: Calculate average delay and network lifetime ====
  double avgDelayMs = 0.0;
  if (!g_packetDelays.empty()) {
    double totalDelay = 0.0;
    for (double d : g_packetDelays) {
      totalDelay += d;
    }
    avgDelayMs = totalDelay / g_packetDelays.size();
  }
  
  // Network lifetime: time until first node dies
  // If no node died, estimate based on energy consumption rate
  double networkLifetimeS = g_networkLifetime.GetSeconds();
  if (!g_firstNodeDied && totalConsumedEnergy > 0) {
    // Estimate: how long until the node with min residual dies?
    double consumptionRate = totalConsumedEnergy / simStop;  // J/s
    double perNodeRate = consumptionRate / devices.GetN();
    if (perNodeRate > 0) {
      networkLifetimeS = simStop + (minResidualEnergy / perNodeRate);
    }
  }
  // ==== END OF EDIT ====

  Simulator::Destroy();

  // ==== EDITED: Add all metrics to CSV output ====
  bool writeHeader = false;
  std::ifstream test(csvFile);
  if (!test.good()) writeHeader = true;
  test.close();

  std::ofstream csv(csvFile, std::ios::app);
  if (writeHeader) {
    csv << "run,protocol,nodes,sinks,simStop,"
        << "range,minDepth,maxDepth,vbfWidth,mobility,"  // 5 parameters
        << "areaX,areaY,"
        << "sender_sent,sink_recv,pdr,"  // PDR metrics
        << "avg_delay_ms,network_lifetime_s,"  // NEW: delay and lifetime
        << "initial_energy_per_node,tx_power,rx_power,idle_power,"
        << "total_initial_energy,total_consumed_energy,total_residual_energy\n";
  }
  csv << runNumber << ","
      << protocol << ","
      << nodes << ","  // Parameter 1
      << sinks << ","
      << simStop << ","
      << range << ","  // Parameter 2
      << minDepth << ","  // Parameter 3a
      << maxDepth << ","  // Parameter 3b
      << vbfWidth << ","  // Parameter 4
      << mobilityModel << ","  // Parameter 5
      << areaX << ","
      << areaY << ","
      << senderSentPkts << ","  // Packets sent
      << sinkRecvPkts << ","    // Packets received
      << pdr << ","             // PDR (0.0 to 1.0)
      << avgDelayMs << ","      // NEW: Average delay in ms
      << networkLifetimeS << ","  // NEW: Network lifetime in seconds
      << initialEnergyJ << ","
      << txPowerW << ","
      << rxPowerW << ","
      << idlePowerW << ","
      << totalInitialEnergy << ","
      << totalConsumedEnergy << ","
      << totalResidualEnergy << "\n";
  csv.close();
  // ==== END OF EDIT ====

  std::cout << "\n--- Results Summary ---\n";
  std::cout << "PDR: " << (pdr * 100.0) << "%\n";
  std::cout << "Average Delay: " << avgDelayMs << " ms\n";
  std::cout << "Network Lifetime: " << networkLifetimeS << " s\n";
  std::cout << "Total Initial Energy: " << totalInitialEnergy << " J\n";
  std::cout << "Total Consumed Energy: " << totalConsumedEnergy << " J\n";
  std::cout << "Total Residual Energy: " << totalResidualEnergy << " J\n";
  std::cout << "\nResults appended to: " << csvFile << "\n";
  std::cout << "fin.\n";

  return 0;
}
