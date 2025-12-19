/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * UWSN Protocol Comparison
 * Based on original working version
 * Added: Command line options, CBR mode, PDR output
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

int
main(int argc, char *argv[])
{
  // ============ PARAMETERS (adjusted for better connectivity) ============
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
  // Smaller area for denser node deployment (better connectivity)
  double areaX = 200.0;
  double areaY = 200.0;
  double minDepth = 0.0;
  double maxDepth = 150.0;
  // Sink at surface center, sender at bottom
  double sinkX = 100.0;
  double sinkY = 100.0;
  double sinkZ = 0.0;
  double senderX = 50.0;
  double senderY = 50.0;
  double senderZ = 120.0;
  uint32_t runNumber = 1;
  bool useCBR = false;  // CBR mode option

  // ============ COMMAND LINE OPTIONS ============
  CommandLine cmd;
  cmd.AddValue("protocol", "Protocol: VBF, DBR, flooding", protocol);
  cmd.AddValue("runNumber", "Run number for random seed", runNumber);
  cmd.AddValue("nodes", "Number of intermediate nodes", nodes);
  cmd.AddValue("simStop", "Simulation duration", simStop);
  cmd.AddValue("range", "Transmission range", range);
  cmd.AddValue("csvFile", "Output CSV file", csvFile);
  cmd.AddValue("dataRate", "Data rate in bps", m_dataRate);
  cmd.AddValue("packetSize", "Packet size in bytes", m_packetSize);
  cmd.AddValue("cbr", "Use CBR (constant bit rate) traffic", useCBR);
  cmd.Parse(argc, argv);

  RngSeedManager::SetRun(runNumber);

  std::cout << "\n=== UWSN PROTOCOL COMPARISON ===\n";
  std::cout << "Protocol: " << protocol << " | Run: " << runNumber << "\n";
  std::cout << "Nodes: " << nodes << ", Sinks: " << sinks << "\n";
  std::cout << "Traffic: " << (useCBR ? "CBR" : "OnOff") << " @ " << m_dataRate << " bps\n";
  std::cout << "Area: " << areaX << "x" << areaY << "x[" << minDepth << "," << maxDepth << "]\n";
  std::cout << "Sink: (" << sinkX << "," << sinkY << "," << sinkZ << ")\n";
  std::cout << "Sender: (" << senderX << "," << senderY << "," << senderZ << ")\n";
  std::cout << "Range: " << range << "m, SimTime: " << simStop << "s\n";

  // Create nodes
  NodeContainer nodesCon, sinksCon, senderCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);
  senderCon.Create(1);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);
  socketHelper.Install(senderCon);

  // Aqua-Sim setup (ORIGINAL)
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetPhy("ns3::AquaSimPhyCmn");
  asHelper.SetMac("ns3::AquaSimBroadcastMac");

  // Protocol setup (ORIGINAL)
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

  // Mobility setup (ORIGINAL)
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

  // Device + Energy setup (ORIGINAL)
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

  mobility.Install(nodesCon);
  mobility.Install(sinksCon);
  mobility.Install(senderCon);

  // Application setup
  PacketSocketAddress socket;
  socket.SetAllDevices();
  socket.SetPhysicalAddress(devices.Get(nodes)->GetAddress());
  socket.SetProtocol(0);

  OnOffHelper app("ns3::PacketSocketFactory", Address(socket));
  
  if (useCBR) {
    // CBR mode: continuous transmission
    app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1000]"));
    app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  } else {
    // Original OnOff mode: ~0.66% duty cycle
    app.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0066]"));
    app.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.9934]"));
  }
  app.SetAttribute("DataRate", DataRateValue(m_dataRate));
  app.SetAttribute("PacketSize", UintegerValue(m_packetSize));

  ApplicationContainer apps = app.Install(senderCon);
  apps.Start(Seconds(0.5));
  apps.Stop(Seconds(simStop));

  // Run simulation
  std::cout << "Running Simulation...\n";
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();

  // Print channel counters
  std::cout << "\n--- Channel Statistics ---\n";
  asHelper.GetChannel()->PrintCounters();

  // Get sender and sink specific stats
  Ptr<AquaSimNetDevice> senderDev = DynamicCast<AquaSimNetDevice>(devices.Get(nodes + 1));
  Ptr<AquaSimNetDevice> sinkDev = DynamicCast<AquaSimNetDevice>(devices.Get(nodes));
  
  int senderSent = senderDev ? senderDev->TotalSentPkts() : 0;
  int sinkRecv = (sinkDev && sinkDev->GetRouting()) ? sinkDev->GetRouting()->SendUpPktCount() : 0;
  double pdr = (senderSent > 0) ? (double)sinkRecv / senderSent * 100.0 : 0.0;

  std::cout << "\n--- End-to-End ---\n";
  std::cout << "Sender Sent: " << senderSent << "\n";
  std::cout << "Sink Received: " << sinkRecv << "\n";
  std::cout << "PDR: " << pdr << "%\n";

  // Energy calculation (ORIGINAL)
  double totalInitialEnergy = 0.0;
  double totalResidualEnergy = 0.0;
  for (uint32_t i = 0; i < devices.GetN(); i++) {
    Ptr<AquaSimNetDevice> dev = DynamicCast<AquaSimNetDevice>(devices.Get(i));
    if (dev && dev->EnergyModel()) {
      Ptr<AquaSimEnergyModel> em = dev->EnergyModel();
      totalInitialEnergy += em->GetInitialEnergy();
      totalResidualEnergy += em->GetEnergy();
    }
  }
  double totalConsumedEnergy = totalInitialEnergy - totalResidualEnergy;
  double avgEnergyPerNode = totalConsumedEnergy / devices.GetN();

  Simulator::Destroy();

  // CSV output (enhanced)
  bool writeHeader = false;
  std::ifstream test(csvFile);
  if (!test.good()) writeHeader = true;
  test.close();

  std::ofstream csv(csvFile, std::ios::app);
  if (writeHeader) {
    csv << "run,protocol,nodes,simStop,range,traffic_mode,data_rate,packet_size,"
        << "sender_sent,sink_recv,pdr,"
        << "total_initial_energy,total_consumed_energy,avg_energy_per_node\n";
  }
  csv << runNumber << ","
      << protocol << ","
      << nodes << ","
      << simStop << ","
      << range << ","
      << (useCBR ? "CBR" : "OnOff") << ","
      << m_dataRate << ","
      << m_packetSize << ","
      << senderSent << ","
      << sinkRecv << ","
      << pdr << ","
      << totalInitialEnergy << ","
      << totalConsumedEnergy << ","
      << avgEnergyPerNode << "\n";
  csv.close();

  std::cout << "\n=== RESULTS ===\n";
  std::cout << "PDR: " << pdr << "%\n";
  std::cout << "Energy Consumed: " << totalConsumedEnergy << " J\n";
  std::cout << "Avg Energy/Node: " << avgEnergyPerNode << " J\n";
  std::cout << "Results saved to: " << csvFile << "\n";
  std::cout << "fin.\n";

  return 0;
}
