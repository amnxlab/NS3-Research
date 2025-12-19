/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * UWSN Protocol Comparison - Final Version
 * - Correct energy tracking
 * - No unnecessary callbacks
 * - Matches requested CSV format
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
  // Parameters
  double simStop = 1000;
  int nodes = 100;
  int sinks = 1;
  uint32_t m_dataRate = 10000;
  uint32_t m_packetSize = 40;
  double range = 100;
  std::string protocol = "VBF";
  std::string csvFile = "comparison-results.csv";
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
  cmd.AddValue("runNumber", "Run number", runNumber);
  cmd.Parse(argc, argv);

  RngSeedManager::SetRun(runNumber);

  std::cout << "\n=== UWSN PROTOCOL COMPARISON ===\n";
  std::cout << "Protocol: " << protocol << " | Run: " << runNumber << "\n";
  std::cout << "Nodes: " << nodes << ", Sinks: " << sinks << "\n";
  std::cout << "Area: " << areaX << "x" << areaY << "x[" << minDepth << "," << maxDepth << "]\n";
  std::cout << "Sink: (" << sinkX << "," << sinkY << "," << sinkZ << ")\n";
  std::cout << "Sender: (" << senderX << "," << senderY << "," << senderZ << ")\n";

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

  // Mobility setup
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

  mobility.Install(nodesCon);
  mobility.Install(sinksCon);
  mobility.Install(senderCon);

  // Application
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

  // Run simulation
  std::cout << "Running Simulation...\n";
  Simulator::Stop(Seconds(simStop));
  Simulator::Run();

  // Print channel counters (use these for PDR)
  asHelper.GetChannel()->PrintCounters();

  // Energy calculation
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

  Simulator::Destroy();

  // CSV output
  bool writeHeader = false;
  std::ifstream test(csvFile);
  if (!test.good()) writeHeader = true;
  test.close();

  std::ofstream csv(csvFile, std::ios::app);
  if (writeHeader) {
    csv << "run,protocol,nodes,sinks,simStop,range,areaX,areaY,minDepth,maxDepth,"
        << "initial_energy_per_node,tx_power,rx_power,idle_power,"
        << "total_initial_energy,total_consumed_energy,total_residual_energy\n";
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
      << initialEnergyJ << ","
      << txPowerW << ","
      << rxPowerW << ","
      << idlePowerW << ","
      << totalInitialEnergy << ","
      << totalConsumedEnergy << ","
      << totalResidualEnergy << "\n";
  csv.close();

  std::cout << "\nResults appended to: " << csvFile << std::endl;
  std::cout << "Total Initial Energy: " << totalInitialEnergy << " J" << std::endl;
  std::cout << "Total Consumed Energy: " << totalConsumedEnergy << " J" << std::endl;
  std::cout << "Total Residual Energy: " << totalResidualEnergy << " J" << std::endl;
  std::cout << "➡️ Use 'Sent Pkts' and 'SendUp Pkts' from output to compute PDR." << std::endl;
  std::cout << "fin.\n";

  return 0;
}
