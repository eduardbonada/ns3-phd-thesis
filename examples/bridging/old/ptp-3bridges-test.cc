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

NS_LOG_COMPONENT_DEFINE ("PtpBridgesTest");

int 
main (int argc, char *argv[])
{
#if 1 
  LogComponentEnable ("PtpBridgesTest", LOG_LEVEL_ALL);
  //LogComponentEnable ("BridgeHelper", LOG_LEVEL_ALL);
  //LogComponentEnable ("EthPointToPointHelper", LOG_LEVEL_ALL);
  //LogComponentEnable ("EthPointToPointNetDevice", LOG_LEVEL_ALL); 
  //LogComponentEnable ("BridgeNetDevice", LOG_LEVEL_ALL); 
  LogComponentEnable ("STP", LOG_LEVEL_DEBUG); 
#endif

  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  CommandLine cmd;
  cmd.Parse (argc, argv);

  //variables
  NS_LOG_INFO ("Load variables.");
  uint64_t g_numNodes;
  g_numNodes = 3;

  // create the nodes required by the topology
  //NodeList
  NS_LOG_INFO ("Create nodes.");
  NodeContainer bridges;
  bridges.Create (g_numNodes);
  
  // connect the nodes, create the topology
  NS_LOG_INFO ("Build Topology - EthPointToPoint Helper");
  EthPointToPointHelper ethPtpHlp;
  ethPtpHlp.SetDeviceAttribute ("DataRate", DataRateValue (10000000));
  ethPtpHlp.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

  //Create/read the physical vertex adjacency matrix
  NS_LOG_INFO ("Build Topology - Create Phy Vertex Adjacency Matrix");  
  int vertexAdjPhyMx[3][3]={{0, 1, 1},{1, 0, 1},{1, 1, 0}};
  
  // Create the eth-ptp link between bridges
  NS_LOG_INFO ("Build Topology - Create EthPtp links between bridges");

  int vertexAdjPhyMxVisited[3][3]={{0, 0, 0},{0, 0, 0},{0, 0, 0}};
  NetDeviceContainer bridgePorts[g_numNodes];
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
    {
      for (uint64_t j=0 ; j < g_numNodes ; j++ )
        {
          if(vertexAdjPhyMxVisited[i][j]==0 && vertexAdjPhyMx[i][j]==1)
            {
              NetDeviceContainer link = ethPtpHlp.Install (bridges.Get (i), bridges.Get (j));
              bridgePorts[i].Add (link.Get (0));
              bridgePorts[j].Add (link.Get (1));
              
              vertexAdjPhyMxVisited[i][j]=1;
              vertexAdjPhyMxVisited[j][i]=1;
            }
        }  
    }
 
  // Create the bridgenetdevice's, which will do the frame forwarding, the learning...
  NS_LOG_INFO ("Build Topology - Create BridgeNetDevice");
  BridgeHelper bridgeHlp;  // no parameters to configure in the helper
  Ptr<BridgeNetDevice> bridgeNetDev[g_numNodes];
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
    {
      bridgeNetDev[i] = bridgeHlp.Install (bridges.Get (i), bridgePorts[i]);
    }

  //Create the STP
  NS_LOG_INFO ("Build Topology - Create STP");
  Ptr<STP> stpModules[g_numNodes];  
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
    {
      stpModules[i] = Create <STP> ();
      stpModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridges.Get(i)->GetId());
      
    }
    
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "xxx.tr"
  NS_LOG_INFO ("Configure Tracing.");
  std::ofstream ascii;
  ascii.open ("examples/bridging/ptp-bridges-test.tr");
  //EthPointToPointHelper::EnableAscii (ascii, NodeContainer(bridges.Get (0)));
  //EthPointToPointHelper::EnableAsciiAll (ascii);
  

  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
