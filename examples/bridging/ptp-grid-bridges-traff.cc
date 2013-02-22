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
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime> 

///////////////////////////////////////////////////////////////////////
// note this is copies to xstp.h, main simulation file and bpdu header.
#define ROOT_ROLE "R"
#define DESIGNATED_ROLE "D"
#define ALTERNATE_ROLE "A"
#define BACKUP_ROLE "B"
// note this is copies to xstp.h, main simulation file and bpdu header.
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////
// copy this to main file and traffgen if changed
#define DIST_CONSTANT     1
#define DIST_UNIFORM      2
#define DIST_EXPONENTIAL  3
#define DIST_PARETO       4
#define DIST_NORMAL       5
#define DIST_LOGNORMAL    6
// copy this to main file and traffgen if changed
///////////////////////////////////////////////////

#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/core-module.h"
#include "ns3/helper-module.h"
#include "ns3/bridge-module.h"

#include "ns3/simulation-god.h"

#include "ns3/net-anim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PtpGridBridges");

//////////////////////////////////
// ReadCostMatrixFromFile()
//////////////////////////////////
int ReadCostMatrixFromFile(std::string filename, std::vector < std::vector < int > > *mx, uint64_t numNodes)
{
/*
  - fills the vertex adjacency matrix mx according to the values read from the cost matrix in input file
  - the output matrix contains '1' if nodes are connected, '0' otherwise
*/

  std::ifstream mxFile;
  std::string valueRead;
  int valueReadInt;
  int fakeInf = 999999;
      
  //open file
  mxFile.open(filename.c_str());
  if (!mxFile)
    {
      std::cout << "ERROR: file " << filename.c_str() << " is not correct." << std::endl;
      std::exit(-1);
    }  
  
  // read data
  for(uint64_t i=0 ; i < numNodes ; i++)
    {
      for(uint64_t j=0 ; j < numNodes ; j++)
        {
          if(mxFile.eof())
            {
              std::cout << "ERROR: EOF reached before reading all nodes (check g_numNodes)" << std::endl;
              exit(1);
            }
          mxFile >> valueRead;
          valueReadInt = atoi(valueRead.c_str());
          
          //we set the values of the vertex adj matrix
          if(valueReadInt == 0)
            {
              (*mx).at(i).at(j) = 0;
            }
          else
            {
              if(valueReadInt == fakeInf)
                {
                  (*mx).at(i).at(j) = 0;
                }
              else
                {
                  (*mx).at(i).at(j) = 1;          
                }
            }
        }
    }
  
  // check there is nothing else to read
  mxFile >> valueRead;
  if(!mxFile.eof())
    {
      std::cout << "ERROR: still data to read in file (check g_numNodes)" << std::endl;
      exit(1);
    }
  
  mxFile.close(); 
  
  return 1;
}



//////////////////////////////////
// main
//////////////////////////////////
int 
main (int argc, char *argv[])
{
  LogComponentEnable ("PtpGridBridges", LOG_LEVEL_ALL);

  LogComponentEnable ("BridgeTraffGen", LOG_LEVEL_INFO); 
  LogComponentEnable ("BridgeTraffSink", LOG_LEVEL_INFO); 

  LogComponentEnable ("STP", LOG_LEVEL_INFO); 
  LogComponentEnable ("RSTP", LOG_LEVEL_INFO);
  LogComponentEnable ("RSTPconf", LOG_LEVEL_INFO);
  LogComponentEnable ("SPBISIS", LOG_LEVEL_INFO);
  LogComponentEnable ("SPBDV", LOG_LEVEL_INFO);
  LogComponentEnable ("SPBDVconf", LOG_LEVEL_INFO);

  LogComponentEnable ("EthPointToPointChannel", LOG_LEVEL_INFO);   // NS_LOG_INFO to trace the link failure
  //LogComponentEnable ("BridgeNetDevice", LOG_LEVEL_ALL);          // NS_LOG_LOGIC for debbuging - NS_LOG_INFO for normal traces

  LogComponentEnable ("SimulationGod", LOG_LEVEL_INFO);

  //LogComponentEnable ("BpduHeader", LOG_LEVEL_INFO);
  //LogComponentEnable ("BpduMergedHeader", LOG_LEVEL_ALL);
    
  //LogComponentEnable ("EthPointToPointNetDevice", LOG_LEVEL_ALL);  
  LogComponentEnable ("DropTailQueue", LOG_LEVEL_INFO);    
  //LogComponentEnable ("BridgeHelper", LOG_LEVEL_ALL);
  //LogComponentEnable ("EthPointToPointHelper", LOG_LEVEL_ALL);

  //LogComponentEnable ("Application", LOG_LEVEL_ALL);
  //LogComponentEnable ("Simulator", LOG_LEVEL_ALL);

  // SIMULATION default parameters
  uint64_t     g_numNodes = 4;
  std::string phyMxFilename;

  //SEED
  time_t seedTime;
  seedTime = time(NULL);
  srand ( seedTime );
  NS_LOG_INFO ("\nSEED => " << seedTime);
   
  // LINK default parameters
  uint64_t linkDatarateBps = 1000000000;
  uint64_t linkDelayUs     = 5;
  
  // FAILURE default parameters
  uint16_t activateFailure       = 0;
  uint16_t activatePhyFailDetect = 0;
  uint64_t linkFailureTimeUs     = 0;
  int64_t nodeAToFail            = 0;
  int64_t nodeBToFail            = 0; //-1 for all
  
  // STP default parameters
  uint16_t stpMode                       = 1;
  uint16_t randomBridgeIds               = 0;
  int64_t  forcedRootNode                = 0;
  int64_t  forcedSecondRootNode          = 0;
  uint16_t activateRandBridgeInit        = 0;
  uint16_t xstpSimulationTimeSecs        = 100;
  uint16_t xstpActivatePeriodicalBpdus   = 0;
  uint16_t xstpActivateExpirationMaxAge  = 0;
  uint16_t xstpActivateIncreaseMessAge   = 0;
  uint16_t xstpActivateTopologyChange    = 0;
  uint16_t xstpMaxAge                    = 20;
  uint16_t xstpHelloTime                 = 2;
  uint16_t xstpForwDelay                 = 15;
  double   txHoldPeriod                  = 1;
  uint32_t xstpMaxBpduTxHoldPeriod       = 10000;
  uint32_t xstpResetTxHoldPeriod         = 1;
  uint16_t spbdvActivatePathVectorDuplDetect = 0;
  uint16_t spbdvActivatePeriodicalStpStyle   = 0;
  uint16_t spbdvActivateBpduMerging      = 0;
  uint16_t spbisisActivateBpduForwarding = 0;
  
  // TRAFFIC default parameters
  uint16_t trafficActivate               = 0;
  uint16_t trafficPktBytesDistr          = 1;
  uint16_t trafficPktBytesMean           = 1000;
  uint16_t trafficPktBytesVariance       = 0;                  
  uint16_t trafficPktBytesUpper          = 0;                  
  uint16_t trafficPktBytesLower          = 0;
  uint16_t trafficInterPktTimeDistr      = 1;
  double trafficInterPktTimeMean         = 1;
  double trafficInterPktTimeVariance     = 0;                  
  double trafficInterPktTimeUpper        = 0;                  
  double trafficInterPktTimeLower        = 0;
  uint16_t trafficDestAddrMode           = 0;

  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  CommandLine cmd;
  cmd.AddValue("numNodes", "Number of nodes in the network", g_numNodes);
  cmd.AddValue("phyMxFilename", "Filename of the physical topology vertex adjacency matrix", phyMxFilename);

  cmd.AddValue("linkDatarateBps", "EthPointToPoint: Datarate of the point-to-point links", linkDatarateBps);
  cmd.AddValue("linkDelayUs", "EthPointToPoint: Delay of the point-to-point links", linkDelayUs);

  cmd.AddValue("activateFailure", "Activates the failure property", activateFailure);
  cmd.AddValue("activatePhyFailDetect", "Activates the link physical detection of the failure", activatePhyFailDetect);
  cmd.AddValue("linkFailureTimeUs", "Time when the link specified by 'nodeAToFail-nodeBToFail' fails", linkFailureTimeUs);
  cmd.AddValue("nodeAToFail", "One edge of the link to fail", nodeAToFail);
  cmd.AddValue("nodeBToFail", "The other edge of the link to fail", nodeBToFail);

  cmd.AddValue("stpMode", "Configure STP(1), RSTP(2), SPBISIS(3), SPBDV(4), RSTPconf(5)", stpMode);
  cmd.AddValue("randomBridgeIds", "Assign bridge IDs: random (true) or sequential(false)", randomBridgeIds);
  cmd.AddValue("forcedRootNode", "Force the root to this node (random if -1). Only meaning when randomBridgeIds active.", forcedRootNode);
  cmd.AddValue("forcedSecondRootNode", "Force the second root, elected when primary fails, to this node (random if -1). Only meaning when randomBridgeIds active.", forcedSecondRootNode);
  cmd.AddValue("activateRandBridgeInit", "Assign random startup times to nodes", activateRandBridgeInit);
  cmd.AddValue("xstpSimulationTimeSecs", "xSTP: Simulation Seconds (for terminating reasons)", xstpSimulationTimeSecs);
  cmd.AddValue("xstpActivatePeriodicalBpdus", "xSTP: Activate the creation of periodical BPDUs", xstpActivatePeriodicalBpdus);
  cmd.AddValue("xstpActivateExpirationMaxAge", "xSTP: Activate expiring max age timers", xstpActivateExpirationMaxAge);
  cmd.AddValue("xstpActivateIncreaseMessAge", "xSTP: Activate increasing MessAge value in BPDU", xstpActivateIncreaseMessAge);
  cmd.AddValue("xstpActivateTopologyChange", "xSTP: Activate Topology Change procedure", xstpActivateTopologyChange);
  cmd.AddValue("xstpMaxAge", "xSTP: MaxAge timer", xstpMaxAge);
  cmd.AddValue("xstpForwDelay", "xSTP: Forwarding Delay timer", xstpForwDelay);  
  cmd.AddValue("xstpHelloTime", "xSTP: HelloTime timer", xstpHelloTime);
  cmd.AddValue("txHoldPeriod", "xSTP&SPB-LS: TxHoldPeriod timer", txHoldPeriod);
  cmd.AddValue("xstpMaxBpduTxHoldPeriod", "xSTP: Maximum Number of BPDUs per TxHoldPeriod", xstpMaxBpduTxHoldPeriod);
  cmd.AddValue("xstpResetTxHoldPeriod", "RSTP: Whether the value is reset or decremented (only RSTP)", xstpResetTxHoldPeriod);
  
  cmd.AddValue("spbdvActivatePathVectorDuplDetect", "SPBDV: activates the detection of duplicated elements in the path vector (in order to avoid looping BPDUs)", spbdvActivatePathVectorDuplDetect);
  cmd.AddValue("spbdvActivatePeriodicalStpStyle", "SPBDV: activates the STP style (only root sends) when sending periodical BPDUs", spbdvActivatePeriodicalStpStyle);
  cmd.AddValue("spbdvActivateBpduMerging", "SPBDV: activates BPDU merging (otherwise one BPDU per frame)", spbdvActivateBpduMerging);

  cmd.AddValue("spbisisActivateBpduForwarding", "SPBISIS: activates the exchange of BPDUs to transition ports to forwarding (otherwise the transition is made by simGod)", spbisisActivateBpduForwarding);

  cmd.AddValue("trafficActivate", "Traffic: Activate the generation of data traffic", trafficActivate);
  cmd.AddValue("trafficPktBytesDistr", "Traffic: Distribution for the packet length computations", trafficPktBytesDistr);
  cmd.AddValue("trafficPktBytesMean", "Traffic: Mean for the packet length computations", trafficPktBytesMean);
  cmd.AddValue("trafficPktBytesVariance", "Traffic: Variance for the packet length computations", trafficPktBytesVariance);
  cmd.AddValue("trafficPktBytesUpper", "Traffic: Upper value for the packet length computations", trafficPktBytesUpper);
  cmd.AddValue("trafficPktBytesLower", "Traffic: Lower value for the packet length computations", trafficPktBytesLower);
  cmd.AddValue("trafficInterPktTimeDistr", "Traffic: Distribution for the inter packet time computations", trafficInterPktTimeDistr);
  cmd.AddValue("trafficInterPktTimeMean", "Traffic: Mean for the inter packet time computations", trafficInterPktTimeMean);
  cmd.AddValue("trafficInterPktTimeVariance", "Traffic: Variance for the inter packet time computations", trafficInterPktTimeVariance);
  cmd.AddValue("trafficInterPktTimeUpper", "Traffic: Upper value for the inter packet time computations", trafficInterPktTimeUpper);
  cmd.AddValue("trafficInterPktTimeLower", "Traffic: Lower value for the inter packet time computations", trafficInterPktTimeLower);
  cmd.AddValue("trafficDestAddrMode", "Traffic: Destination Addresses Mode", trafficDestAddrMode);

  cmd.Parse (argc, argv);

  //print input parameters
  NS_LOG_INFO ("\nInput Parameters");
  NS_LOG_INFO ("----------------");
  NS_LOG_INFO ("  numNodes:                       " << g_numNodes);
  NS_LOG_INFO ("  phyMxFilename:                  " << phyMxFilename);
  NS_LOG_INFO ("  linkDatarateBps:                " << linkDatarateBps);
  NS_LOG_INFO ("  linkDelayUs:                    " << linkDelayUs);
  NS_LOG_INFO ("  activateFailure:                " << activateFailure);
  NS_LOG_INFO ("  activatePhyFailDetect:          " << activatePhyFailDetect);
  NS_LOG_INFO ("  linkFailureTimeUs:              " << linkFailureTimeUs);
  NS_LOG_INFO ("  nodeAToFail:                    " << nodeAToFail);
  NS_LOG_INFO ("  nodeBToFail:                    " << nodeBToFail);
  NS_LOG_INFO ("  stpMode:                        " << stpMode);
  NS_LOG_INFO ("  randomBridgeIds:                " << randomBridgeIds);
  NS_LOG_INFO ("  forcedRootNode:                 " << forcedRootNode);
  NS_LOG_INFO ("  forcedSecondRootNode:           " << forcedSecondRootNode);
  NS_LOG_INFO ("  activateRandBridgeInit:         " << activateRandBridgeInit);
  NS_LOG_INFO ("  xstpSimulationTimeSecs:         " << xstpSimulationTimeSecs);
  NS_LOG_INFO ("  xstpActivatePeriodicalBpdus:    " << xstpActivatePeriodicalBpdus);
  NS_LOG_INFO ("  xstpActivateExpirationMaxAge:   " << xstpActivateExpirationMaxAge);
  NS_LOG_INFO ("  xstpActivateIncreaseMessAge:    " << xstpActivateIncreaseMessAge);
  NS_LOG_INFO ("  xstpActivateTopologyChange:     " << xstpActivateTopologyChange);
  NS_LOG_INFO ("  spbisisActivateBpduForwarding:  " << spbisisActivateBpduForwarding);
  NS_LOG_INFO ("  spbdvActivatePVDuplDetect:      " << spbdvActivatePathVectorDuplDetect);
  NS_LOG_INFO ("  spbdvActivatePeriodicalStpStyle:" << spbdvActivatePeriodicalStpStyle);
  NS_LOG_INFO ("  spbdvActivateBpduMerging:       " << spbdvActivateBpduMerging);
  NS_LOG_INFO ("  xstpMaxAge:                     " << xstpMaxAge);
  NS_LOG_INFO ("  xstpForwDelay:                  " << xstpForwDelay);  
  NS_LOG_INFO ("  xstpHelloTime:                  " << xstpHelloTime);
  NS_LOG_INFO ("  txHoldPeriod:                   " << txHoldPeriod);    
  NS_LOG_INFO ("  xstpMaxBpduTxHoldPeriod:        " << xstpMaxBpduTxHoldPeriod);   
  NS_LOG_INFO ("  xstpResetTxHoldPeriod:          " << xstpResetTxHoldPeriod);   
  NS_LOG_INFO ("  trafficActivate                 " << trafficActivate);
  NS_LOG_INFO ("  trafficPktBytesDistr:           " << trafficPktBytesDistr);   
  NS_LOG_INFO ("  trafficPktBytesMean:            " << trafficPktBytesMean);   
  NS_LOG_INFO ("  trafficPktBytesVariance:        " << trafficPktBytesVariance);   
  NS_LOG_INFO ("  trafficPktBytesUpper:           " << trafficPktBytesUpper);   
  NS_LOG_INFO ("  trafficPktBytesLower:           " << trafficPktBytesLower);   
  NS_LOG_INFO ("  trafficInterPktTimeDistr:       " << trafficInterPktTimeDistr);   
  NS_LOG_INFO ("  trafficInterPktTimeMean:        " << trafficInterPktTimeMean);   
  NS_LOG_INFO ("  trafficInterPktTimeVariance:    " << trafficInterPktTimeVariance);   
  NS_LOG_INFO ("  trafficInterPktTimeUpper:       " << trafficInterPktTimeUpper);   
  NS_LOG_INFO ("  trafficInterPktTimeLower:       " << trafficInterPktTimeLower);   
  NS_LOG_INFO ("  trafficDestAddrMode:            " << trafficDestAddrMode);   
  NS_LOG_INFO ("");

  ////////////////////////////////////////////////////
  // create the simulation god
  ////////////////////////////////////////////////////
  Ptr<SimulationGod> simGod;
  simGod = Create <SimulationGod> ();
  simGod->SetStpMode(stpMode);



  ////////////////////////////////////////////////////
  // create the nodes required by the topology
  ////////////////////////////////////////////////////
  NS_LOG_INFO ("\nCREATE NODES.");
  NodeContainer bridges;
  bridges.Create (g_numNodes);
  simGod->SetNumNodes(g_numNodes);
  simGod->PrintSimInfo();



  ////////////////////////////////////////////////////
  // Create/read the physical vertex adjacency matrix
  ////////////////////////////////////////////////////
  NS_LOG_INFO ("\nBUILD TOPOLOGY - Create Phy Vertex Adjacency Matrix");
  
  // initialize the physyical (vertex adjacency) matrix
  std::vector < std::vector < int > > vertexAdjPhyMx;
  std::vector < std::vector < int > > vertexAdjPhyMxVisited;
  std::vector < int > rowTmp ;
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
    {
      rowTmp.clear();
      for (uint64_t j=0 ; j < g_numNodes ; j++ )
      {
        rowTmp.push_back(0);
      }
      vertexAdjPhyMx.push_back(rowTmp);
    }
  vertexAdjPhyMxVisited = vertexAdjPhyMx;
  
  // fill the physyical (vertex adjacency) matrix
  ReadCostMatrixFromFile(phyMxFilename, &vertexAdjPhyMx, g_numNodes);
  
  // tell God about the physical matrix
  simGod->SetVertexAdjPhyMx(&vertexAdjPhyMx);



  ////////////////////////////////////////////////////
  // connect the nodes, create the topology
  ////////////////////////////////////////////////////
  NS_LOG_INFO ("\nBUILD TOPOLOGY - EthPointToPoint Helper");
  EthPointToPointHelper ethPtpHlp;
  ethPtpHlp.SetDeviceAttribute ("DataRate", DataRateValue (linkDatarateBps));
  ethPtpHlp.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (linkDelayUs)));      


  
  ////////////////////////////////////////////////////
  // Create the eth-ptp link between bridges
  ////////////////////////////////////////////////////
  NS_LOG_INFO ("\nBUILD TOPOLOGY - Create EthPtp links between bridges");
  NetDeviceContainer bridgePorts[g_numNodes];
  Ptr<EthPointToPointChannel> ethPtPChannels[g_numNodes][g_numNodes];
  uint16_t tempPortMx[g_numNodes][g_numNodes];
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
    {
      for (uint64_t j=0 ; j < g_numNodes ; j++ )
        {
          if(vertexAdjPhyMxVisited.at(i).at(j)==0 && vertexAdjPhyMx.at(i).at(j)==1)
            {
              //NS_LOG_INFO ("  Create EthPtp links between nodes " << i << " and " << j);
              Ptr<EthPointToPointChannel> chTmp;
              NetDeviceContainer link = ethPtpHlp.Install (bridges.Get (i), bridges.Get (j), &chTmp);
              
              chTmp->SetSimulationGod(simGod);
              
              ethPtPChannels[i][j] = chTmp;
              ethPtPChannels[j][i] = chTmp;
              
              bridgePorts[i].Add (link.Get (0));
              bridgePorts[j].Add (link.Get (1));
              
              vertexAdjPhyMxVisited.at(i).at(j)=1;
              vertexAdjPhyMxVisited.at(j).at(i)=1;
              
              simGod->SetPortConnection( i, j, link.Get(0)->GetIfIndex() );
              simGod->SetPortConnection( j, i, link.Get(1)->GetIfIndex() );
              
              //this will be used later to schedule link failures in stp modules
              tempPortMx[i][j] = link.Get(0)->GetIfIndex();
              tempPortMx[j][i] = link.Get(1)->GetIfIndex();

            }
        }  
    }

  simGod->PrintPortMx();

  //Schedule failure event
  if(activateFailure==1)
    {
      if(nodeBToFail!=-1) //failure of a single links
        {
          ethPtPChannels[nodeAToFail][nodeBToFail]->ScheduleLinkFailure(MicroSeconds(linkFailureTimeUs));
        }
      else //node failure
        {
          for(uint64_t n=0 ; n < g_numNodes ; n++)
            {
              if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                {
                  ethPtPChannels[nodeAToFail][n]->ScheduleLinkFailure(MicroSeconds(linkFailureTimeUs));                  
                }
            }
        }
    }  

  ////////////////////////////////////////////////////
  // Create the bridgenetdevice's, which will do the frame forwarding, the learning...
  ////////////////////////////////////////////////////
  NS_LOG_INFO ("\nBUILD TOPOLOGY - Create BridgeNetDevice");
  BridgeHelper bridgeHlp;  // no parameters to configure in the helper
  Ptr<BridgeNetDevice> bridgeNetDev[g_numNodes];
  Mac48Address hostAddresses[g_numNodes];
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
    {
      bridgeNetDev[i]  = bridgeHlp.Install (bridges.Get (i), bridgePorts[i]);
      hostAddresses[i] = Mac48Address::Allocate ();
      bridgeNetDev[i] -> SetAddress (hostAddresses[i]);
      bridgeNetDev[i] -> SetNumNodes (g_numNodes);

    }

  ////////////////////////////////////////////////////
  // create and connect the traffic-gen/sink
  ////////////////////////////////////////////////////
  
  Ptr<BridgeTraffGen>  bridgeTraffGens[g_numNodes];
  Ptr<BridgeTraffSink> bridgeTraffSinks[g_numNodes];
        
  if(trafficActivate==1)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create BridgeTraffGen/Sink");
  
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // traffic generator
          bridgeTraffGens[i]  = Create <BridgeTraffGen> (bridges.Get(i), bridgeNetDev[i]);
          bridgeTraffGens[i]->SetPktLength(trafficPktBytesDistr, trafficPktBytesMean, trafficPktBytesVariance, trafficPktBytesUpper, trafficPktBytesLower); //bytes
          bridgeTraffGens[i]->SetInterPktTime(trafficInterPktTimeDistr, trafficInterPktTimeMean, trafficInterPktTimeVariance, trafficInterPktTimeUpper, trafficInterPktTimeLower); //seconds
          
          // set start-end times
          //bridgeTraffGens[i]->Start(Seconds(0));
          //bridgeTraffGens[i]->Stop(Seconds(0.005));
          bridgeTraffGens[i]->Start(Seconds(3-0.003));
          bridgeTraffGens[i]->Stop(Seconds(3+0.005));
          //bridgeTraffGens[i]->Start(Seconds(3-1));
          //bridgeTraffGens[i]->Stop(Seconds(3+40));

          // traffic sink
          bridgeTraffSinks[i] = Create <BridgeTraffSink> (bridges.Get(i));
          bridgeNetDev[i]->SetTraffSink(bridgeTraffSinks[i]);
        }
      
      if(trafficDestAddrMode == 0) // broadcast
        {
          // do nothing because this is the default mode    
        }

      if(trafficDestAddrMode == 1) // pairs (i <=> nodes-i)
        {
          for (uint64_t i=0 ; i < g_numNodes ; i++ )
            {
              bridgeTraffGens[i]->SetDestMacAddress(hostAddresses[g_numNodes-1-i]);
            }      
        }

      if(trafficDestAddrMode == 2) // random
        {
          NS_LOG_INFO ("\Error: Random mode for DAs not implemented yet");
          exit(0);
        }
    }
 
 
  ////////////////////////////////////////////////////
  //Create the xSTP
  ////////////////////////////////////////////////////
  Ptr<STP>      stpModules[g_numNodes];  
  Ptr<RSTP>     rstpModules[g_numNodes];
  Ptr<RSTPconf> rstpConfModules[g_numNodes];
  Ptr<SPBISIS>  spbIsIsModules[g_numNodes];
  Ptr<SPBDV>    spbDvModules[g_numNodes];
  Ptr<SPBDVconf>    spbDvConfModules[g_numNodes];   

  NS_LOG_INFO ("\nBUILD TOPOLOGY - Fill Bridge IDs array");
  uint64_t bridgeIds[g_numNodes]; // array of bridgeIds to be assigned to nodes
  bool     assigned[g_numNodes];  // aux for random ids
  double   initTimes[g_numNodes]; // time when bridges start up
   
  for (uint64_t i=0 ; i < g_numNodes ; i++ ){ assigned[i]=false; initTimes[i]=0.0;}

  // create an array with the bridgeIds to configure to the nodes
  if( (forcedRootNode!=-1 && randomBridgeIds==0) || (forcedSecondRootNode!=-1 && randomBridgeIds==0) )
    {
      NS_LOG_INFO ("ERROR: Inconsistent forcedRootNode(" << forcedRootNode << ")/forcedSecondRootNode(" << forcedSecondRootNode << ") and randomBridgeIds(" << randomBridgeIds << ")");
      exit(1);
    }
  if(forcedRootNode==forcedSecondRootNode && forcedRootNode!=-1)
    {
      NS_LOG_INFO ("ERROR: forcedRootNode(" << forcedRootNode << ") and forcedSecondRootNode(" << forcedSecondRootNode << ") cannot have the same value");
      exit(1);
    }
  if(forcedRootNode==-1 && forcedSecondRootNode!=-1)
    {
      NS_LOG_INFO ("ERROR: forcedSecondRootNode(" << forcedSecondRootNode << ") will not take effect with this value of forcedRootNode(" << forcedRootNode << ")");
      exit(1);
    }
  
  // set the bridge ids
  if(randomBridgeIds==0)
    { 
      // sequential numbers following creation of nodes
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          bridgeIds[i]=bridges.Get(i)->GetId();
        }
    }
  else
    {
      uint64_t iStart; // starting point of the index of the for-loop, editable before in case forcedRoot 
      
      // assign forced root node, if this is the case
      if(forcedRootNode!=-1)
        {
          assigned[forcedRootNode]=true;
          bridgeIds[forcedRootNode]=0;
          
          iStart=1; // start assigning bridgeId 1
          if(forcedSecondRootNode!=-1)
            {
              assigned[forcedSecondRootNode]=true;
              bridgeIds[forcedSecondRootNode]=1;
              iStart=2; // start assigning bridgeId 2
            }
        }
      else
        {
          iStart=0; // start assigning bridgeId 0
        }
        
      // fill the array with random ids
      for (uint64_t i=iStart ; i < g_numNodes ; i++ )
        {
          // select a random position
          uint64_t position = (rand()%g_numNodes);
          while(assigned[position]==true)
            {
              position = (rand()%g_numNodes);
            }
          assigned[position]=true;
          bridgeIds[position]=i;
        }      
    }

/*
bridgeIds[0]=0;
bridgeIds[1]=7;
bridgeIds[2]=5;
bridgeIds[3]=4;

bridgeIds[4]=3;
bridgeIds[5]=6;
bridgeIds[6]=1;
bridgeIds[7]=2;

bridgeIds[8]=7;
bridgeIds[9]=14;
bridgeIds[10]=1;
bridgeIds[11]=6;
bridgeIds[12]=10;
bridgeIds[13]=3;
bridgeIds[14]=5;
bridgeIds[15]=4;

NS_LOG_INFO("##################################");
NS_LOG_INFO("##################################");
NS_LOG_INFO("##################################");
NS_LOG_INFO("##################################");
NS_LOG_INFO("##### USING FORCED BRIDGE IDS ####");
NS_LOG_INFO("##################################");
NS_LOG_INFO("##################################");
NS_LOG_INFO("##################################");*/

  
  // create an array with the nodes start up times
  if(activateRandBridgeInit==1)
    {
      // we model the startup time as a random variable uniformly distributed between 0 and Max
      // Max is the time when a potential 'reset' message from the administrator is received by the furthest node in the network
        //double thop = ( (double)linkDelayUs/1000000 + (64*8)/(double)linkDatarateBps ); // time of one hop in seconds = delay + txTime of an arbitrary 'reset' packet(64 bytes)
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          //initTimes[i] = 0.0 + (rand()%g_numNodes)*thop;
          initTimes[i] = 0.0 + (((double) rand())/RAND_MAX )*xstpHelloTime;
        }     
    }
    
  // finally create xSTP objects 
  if(stpMode==1)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create STP");
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // create and init object
          stpModules[i] = Create <STP> ();
          stpModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridgeIds[i], initTimes[i], xstpSimulationTimeSecs, xstpActivatePeriodicalBpdus, xstpActivateExpirationMaxAge, xstpActivateIncreaseMessAge, xstpActivateTopologyChange, xstpMaxAge, xstpHelloTime, xstpForwDelay, txHoldPeriod, xstpMaxBpduTxHoldPeriod, simGod);
          bridgeNetDev[i]->SetBridgeId(bridgeIds[i]);
          simGod->AddSTPPointer(stpModules[i]);
          simGod->SetNodeIdBridgeIdMap((bridges.Get(i))->GetId(), bridgeIds[i]);
        }
                
      // notify link failure to stp block if link phy detection is enabled
      for (int64_t i=0 ; i < (int64_t)g_numNodes ; i++ )
        {
          if(activateFailure==1 &&  activatePhyFailDetect==1 && nodeAToFail==i )
            {
              if(nodeBToFail!=-1) //failure of a single link
                {
                  stpModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeAToFail][nodeBToFail] );
                  stpModules[nodeBToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeBToFail][nodeAToFail] );
                }
              else //node failure
                {
                  for(uint64_t n=0 ; n < g_numNodes ; n++)
                    {
                      if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                        {
                          stpModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[nodeAToFail][n] );
                          stpModules[n]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[n][nodeAToFail] );
                        }
                    }
                }         
            }     
        }
        
    }
  else if(stpMode==2)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create RSTP");
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // create and init object
          rstpModules[i] = Create <RSTP> ();
          rstpModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridgeIds[i], initTimes[i], xstpSimulationTimeSecs, xstpActivatePeriodicalBpdus, xstpActivateExpirationMaxAge, xstpActivateIncreaseMessAge, xstpActivateTopologyChange, xstpMaxAge, xstpHelloTime, xstpForwDelay, txHoldPeriod, xstpMaxBpduTxHoldPeriod, xstpResetTxHoldPeriod, simGod);    
          bridgeNetDev[i]->SetBridgeId(bridgeIds[i]);
          simGod->AddRSTPPointer(rstpModules[i]);
          simGod->SetNodeIdBridgeIdMap((bridges.Get(i))->GetId(), bridgeIds[i]);
        }
      
      // notify link failure to stp block if link phy detection is enabled
      for (int64_t i=0 ; i < (int64_t)g_numNodes ; i++ )
        {
          if(activateFailure==1 &&  activatePhyFailDetect==1 && nodeAToFail==i )
            {
              if(nodeBToFail!=-1) //failure of a single link
                {
                  rstpModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeAToFail][nodeBToFail] );
                  rstpModules[nodeBToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeBToFail][nodeAToFail] );
                }
              else //node failure
                {
                  for(uint64_t n=0 ; n < g_numNodes ; n++)
                    {
                      if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                        {
                          rstpModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[nodeAToFail][n] );
                          rstpModules[n]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[n][nodeAToFail] );

                        }
                    }
                }         
            }     
        }      
      
    } 
  else if(stpMode==5)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create RSTP");
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // create and init object
          rstpConfModules[i] = Create <RSTPconf> ();
          rstpConfModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridgeIds[i], initTimes[i], xstpSimulationTimeSecs, xstpActivatePeriodicalBpdus, xstpActivateExpirationMaxAge, xstpActivateIncreaseMessAge, xstpActivateTopologyChange, xstpMaxAge, xstpHelloTime, xstpForwDelay, txHoldPeriod, xstpMaxBpduTxHoldPeriod, xstpResetTxHoldPeriod, simGod);    
          bridgeNetDev[i]->SetBridgeId(bridgeIds[i]);
          //simGod->AddRSTPPointer(rstpModules[i]);
          simGod->SetNodeIdBridgeIdMap((bridges.Get(i))->GetId(), bridgeIds[i]);
        }
      
      // notify link failure to stp block if link phy detection is enabled
      for (int64_t i=0 ; i < (int64_t)g_numNodes ; i++ )
        {
          if(activateFailure==1 &&  activatePhyFailDetect==1 && nodeAToFail==i )
            {
              if(nodeBToFail!=-1) //failure of a single link
                {
                  rstpConfModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeAToFail][nodeBToFail] );
                  rstpConfModules[nodeBToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeBToFail][nodeAToFail] );
                }
              else //node failure
                {
                  for(uint64_t n=0 ; n < g_numNodes ; n++)
                    {
                      if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                        {
                          rstpConfModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[nodeAToFail][n] );
                          rstpConfModules[n]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[n][nodeAToFail] );

                        }
                    }
                }         
            }     
        }      
      
    } 
  else if(stpMode==3)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create SPB-ISIS");
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // create and init object
          spbIsIsModules[i] = Create <SPBISIS> ();
          spbIsIsModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridgeIds[i], initTimes[i], xstpSimulationTimeSecs, g_numNodes, simGod, spbisisActivateBpduForwarding, txHoldPeriod);    
          bridgeNetDev[i]->SetBridgeId(bridgeIds[i]);
          simGod->AddSPBISISPointer(spbIsIsModules[i]);
          simGod->SetNodeIdBridgeIdMap((bridges.Get(i))->GetId(), bridgeIds[i]);
         }
      
      // notify link failure to spb block if link phy detection is enabled
      for (int64_t i=0 ; i < (int64_t)g_numNodes ; i++ )
        {
          if(activateFailure==1 &&  activatePhyFailDetect==1 && nodeAToFail==i )
            {
              if(nodeBToFail!=-1) //failure of a single link
                {
                  spbIsIsModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeAToFail][nodeBToFail] );
                  spbIsIsModules[nodeBToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeBToFail][nodeAToFail] );
                }
              else //node failure
                {
                  for(uint64_t n=0 ; n < g_numNodes ; n++)
                    {
                      if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                        {
                          spbIsIsModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[nodeAToFail][n] );
                          spbIsIsModules[n]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[n][nodeAToFail] );

                        }
                    }
                }         
            }     
        }   
    } 
 
  else if(stpMode==4)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create SPB-DV");
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // create and init object
          spbDvModules[i] = Create <SPBDV> ();
          spbDvModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridgeIds[i], initTimes[i], xstpSimulationTimeSecs, xstpActivatePeriodicalBpdus, xstpActivateExpirationMaxAge, xstpActivateIncreaseMessAge, xstpActivateTopologyChange, spbdvActivatePathVectorDuplDetect, spbdvActivatePeriodicalStpStyle, spbdvActivateBpduMerging, xstpMaxAge, xstpHelloTime, xstpForwDelay, txHoldPeriod, xstpMaxBpduTxHoldPeriod, xstpResetTxHoldPeriod, g_numNodes, simGod); 
          bridgeNetDev[i]->SetBridgeId(bridgeIds[i]);
          //simGod->AddSPBDVPointer(spbDvModules[i]);
          simGod->SetNodeIdBridgeIdMap((bridges.Get(i))->GetId(), bridgeIds[i]);
         }
      
      // notify link failure to spb block if link phy detection is enabled
      for (int64_t i=0 ; i < (int64_t)g_numNodes ; i++ )
        {
          if(activateFailure==1 &&  activatePhyFailDetect==1 && nodeAToFail==i )
            {
              if(nodeBToFail!=-1) //failure of a single link
                {
                  spbDvModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeAToFail][nodeBToFail] );
                  spbDvModules[nodeBToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeBToFail][nodeAToFail] );
                }
              else //node failure
                {
                  for(uint64_t n=0 ; n < g_numNodes ; n++)
                    {
                      if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                        {
                          spbDvModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[nodeAToFail][n] );
                          spbDvModules[n]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[n][nodeAToFail] );
                        }
                    }
                }         
            }     
        }   
    }  
    
 
  else if(stpMode==6)
    {
      NS_LOG_INFO ("\nBUILD TOPOLOGY - Create SPB-DV Conf");
      for (uint64_t i=0 ; i < g_numNodes ; i++ )
        {
          // create and init object
          spbDvConfModules[i] = Create <SPBDVconf> ();
          spbDvConfModules[i]->Install(bridges.Get (i), bridgeNetDev[i], bridgePorts[i], bridgeIds[i], initTimes[i], xstpSimulationTimeSecs, xstpActivatePeriodicalBpdus, xstpActivateExpirationMaxAge, xstpActivateIncreaseMessAge, xstpActivateTopologyChange, spbdvActivatePathVectorDuplDetect, spbdvActivatePeriodicalStpStyle, spbdvActivateBpduMerging, xstpMaxAge, xstpHelloTime, xstpForwDelay, txHoldPeriod, xstpMaxBpduTxHoldPeriod, xstpResetTxHoldPeriod, g_numNodes, simGod); 
          bridgeNetDev[i]->SetBridgeId(bridgeIds[i]);
          //simGod->AddSPBDVPointer(spbDvModules[i]);
          simGod->SetNodeIdBridgeIdMap((bridges.Get(i))->GetId(), bridgeIds[i]);
         }
      
      // notify link failure to spb block if link phy detection is enabled
      for (int64_t i=0 ; i < (int64_t)g_numNodes ; i++ )
        {
          if(activateFailure==1 &&  activatePhyFailDetect==1 && nodeAToFail==i )
            {
              if(nodeBToFail!=-1) //failure of a single link
                {
                  spbDvConfModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeAToFail][nodeBToFail] );
                  spbDvConfModules[nodeBToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs) , tempPortMx[nodeBToFail][nodeAToFail] );
                }
              else //node failure
                {
                  for(uint64_t n=0 ; n < g_numNodes ; n++)
                    {
                      if(vertexAdjPhyMx.at(nodeAToFail).at(n)==1)
                        {
                          spbDvConfModules[nodeAToFail]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[nodeAToFail][n] );
                          spbDvConfModules[n]->ScheduleLinkFailure( MicroSeconds(linkFailureTimeUs), tempPortMx[n][nodeAToFail] );
                        }
                    }
                }         
            }     
        }   
    }  
    
    
   
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "xxx.tr"
  ////////////////////////////////////////////////////
  //NS_LOG_INFO ("Configure Tracing.");
  //std::ofstream ascii;
  //ascii.open ("examples/bridging/ptp-bridges-test.tr");
  //EthPointToPointHelper::EnableAscii (ascii, NodeContainer(bridges.Get (0)));
  //EthPointToPointHelper::EnableAsciiAll (ascii);
  
  // Now, do the actual simulation
  ////////////////////////////////////////////////////
  NS_LOG_INFO ("\nRUN SIMULATION.");
  Simulator::Run ();
  Simulator::Destroy ();
  
  for (uint64_t i=0 ; i < g_numNodes ; i++ )
  {
    for (uint64_t j=0 ; j < g_numNodes ; j++ )
      {
        if(ethPtPChannels[i][j]) ethPtPChannels[i][j]->AlternativeDoDispose();
      }
  }

  NS_LOG_INFO ("Done.");
}
