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

NS_LOG_COMPONENT_DEFINE ("CsmaBridgeTestExample");

int 
main (int argc, char *argv[])
{
  //
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //
#if 1 
  LogComponentEnable ("CsmaBridgeTestExample", LOG_LEVEL_ALL);
  LogComponentEnable ("BridgeHelper", LOG_LEVEL_FUNCTION);
  LogComponentEnable ("BridgeNetDevice", LOG_LEVEL_FUNCTION);
  LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_FUNCTION); 
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_FUNCTION); 
  LogComponentEnable ("Node", LOG_LEVEL_FUNCTION); 
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
  NodeContainer hosts;
  hosts.Create (2);
  NodeContainer bridges;
  bridges.Create (2);
  
  //
  // connect the nodes, create the topology
  //
  NS_LOG_INFO ("Build Topology - Csma Helper");
  CsmaHelper csmaHlp;
  csmaHlp.SetChannelAttribute ("DataRate", DataRateValue (10000000));
  csmaHlp.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (50)));

  // Create the csma links, from each terminal to the corresponding bridge
  NS_LOG_INFO ("Build Topology - Create Csma links bridge-hosts");
  std::vector< NetDeviceContainer > hostDevices;
  std::vector< NetDeviceContainer > bridgeDevices;
  
  for (int i = 0; i < 2; i++)
    {
      NetDeviceContainer link = csmaHlp.Install (NodeContainer (hosts.Get(i), bridges.Get(i)));
      NetDeviceContainer NetDevContTmp1, NetDevContTmp2;
      
      NetDevContTmp1.Add (link.Get (0));
      hostDevices.push_back(NetDevContTmp1);

      NetDevContTmp2.Add (link.Get (1));
      bridgeDevices.push_back(NetDevContTmp2);
    }

  // Create the csma link between bridges
  NS_LOG_INFO ("Build Topology - Create Csma links between bridges");
  NetDeviceContainer linkB = csmaHlp.Install (NodeContainer (bridges.Get (0), bridges.Get (1)));
  bridgeDevices[0].Add (linkB.Get (0));
  bridgeDevices[1].Add (linkB.Get (1));
 
  // Create the bridgenetdevice's, which will do the frame forwarding, the learning...
  NS_LOG_INFO ("Build Topology - Create BridgeNetDevice");
  BridgeHelper bridgeHlp;  // no parameters to configure in the helper
  bridgeHlp.Install (bridges.Get (0), bridgeDevices[0]);
  bridgeHlp.Install (bridges.Get (1), bridgeDevices[1]);
  
  // Add internet stack to the hosts
  NS_LOG_INFO ("Build Topology - Add IP stack to hosts");
  InternetStackHelper internet;
  internet.Install (hosts);

  //
  // We've got the "hardware" in place.  Now we need to add IP addresses.
  //
  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (hostDevices[0]);
  ipv4.Assign (hostDevices[1]);
  
  //
  // Create an OnOff application to send UDP datagrams from node zero to node 1.
  //
  NS_LOG_INFO ("Create Applications.");
  uint16_t port = 9;   // Discard port (RFC 863)

  OnOffHelper onoff ("ns3::UdpSocketFactory", 
                     Address (InetSocketAddress (Ipv4Address ("10.1.1.2"), port)));
  onoff.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (10)));
  onoff.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
  onoff.SetAttribute ("PacketSize", UintegerValue (100));
  onoff.SetAttribute ("DataRate", DataRateValue(100));
  onoff.SetAttribute ("MaxBytes", UintegerValue (100));  

  ApplicationContainer app = onoff.Install (hosts.Get (0));
  // Start the application
  app.Start (Seconds (1.0));
  app.Stop (Seconds (10.0));

  // Create an optional packet sink to receive these packets
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  app = sink.Install (hosts.Get (1));
  app.Start (Seconds (0.0));

  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "csma-bridge.tr"
  //
  NS_LOG_INFO ("Configure Tracing.");
  std::ofstream ascii;
  ascii.open ("examples/bridging/csma-bridge-test.tr");
  CsmaHelper::EnableAsciiAll (ascii);
  //CsmaHelper::EnableAscii (ascii, hosts.Get(0)->GetId(), hostDevices[0].Get(0)->GetIfIndex());
  //CsmaHelper::EnableAscii (ascii, hosts.Get(1)->GetId(), hostDevices[1].Get(0)->GetIfIndex());
  //CsmaHelper::EnableAscii (ascii, bridges.Get(0)->GetId(), bridgeDevices[0].Get(0)->GetIfIndex());
  //CsmaHelper::EnableAscii (ascii, bridges.Get(0)->GetId(), bridgeDevices[0].Get(1)->GetIfIndex());
  //CsmaHelper::EnableAscii (ascii, bridges.Get(1)->GetId(), bridgeDevices[1].Get(0)->GetIfIndex());
  //CsmaHelper::EnableAscii (ascii, bridges.Get(1)->GetId(), bridgeDevices[1].Get(1)->GetIfIndex());
  
  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     csma-bridge-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
  //CsmaHelper::EnablePcapAll ("csma-bridge", false);

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
