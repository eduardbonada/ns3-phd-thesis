/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <iostream>
#include <fstream>

#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/core-module.h"
#include "ns3/helper-module.h"
#include "ns3/bridge-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("OnlyBridgesTestExample");

int 
main (int argc, char *argv[])
{
  //
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //
#if 1 
  LogComponentEnable ("OnlyBridgesTestExample", LOG_LEVEL_ALL);
  //LogComponentEnable ("BridgeHelper", LOG_LEVEL_ALL);
  //LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_FUNCTION); 
  LogComponentEnable ("BridgeNetDevice", LOG_LEVEL_FUNCTION); 
  LogComponentEnable ("STP", LOG_LEVEL_ALL); 
#endif

  //
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  //
  CommandLine cmd;
  cmd.Parse (argc, argv);

  //
  // create the nodes required by the topology
  //
  NS_LOG_INFO ("Create nodes.");
  NodeContainer bridges;
  bridges.Create (2);
  
  //
  // connect the nodes, create the topology
  //
  NS_LOG_INFO ("Build Topology - Csma Helper");
  CsmaHelper csmaHlp;
  csmaHlp.SetChannelAttribute ("DataRate", DataRateValue (10000000));
  csmaHlp.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (50)));

  // Create the csma link between bridges
  NS_LOG_INFO ("Build Topology - Create Csma links between bridges");
  std::vector< NetDeviceContainer > bridgeDevices;
  NetDeviceContainer link = csmaHlp.Install (NodeContainer (bridges.Get (0), bridges.Get (1)));
  NetDeviceContainer NetDevContTmp1, NetDevContTmp2;
  bridgeDevices.push_back(NetDevContTmp1);
  bridgeDevices.push_back(NetDevContTmp2);
  bridgeDevices[0].Add (link.Get (0));
  bridgeDevices[1].Add (link.Get (1));
 
  // Create the bridgenetdevice's, which will do the frame forwarding, the learning...
  NS_LOG_INFO ("Build Topology - Create BridgeNetDevice");
  BridgeHelper bridgeHlp;  // no parameters to configure in the helper
  Ptr<BridgeNetDevice> bridgeNetDevOne = bridgeHlp.Install (bridges.Get (0), bridgeDevices[0]);
  Ptr<BridgeNetDevice> bridgeNetDevTwo = bridgeHlp.Install (bridges.Get (1), bridgeDevices[1]);
  
  //Create the STP
  NS_LOG_INFO ("Build Topology - Create STP");
  Ptr<STP> stpOne = Create <STP> ();
  Ptr<STP> stpTwo = Create <STP> ();
  stpOne->Install(bridges.Get (0), bridgeNetDevOne, bridgeDevices[0]);
  stpOne->Install(bridges.Get (1), bridgeNetDevTwo, bridgeDevices[1]);
    
  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "csma-bridge.tr"
  //
  NS_LOG_INFO ("Configure Tracing.");
  std::ofstream ascii;
  ascii.open ("examples/bridging/only-csma-bridges-test.tr");
  CsmaHelper::EnableAsciiAll (ascii);
  

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
