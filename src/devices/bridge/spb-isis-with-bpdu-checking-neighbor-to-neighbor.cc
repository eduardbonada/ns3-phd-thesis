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
 *
 * Author: Eduard Bonada<eduard.bonada@upf.edu>
 */

#include "spb-isis.h"
#include "bridge-net-device.h"
#include "bpdu-header.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include <stdlib.h>
#include <cmath>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("SPBISIS");

namespace ns3 {

TypeId
SPBISIS::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SPBISIS")
    .SetParent<Object> ()
    .AddConstructor<SPBISIS> ()
    ;
  return tid;
}

SPBISIS::SPBISIS()
{
  NS_LOG_FUNCTION_NOARGS ();
}
  
SPBISIS::~SPBISIS()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//////////////////////
// SPBISIS::Install //
//////////////////////
void
SPBISIS::Install (
    Ptr<Node> node,
    Ptr<BridgeNetDevice> bridge,
    NetDeviceContainer ports,
    uint64_t bridgeId,
    double initTime,
    uint16_t spbSimulationTimeSecs,
    uint64_t numNodes,
    Ptr<SimulationGod> god,
    uint16_t spbisisActivateBpduForwarding
    )
{
  m_node = node;
  m_bridgePort = bridge;
  m_simGod = god;
  m_numNodes = numNodes; 
  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBISIS: Installing SPB-ISIS with bridge Id " << bridgeId << " starting at " << initTime);
  
  // link to each of the ports of the bridge to be able to directly send messages to them
  for (NetDeviceContainer::Iterator i = ports.Begin (); i != ports.End (); ++i)
    {
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBISIS: Link to port " << (*i)->GetIfIndex() << " of node " << (*i)->GetNode()->GetId());
      m_ports.push_back (*i);
      
      //Register the m_RxStpCallback of the bridge net device in the protocol handler
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBISIS: RegisterProtocolHandler for " << (*i)->GetIfIndex());
      m_node->RegisterProtocolHandler (MakeCallback (&SPBISIS::ReceiveFromDevice, this), 9999, *i, true); // protocol = 9999 because it is the BPDUs protocol field (data is 8888)
      
      portInfo_t pI;
      m_portInfo.push_back(pI);
    }
  
  //general node variables
  m_simulationtime         = spbSimulationTimeSecs;
  m_activateBpduForwarding = spbisisActivateBpduForwarding;

  //init bridge info
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      m_bridgeInfo.rootId.push_back(0);   
      m_bridgeInfo.cost.push_back(0);
      m_bridgeInfo.rootPort.push_back(0);
    }
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      m_bridgeInfo.rootId.at(i)    = i;  
      m_bridgeInfo.cost.at(i)      = 9999;  
      m_bridgeInfo.rootPort.at(i)  = -1;      
    }
  m_bridgeInfo.bridgeId  = bridgeId;
  m_bridgeInfo.rootId.at(bridgeId)    = bridgeId;  // own tree  
  m_bridgeInfo.cost.at(bridgeId)      = 0;         // own tree
  
  //init the topology cost matrix to 0s and Infs
  std::vector < uint32_t > rowTmp;
  std::vector < uint32_t > rowTmp2;
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      rowTmp.clear();
      for(uint64_t j=0 ; j<m_numNodes ; j++)
        {
          if(i==j) rowTmp.push_back(0); //diagonal, cost 0
          else     rowTmp.push_back(999999); //cost infinite
          rowTmp2.push_back(0);
        }
      m_bridgeInfo.topCostMx.push_back(rowTmp);
      m_bridgeInfo.topVertAdjMx.push_back(rowTmp2);
      m_bridgeInfo.topPorts.push_back(999999);
    }
  //PrintTopCostMx();
  
  //init the trees vertex adjacency matrices to 0
  for(uint64_t t=0 ; t<m_numNodes ; t++)
    {
      m_bridgeInfo.treeVertAdjMx.push_back(std::vector<std::vector<uint32_t> >());
      for(uint64_t i=0 ; i<m_numNodes ; i++)
        {
          m_bridgeInfo.treeVertAdjMx[t].push_back(std::vector<uint32_t>());
          for(uint64_t j=0 ; j<m_numNodes ; j++)
            {
              m_bridgeInfo.treeVertAdjMx[t][i].push_back(0);
            }
        }
      //PrintTreeVertAdjMx(t);
    }
  
  //init lspDB - create as many vector elements as nodes, but set them as empty (sequence 0, sequences start at 1)
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      lspInfo_t emptyLsp;
      emptyLsp.seq    = 0;
      m_bridgeInfo.lspDB.push_back(emptyLsp);
    }

  //set own LSP to lspDB (no adjacencies set as no hello have been received)
  m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).bridgeId = m_bridgeInfo.bridgeId;
  m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq      = m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq + 1;
  //PrintLspDB();
  
  //init ports
  for (uint16_t i=0 ; i < m_portInfo.size () ; i++)
    {
      for(uint64_t n=0 ; n<m_numNodes ; n++)
        {
          m_portInfo[i].role.push_back("");
          m_portInfo[i].state.push_back("");
          m_portInfo[i].designatedRootId.push_back(0);
          m_portInfo[i].designatedCost.push_back(0);
          m_portInfo[i].designatedBridgeId.push_back(0);             
          m_portInfo[i].neighborRootId.push_back(0);
          m_portInfo[i].neighborCost.push_back(0);
          m_portInfo[i].neighborBridgeId.push_back(0); 
        }
      for(uint64_t n=0 ; n<m_numNodes ; n++)
        {
          m_portInfo[i].role.at(n)  = DESIGNATED_ROLE;
          m_portInfo[i].state.at(n) = DISCARDING_STATE;
          m_portInfo[i].designatedRootId.at(n)   = bridgeId;
          m_portInfo[i].designatedCost.at(n)     = 0;
          m_portInfo[i].designatedBridgeId.at(n) = bridgeId;      
                    
          //configure physical link failure detection. it will be scheduled if function called from main simulation config file
          m_portInfo[i].phyLinkFailDetectTimer.SetFunction(&SPBISIS::PhyLinkFailureDetected,this);
        }
    }

  //schedule the HELLO packets at 0sec
  m_bridgeInfo.helloTimer.SetFunction(&SPBISIS::SendHello,this);
  m_bridgeInfo.helloTimer.SetDelay(Seconds(10000));
  m_bridgeInfo.helloTimer.Schedule(Seconds(initTime));

}

////////////////////////////////
// SPBISIS::ReceiveFromDevice //
////////////////////////////////
void
SPBISIS::ReceiveFromDevice (Ptr<NetDevice> netDevice, Ptr<const Packet> pktLsp, uint16_t type, Address const &src, Address const &dst, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
    
  //extract header from packet
  BpduHeader bpdu = BpduHeader ();
  Ptr<Packet> p = pktLsp->Copy ();
  p->RemoveHeader(bpdu);

  uint16_t inPort = netDevice->GetIfIndex();
  
  if(m_portInfo[inPort].state.at(0)==DISABLED_STATE) // the port is disabled for all trees, so checking for tree 0 is enough
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Received message in Disabled port");
      exit(1);      
    }
  else
    {
      // HELLO Received
      //////////////////////////////////////////////
      if(bpdu.GetType()==HELLO_TYPE)
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Recvs HELLO [b:" << bpdu.GetBridgeId() << "|c:" << bpdu.GetHelloCost() << "] (bytes: 176 )");        
          
          // need to confirm the link cost received from the neighbour is the same than hte link cost stored locally
          
          // update own index in lspDB (seq++, add adjNodeId, add adjCost)
          m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq         = m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq + 1;
          m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.push_back(bpdu.GetBridgeId());
          m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjCost.push_back(bpdu.GetHelloCost());
          
          //update cost matrix
          m_bridgeInfo.topCostMx.at(m_bridgeInfo.bridgeId).at(bpdu.GetBridgeId()) = bpdu.GetHelloCost();
          m_bridgeInfo.topCostMx.at(bpdu.GetBridgeId()).at(m_bridgeInfo.bridgeId) = bpdu.GetHelloCost();
          
          //update vertex adjacency matrix
          m_bridgeInfo.topVertAdjMx.at(m_bridgeInfo.bridgeId).at(bpdu.GetBridgeId()) = 1;
          m_bridgeInfo.topVertAdjMx.at(bpdu.GetBridgeId()).at(m_bridgeInfo.bridgeId) = 1;
          
          //update port matrix
          m_bridgeInfo.topPorts.at(bpdu.GetBridgeId()) = inPort;
       
          //PrintTopCostMx();
          //PrintLspDB();
      
          //send LSP
          for(uint16_t p=0 ; p < m_ports.size() ; p++)
            {
              if(m_portInfo[p].state.at(0)!=DISABLED_STATE) // the port is disabled for all trees, so checking for tree 0 is enough
                SendLSP(p);
            }
          
        }

      // LSP Received
      //////////////////////////////////////////////
      else if (bpdu.GetType()==LSP_TYPE)
        {
          //tracing
          std::stringstream stringOut;   
          uint64_t numBytes=83; //83 fixed and 12 per adjacency
          stringOut << Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Recvs LSP   [b:" << bpdu.GetBridgeId() << "|s:" << bpdu.GetLspSequence() << "|a:" << bpdu.GetLspNumAdjacencies() << "]=>";
          for (uint64_t i=0 ; i < bpdu.GetLspNumAdjacencies() ; i++ )
            {
              stringOut << "[b:" << bpdu.GetAdjBridgeId(i) << "|c:" << bpdu.GetAdjCost(i) << "]";
              numBytes = numBytes + 12;
            }
          NS_LOG_INFO(stringOut.str() << " (link:" << m_ports[inPort]->GetChannel()->GetId() << ") (bytes: " << numBytes << " )");
          
          //if stored LSP for that node has higher seq => reply with own LSP
          if(bpdu.GetLspSequence() < m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq)
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Lower Sequence Number (local: " << m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq << ")");
              SendLSP(inPort);
            }
          
          //if stored LSP for that node has same seq => discard, as it has been already received
          if(bpdu.GetLspSequence() == m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq)
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Same Sequence Number (local: " << m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq << ")");
            }
               
          //if stored LSP for that node has lower seq - new LSP, then update
          if(bpdu.GetLspSequence() > m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq)
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Higher Sequence Number (local: " << m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq << ")");
              
              //update lspDB for that node (copy from received)
              m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).bridgeId = bpdu.GetBridgeId();
              m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).seq      = bpdu.GetLspSequence();
              m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).adjBridgeId.clear();
              m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).adjCost.clear();
              for(uint64_t a=0 ; a<bpdu.GetLspNumAdjacencies() ; a++)
                {
                  m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).adjBridgeId.push_back(bpdu.GetAdjBridgeId(a));
                  m_bridgeInfo.lspDB.at(bpdu.GetBridgeId()).adjCost.push_back(bpdu.GetAdjCost(a));
                }
              //PrintLspDB();
              
              //clear the top matrix rows and the columns for that node (they will be refilled with new information)
              for(uint64_t rc=0 ; rc<m_numNodes ; rc++)
                {
                  //if(rc!=m_bridgeInfo.bridgeId) //except myself, I trust on my LSP
                    //{
                      m_bridgeInfo.topCostMx.at(bpdu.GetBridgeId()).at(rc)    = 999999; 
                      m_bridgeInfo.topCostMx.at(rc).at(bpdu.GetBridgeId())    = 999999;
                      m_bridgeInfo.topVertAdjMx.at(bpdu.GetBridgeId()).at(rc) = 0;
                      m_bridgeInfo.topVertAdjMx.at(rc).at(bpdu.GetBridgeId()) = 0;
                    //}
                }
              
              //update cost mx from that node
              for(uint64_t a=0 ; a<bpdu.GetLspNumAdjacencies() ; a++)
                { 
                  //check consistency with previous values (actually only with values on own row and column)
                  //...
                  
                  //set matrix values
                  m_bridgeInfo.topCostMx.at(bpdu.GetBridgeId()).at(bpdu.GetAdjBridgeId(a)) = bpdu.GetAdjCost(a);
                  m_bridgeInfo.topCostMx.at(bpdu.GetAdjBridgeId(a)).at(bpdu.GetBridgeId()) = bpdu.GetAdjCost(a);
                  
                  //update vertex adjacency matrix
                  m_bridgeInfo.topVertAdjMx.at(bpdu.GetBridgeId()).at(bpdu.GetAdjBridgeId(a)) = 1;
                  m_bridgeInfo.topVertAdjMx.at(bpdu.GetAdjBridgeId(a)).at(bpdu.GetBridgeId()) = 1;
                }          
              PrintTopCostMx();
              PrintTopVertAdjMx();
              
              //flood the recieved LSPs to other neighbors
              for(uint16_t p=0 ; p < m_ports.size() ; p++)
                {
                  if(p!=inPort && m_portInfo[p].state.at(0)!=DISABLED_STATE)  // the port is disabled for all trees, so checking for tree 0 is enough
                    {
                      uint16_t protocol = 9999; //should be the LLC protocol
                      Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
                      Mac48Address dst = Mac48Address("01:80:C2:00:00:00");
                      Ptr<Packet> lspFlood = Create<Packet> (0);
                      lspFlood->AddHeader(bpdu);
                      
                      //tracing
                      std::stringstream stringOut;                
                      uint64_t numBytes=83; // 83 fixed plus 12 per adjacency
                      stringOut << Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "] SPBISIS: Sends LSP   [b:" << bpdu.GetBridgeId() << "|s:" << bpdu.GetLspSequence() << "|a:" << bpdu.GetLspNumAdjacencies() << "]=>";
                      for (uint64_t i=0 ; i < bpdu.GetLspNumAdjacencies() ; i++ )
                        {
                          stringOut << "[b:" << bpdu.GetAdjBridgeId(i) << "|c:" << bpdu.GetAdjCost(i) << "]";
                          numBytes = numBytes + 12;
                        }
                      NS_LOG_INFO(stringOut.str() << " (link:" << m_ports[p]->GetChannel()->GetId() << ") (bytes: " << numBytes << " )");
                      
                      // send to port
                      m_ports[p]->SendFrom (lspFlood, src, dst, protocol);
                    }
                }
 
              //compute new topology and send BPDUs if necessary
              ReconfigureBridge();
              
            } // if LSP seq is higher
        
        } // if LSP received
        
        
      // BPDU Received
      //////////////////////////////////////////////
      else if (bpdu.GetType()==BPDU_TYPE)
        { 
          if(m_activateBpduForwarding==0)
            {
              NS_LOG_ERROR("ERROR: SPB::ReceiveFromDevice() received BPDU type when mode not activated...");
              exit(-1);
            }
          else
            {
              NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Recvs BPDU (bytes: 55 )");
              
              //UpdateAgreements to check the agreement digest and wether the port can go to forwarding
              if(m_simGod->CheckSpbIsIsSameVertAdjMx(m_bridgeInfo.bridgeId, bpdu.GetBridgeId())==true)// if digests in receiving port and neighbor match
                {
                  NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Digests matching");
                  for(uint64_t t=0 ; t<m_numNodes ; t++)
                    {
                      NS_LOG_LOGIC("B" << m_bridgeInfo.bridgeId << "-P" << inPort << "-T" << t << " => desCost:" << m_portInfo[inPort].designatedCost.at(t) << " - neighCost:" << m_portInfo[inPort].neighborCost.at(t));
                      //if port is designated and port.designatedXXX is same or better than port.neighborXXX
                      if(m_portInfo[inPort].role.at(t)==DESIGNATED_ROLE && m_portInfo[inPort].designatedCost.at(t) <= m_portInfo[inPort].neighborCost.at(t))
                        {
                          NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Tree " << t << " Designated Port and DesignatedCost <= neighborCost ");
                          
                          // make port forwarding
                          if(m_portInfo[inPort].state.at(t) != FORWARDING_STATE)
                            {
                              MakeForwarding(inPort, t);
                            }
                        }

                      //if port is root and port.designatedXXX is same or worse than port.neighborXXX (should be the same)                     
                      if(m_portInfo[inPort].role.at(t)==ROOT_ROLE && m_portInfo[inPort].designatedCost.at(t) >= m_portInfo[inPort].neighborCost.at(t))
                        {
                          NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBISIS: Tree " << t << " Root Port and DesignatedCost >= neighborCost ");

                          // make port forwarding
                          if(m_portInfo[inPort].state.at(t) != FORWARDING_STATE)
                            {
                              MakeForwarding(inPort, t);
                            }
                        }
                    }
                }   
                
              // send BPDUs after recieving BPDU????
              if(m_activateBpduForwarding == 1)
                {
                  bool sendBPDU=false;
                  for(uint64_t t=0 ; t<m_numNodes ; t++)
                    {
                      //if(m_portInfo[inPort].state.at(t) != FORWARDING_STATE) sendBPDU=true;
                    }
                  if(sendBPDU) SendBPDU(inPort);
                }
            }               
        }
     
      PrintBridgeInfo();
      
      // print all nodes info  
      m_simGod->PrintBridgesInfo();
  }
}



//////////////////////////////////
// SPBISIS::ReconfigureBridge() //
//////////////////////////////////
void
SPBISIS::ReconfigureBridge ()
{
  /*
    - decides whether to compute Dijkstra or not
    - computes Dijkstra
    - discards contracts-agreements in wrong ports
    - sends BPDUs
  */

  NS_LOG_FUNCTION_NOARGS ();

  ////////////////////////////////////////////////////
  // check conditions to avoid Dijkstra computation //
  ////////////////////////////////////////////////////

  bool avoidComputing=false;
  bool notAllAdjacencies=false;
  bool oneNotConnected=false;
  bool localNotConnected=false;
  bool notAllLSPs=false;
  bool connected[m_numNodes];   
             
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      //if the node 'i' is not connected to any one
      connected[i]=false;
      for(uint64_t j=0 ; j<m_numNodes ; j++)
        {
          if(m_bridgeInfo.topVertAdjMx.at(i).at(j)==1) connected[i]=true;
        }
      
      //if we have not received all LSPs
      if(m_bridgeInfo.lspDB.at(i).seq==0)
        {
          notAllLSPs=true;
          NS_LOG_LOGIC("ReconfigureBridge: Avoiding computations because not all LSPs have been received");
        } 
    }
  
  //if the local node is not connected, avoid as well (this is redundant with first connected[] checking)
  if(connected[m_bridgeInfo.bridgeId]==false)
    {
      NS_LOG_LOGIC("ReconfigureBridge: Avoiding computations because the local node is not connected");
      localNotConnected=true;
      
      // disable ports for all trees if the local node is not connected
      for(uint64_t t=0 ; t < m_numNodes ; t++)
        {
          for(uint16_t p=0 ; p < m_ports.size() ; p++)
            {
              m_portInfo[p].role.at(t)  = ALTERNATE_ROLE;
              m_portInfo[p].state.at(t) = DISCARDING_STATE;
            } 
        }
    }
  
  //if adjacency list size does not match the number of not-Disabled ports (not all hellos received for example)
  /*uint16_t counter=0;
  for(uint16_t p=0 ; p < m_ports.size() ; p++)
    {
      if(m_portInfo[p].state.at(0)!=DISABLED_STATE) counter++;
    }
  if( m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.size() != counter )
    {
      notAllAdjacencies=true;
      NS_LOG_LOGIC("ReconfigureBridge: Avoiding computations because the adjacency list size does not match the number of ports");
    }*/
  
  //////////////////////////////////////////////////////////
  //
  // here i should check if the topology is connected, in 
  // that case we recompute dijsktra
  //
  //////////////////////////////////////////////////////////               
  

   
  /////////////////////////////////////////////////
  // recompute paths if computing is not avoided //
  /////////////////////////////////////////////////

  if(oneNotConnected==true || notAllLSPs==true || notAllAdjacencies==true || localNotConnected==true ) avoidComputing=true; 
    
  if(avoidComputing==true)
    {  
      NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] SPBISIS: Avoiding computations");                                         
    }
  else
    {
/*
      //select the root as minimum node we have in the list of LSPs
      uint64_t minRoot=999999;
      for(uint64_t i=0 ; i<m_numNodes ; i++)
        {
          //if we have an LSP from that node, and in out topology view it is connected to someone, and it has a lower id than the minimum...
          if( m_bridgeInfo.lspDB.at(i).seq!=0 && m_bridgeInfo.lspDB.at(i).bridgeId < minRoot && connected[i]==true)
            {
              minRoot = m_bridgeInfo.lspDB.at(i).bridgeId;
            }
        }
      m_bridgeInfo.rootId=minRoot;
*/      
      //recalculate paths
      NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] SPBISIS: Compute trees");
      for(uint64_t t=0 ; t<m_numNodes ; t++)
        {
          if(connected[t]==true)
            {
              ComputeTree(t);
            }
          else
            {
              NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] SPBISIS: Avoid computation of tree " << t << " because it is not connected");
              // disable ports of the not computed tree
              for(uint16_t p=0 ; p < m_ports.size() ; p++)
                {
                  m_portInfo[p].role.at(t)  = ALTERNATE_ROLE;
                  m_portInfo[p].state.at(t) = DISCARDING_STATE;
                }                 
            }
        }
      
      //notify the simGod so it can check for global transition to forwarding
      m_simGod->SetSpbIsisTopVertAdjMx(m_node->GetId(), &(m_bridgeInfo.topVertAdjMx));
      
      // send BPDUs to all ports
      if(m_activateBpduForwarding == 1)
        {
          for(uint16_t p=0 ; p < m_ports.size() ; p++)
            {
              //call SendBPDU
              SendBPDU(p);
            }
        }      
              
    } //else of avoid computing
}






////////////////////////
// SPBISIS::SendHELLO //
////////////////////////
void
SPBISIS::SendHello ()
{
  uint16_t protocol = 9999; //should be the LLC protocol
  Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
  Mac48Address dst = Mac48Address("01:80:C2:00:00:00");
  
  for(uint16_t p=0 ; p < m_ports.size() ; p++)
    {
      if(m_portInfo[p].state.at(0)!=DISABLED_STATE)
        {
          // create the LSP-HELLO as a packet of lenght 0
          Ptr<Packet> hello = Create<Packet> (0);
              
          // Add LSP-HELLO header
          BpduHeader helloFields = BpduHeader ();
          helloFields.SetType(HELLO_TYPE);
          helloFields.SetBridgeId(m_bridgeInfo.bridgeId);
          helloFields.SetHelloCost(1);
          hello->AddHeader(helloFields);
                    
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "] SPBISIS: Sends HELLO [b:" << helloFields.GetBridgeId() << "|c:" << helloFields.GetHelloCost() << "] (bytes: 176 )");
          m_ports[p]->SendFrom (hello, src, dst, protocol);
        }
    }
}


//////////////////////
// SPBISIS::SendLSP //
//////////////////////
void
SPBISIS::SendLSP (uint16_t outPort)
{  
  uint16_t protocol = 9999; //should be the LLC protocol
  Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
  Mac48Address dst = Mac48Address("01:80:C2:00:00:00");

  if(m_portInfo[outPort].state.at(0)!=DISABLED_STATE)
    {
      // create the LSP as a packet of lenght 0
      Ptr<Packet> lsp = Create<Packet> (0);
          
      // Add LSP header
      BpduHeader lspFields = BpduHeader ();
      lspFields.SetType(LSP_TYPE);
      lspFields.SetBridgeId(m_bridgeInfo.bridgeId);
      lspFields.SetLspSequence(m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq);
      lspFields.SetLspNumAdjacencies(m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.size());

      for (uint64_t i=0 ; i < m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.size() ; i++ )
        {    
          lspFields.AddAdjacency(m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.at(i), m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjCost.at(i));
        }

      lsp->AddHeader(lspFields);

      //tracing
      std::stringstream stringOut;
      uint64_t numBytes=83; //83 fixed and 12 per adjacency            
      stringOut << Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort << "] SPBISIS: Sends LSP   [b:" << lspFields.GetBridgeId() << "|s:" << lspFields.GetLspSequence() << "|a:" << lspFields.GetLspNumAdjacencies() << "]=>";
      for (uint64_t i=0 ; i < m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.size() ; i++ )
        {
          stringOut << "[b:" << m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.at(i) << "|c:" << m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjCost.at(i) << "]";
          numBytes = numBytes + 12;
        }
      NS_LOG_INFO(stringOut.str() <<" (link:" << m_ports[outPort]->GetChannel()->GetId() << ") (bytes: " << numBytes << " )");
      
      // send to port
      m_ports[outPort]->SendFrom (lsp, src, dst, protocol);
    }
}

////////////////////////
// SPBISIS::SendBPDU //
////////////////////////
void
SPBISIS::SendBPDU (uint16_t outPort)
{
  if(m_activateBpduForwarding == 1)
    {
      NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort << "] SPBISIS: Sends BPDU (bytes: 55 )");
      
      uint16_t protocol = 9999; //should be the LLC protocol
      Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
      Mac48Address dst = Mac48Address("01:80:C2:00:00:00");
                  
      // create the BPDU
      Ptr<Packet> bpdu = Create<Packet> (0);
      BpduHeader bpduFields = BpduHeader ();
      bpduFields.SetType(BPDU_TYPE);
      bpduFields.SetBridgeId(m_bridgeInfo.bridgeId);
      bpdu->AddHeader(bpduFields);


      // send to the out port
      m_ports[outPort]->SendFrom (bpdu, src, dst, protocol);

    }
  else
    {
      NS_LOG_ERROR("ERROR: SPB::SendBPDU() sending BPDU when mode not activated...");
      exit(-1);      
    }
}

/////////////////////////////////////
// SPBISIS::SetPortsToForwarding() //
/////////////////////////////////////
void
SPBISIS::SetPortsToForwarding(bool forw)
{

  if(m_activateBpduForwarding==0)
    {
      if(forw==true)
        {
          // set root and designated ports to ports to forwarding
          for(uint16_t p=0 ; p < m_ports.size() ; p++)
            {
              for(uint64_t n=0 ; n<m_numNodes ; n++)
                {
                  if( m_portInfo[p].role.at(n) == ROOT_ROLE || m_portInfo[p].role.at(n) == DESIGNATED_ROLE )
                    {
                      if(m_portInfo[p].state.at(n) != FORWARDING_STATE) MakeForwarding(p,n);
                    }
                }  
            }
        }
      else
        {
          // set all ports to discarding
          for(uint16_t p=0 ; p < m_ports.size() ; p++)
            {
              for(uint64_t n=0 ; n<m_numNodes ; n++)
                { 
                  if( m_portInfo[p].role.at(n) == ROOT_ROLE || m_portInfo[p].role.at(n) == DESIGNATED_ROLE )
                    {
                      if(m_portInfo[p].state.at(n) != DISCARDING_STATE) MakeDiscarding(p,n);
                    }
                }
            }
        }
    }
  else
    {
      if(forw==true) NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] SPBISIS: All ports can go Forwarding, but BPDU mode is activated");
    }
}


///////////////////////////////
// SPBISIS::MakeForwarding() //
///////////////////////////////
void
SPBISIS::MakeForwarding (uint16_t port, uint64_t tree)
{
  m_portInfo[port].state.at(tree)=FORWARDING_STATE;
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBISIS: FRW State (tree " << tree << ")");
  m_simGod->SetActivePort(tree, m_node->GetId(), port, true);
  m_bridgePort->SetPortState(tree, port, true); // set to forwarding
  
  PrintBridgeInfoSingleLine();
}

///////////////////////////////
// SPBISIS::MakeDiscarding() //
///////////////////////////////
void
SPBISIS::MakeDiscarding (uint16_t port, uint64_t tree)
{
  m_portInfo[port].state.at(tree)=DISCARDING_STATE;
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBISIS: DSC State (tree " << tree << ")");
  m_simGod->SetActivePort(tree, m_node->GetId(), port, false);  
  m_bridgePort->SetPortState(tree, port, false); // set to forwarding
}


////////////////////////////
// SPBISIS::ComputeTree() //
////////////////////////////
void
SPBISIS::ComputeTree (uint64_t t)
{
  /*
    - calculate the tree rooted at t, using Dijkstra
    - a set of equal cost shortest paths is computed
      - the path with lower 'sorted-id' is selected (see congruency requirements in SPB)
  */
  
  NS_LOG_LOGIC("Computing tree " << t);
  
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // declare and initialize variables
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  
  std::vector<bool>       selected(m_numNodes, false);                      // set of nodes already selected as shortest paths - initialized to 0 (not selected)
  std::vector<uint32_t>   temporaryCost(m_numNodes,999999);                 // temporary cost for each node (the node just selected updates this value in its neighbors)
  
  std::vector<int64_t>               rowTmp;                                // temporary empty array used to initialize predecessor and successor
  std::vector<std::vector<int64_t> > predecessor(m_numNodes,rowTmp);        // predecessor node (or nodes in case of multiple equal cost paths) from source in shortest path (in a tree, this would be the Designated Bridge)
  std::vector<std::vector<int64_t> > successor(m_numNodes,rowTmp);          // successor node (or nodes in case of multiple equal cost paths) from source in shortest path (In a tree, this would be the nodes to which you are designated)
  std::vector<bool>                  rowTmp2;                               // temporary empty array used to initialize visitedSuccessor
  std::vector<std::vector<bool> >    visitedSuccessor(m_numNodes,rowTmp2);  // parallel array to successors array telling whether that child has been already visited or not (involved in equal cost paths computation)
  
  std::vector<std::vector<uint64_t> > shortestPaths;                         // array including all computed shortest paths from t to any
  std::vector<std::vector<uint64_t> > electedPathsForTree;                   // array including the elected shorest paths that will create the tree

  int64_t current;                        // current aux variable used in algorithm and in equal cost depth-first search
  bool allSelected,noMoreBifurcations;    // variables used to test while termination conditions 

  bool connected[m_numNodes];
  
  //check for not connected nodes, and avoid them in the calculation
  for(uint64_t i=0; i<m_numNodes ; i++)
    {
      connected[i]=false;
      for(uint64_t j=0; j<m_numNodes ; j++)
        {
          if(m_bridgeInfo.topVertAdjMx.at(i).at(j)==1) 
            {
              connected[i]=true;
              //NS_LOG_LOGIC("connected[" << i << "]=" << connected[i]);
            }
        }
    }


  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // Dijkstra algorithm (modified to store equal cost predecessors)
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
    - As Dijkstra algorithm, it computes shortest paths from a source (t) to all other ndoes
    - Modification of Dijkstra algorithm to detect equal cost predecessors
    - the predecessor array becomes an array of arrays to incorporate several predecessors at each node
      - the optimal substructure property(subpath of shortest path is shortest path) ensures that all shortest paths paths are found using multiple predecessors of predecessors, and so on...
  */     
 
  temporaryCost[t] = 0;           // Dijkstra starting at t as source (tree root)

  allSelected=false;
  while(allSelected==false)
    {
      // finds the not selected node with the lowest cost to source (actually, because of the <, it returns the first one found)
      uint32_t min=999999;
      current=-1;
      for(uint64_t i=0; i<m_numNodes ; i++)
        {
          if(selected[i]==false && temporaryCost[i]<min && connected[i]==true)
            {
              //NS_LOG_LOGIC("\t" << "DijkstraMinNotSelected: min <= " << i << "(" << temporaryCost[i] << ")");
              min   = temporaryCost[i];
              current = i;
            }
        }
      //NS_LOG_LOGIC("\t" << "DijkstraMinNotSelected: min cost at node " << current); 
      
      if(current==-1)
        {
          NS_LOG_INFO("WARNING: DijkstraAlg computed no node with minimum cost and some nodes are still not selected");
          allSelected=true;
          //exit(1);
        }

      // add current to selected and update neighbors temporaryCost
      selected[current]=true;
      NS_LOG_LOGIC("\t" << "DijkstraAlg: selected <= " << current);
      for(uint64_t i=0 ; i<m_numNodes ; i++)
        {
          if(selected[i]==false && connected[i]==true) //if not selected yet
            {
              NS_LOG_LOGIC("\t" << "DijkstraAlg: comparing to node " << i << " => old:" << temporaryCost[i] << " - new:" << temporaryCost[current] + m_bridgeInfo.topCostMx[current][i]);        
              
              //update cost estimation it is lower than existing estimation, or equal to account for multiple equal cost paths
              if(temporaryCost[i] == temporaryCost[current] + m_bridgeInfo.topCostMx[current][i] && temporaryCost[i]!=999999)
                {
                  NS_LOG_LOGIC("\t" << "DijkstraAlg: add an equal temporary cost of " << i << "(" << temporaryCost[i] << "=>" << temporaryCost[current] + m_bridgeInfo.topCostMx[current][i] << ")");
                                    
                  //temporary cost not changed because it is equal
                  
                  //add the new predecessor
                  predecessor[i].push_back(current); 
                }
              else if(temporaryCost[i] > temporaryCost[current] + m_bridgeInfo.topCostMx[current][i])
                {
                  NS_LOG_LOGIC("\t" << "DijkstraAlg: update to lower temporary cost of " << i << "(" << temporaryCost[i] << "=>" << temporaryCost[current] + m_bridgeInfo.topCostMx[current][i] << ")");                    
                  //temporary cost updated
                  temporaryCost[i] = temporaryCost[current] + m_bridgeInfo.topCostMx[current][i];
                  
                  //since the cost is lower, clear the old list of predecessors and add the new predecessor
                  predecessor[i].clear();
                  predecessor[i].push_back(current);
                }    
            }
        }
      
      //check end-of-while (and end-of-algorithm) condition
      allSelected=true;
      for(uint64_t i=0 ; i<m_numNodes ; i++) if(selected[i]==false && connected[i]==true) allSelected=false;

    } // end of while


  ////////////////////////////////////////////////////////////////////////////////////////////////////
  //build the successor list out of predecessors list
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  for(int64_t i=0 ; i<(int64_t)m_numNodes ; i++)
    {
      for(int64_t j=0 ; j<(int64_t)m_numNodes ; j++)
        {
           if(i!=j && connected[i]==true && connected[j]==true)
             {
              for(uint64_t k=0 ; k<predecessor.at(j).size() ; k++)
                {
                  if(predecessor.at(j).at(k) == i)
                    {
                      NS_LOG_LOGIC("\t" << "DijkstraAlg: Found " << j << " as successor of " << i);
                      successor.at(i).push_back(j);
                      visitedSuccessor.at(i).push_back(false); // used in DFS
                    }
                }
            }
        }
    }
        
  //printout the predecessors
  std::stringstream stringOut;
  stringOut << "Predecessors\n";
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      stringOut << i << ": ";
      for(uint64_t j=0 ; j<predecessor.at(i).size() ; j++)
        {
          stringOut << predecessor.at(i).at(j) << "\t";          
        }
      stringOut << "\n";
    }
  //NS_LOG_LOGIC("\t" << stringOut.str());

  //printout the successors
  stringOut.str("");
  stringOut << "Successors\n";
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      stringOut << i << ": ";
      for(uint64_t j=0 ; j<successor.at(i).size() ; j++)
        {
          stringOut << successor.at(i).at(j) << "\t";          
        }
      stringOut << "\n";
    }
  //NS_LOG_LOGIC("\t" << stringOut.str());


  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // identify all equal cost shortest paths (depth-first search of the successors list)
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
    - it uses the successor list as an adjecency list of a DAG (note it only contains shortest paths)
    - it computes all paths from source t to others using a non-recursive depth-first search (using bifurcation stacks)
  */
  std::vector<uint64_t>  lastBifurcation;         // list of nodes in previous bifurcations still to be revisited
  bool foundNotVisitedSuccessor;                  // aux bool variable
  uint64_t newCurrent;

  current=t; // start itearating at t (root)
  noMoreBifurcations=false;
  while(noMoreBifurcations==false)
    {

      // recompute allVisited to test while's termination condition (compute it before marking)
      //allVisited=true;
      //for(uint64_t i=0 ; i<m_numNodes ; i++) for(uint64_t j=0 ; j<visitedSuccessor.at(i).size() ; j++) if(visitedSuccessor.at(i).at(j)==false) allVisited=false;

      NS_LOG_LOGIC("\t" << "DepthFirstSearch: Iteration with current: " << current);
      
      // look for current successors
      foundNotVisitedSuccessor=false;
      for(uint64_t i=0 ; i<visitedSuccessor.at(current).size() ; i++) if(visitedSuccessor.at(current).at(i)==false) foundNotVisitedSuccessor=true;
              
      // if current has successors that are not marked in visitedSuccessors yet
      if(foundNotVisitedSuccessor==true)
        {
          NS_LOG_LOGIC("\t" << "DepthFirstSearch:   " << current << " has not-visited successors");
          
          //add current to lastBifurcation
          NS_LOG_LOGIC("\t" << "DepthFirstSearch:   add " << current << " to lastBifurcation");
          lastBifurcation.push_back(current);
          stringOut.str(""); stringOut << "DepthFirstSearch:     LB: "; for(uint64_t i=0 ; i<lastBifurcation.size() ; i++) stringOut << lastBifurcation.at(i) << " ";    
          NS_LOG_LOGIC("\t" << stringOut.str()); 

          //select the new current to the first(or any) successor not yet visited among successors of current
          for(uint64_t i=0 ; i<visitedSuccessor.at(current).size() ; i++)
            {
              if(visitedSuccessor.at(current).at(i)==false)
                {
                  // select new current
                  newCurrent = successor.at(current).at(i);
                  
                  //mark the new current in old current's visitedSuccessors
                  visitedSuccessor.at(current).at(i)=true;
                  
                  NS_LOG_LOGIC("\t" << "DepthFirstSearch:   the new current is " << newCurrent);
                  break;
                }
            }         

          // set current for next iteration
          current = newCurrent;
      
        }
      else // if current has not successors or all successors are already marked in visitedSUccessors
        {   
          NS_LOG_LOGIC("\t" << "DepthFirstSearch: " << current << " does NOT have not-visited successors");
          
          //add a path formed by {the nodes in lastBifurcation} + {current}
          std::vector<uint64_t> path;
          for(uint64_t i=0 ; i<lastBifurcation.size() ; i++)
            {
              path.push_back(lastBifurcation.at(i));
            }
          path.push_back(current);
          shortestPaths.push_back(path);

          //remove the visitedSuccessors of current so they can be used again (equal cost paths)
          for(uint64_t i=0 ; i<visitedSuccessor.at(current).size() ; i++)
            {
              visitedSuccessor.at(current).at(i)=false;
            }
                      
          //set new current to the last element of lastBifurcation
          if(lastBifurcation.size()!=0)
            {
              current = lastBifurcation.back();
              NS_LOG_LOGIC("\t" << "DepthFirstSearch:   get newCurrent from last added in lastBifurcation: " << current);
          
              //remove last element of lastBifurcation
              lastBifurcation.pop_back();
              stringOut.str(""); stringOut << "DepthFirstSearch:     LB: "; for(uint64_t i=0 ; i<lastBifurcation.size() ; i++) stringOut << lastBifurcation.at(i) << " "; 
              NS_LOG_LOGIC("\t" << stringOut.str()); 
            }
          else
            {
              NS_LOG_LOGIC("\t" << "DepthFirstSearch:   lastBifurcation is empty, end of while");
              noMoreBifurcations=true;
            }
        }

      //printout the visitedSuccessors
      stringOut.str("");
      stringOut << "visitedSuccessor\n";
      for(uint64_t i=0 ; i<m_numNodes ; i++)
        {
          stringOut << i << ": ";
          for(uint64_t j=0 ; j<visitedSuccessor.at(i).size() ; j++)
            {
              stringOut << visitedSuccessor.at(i).at(j) << "\t";          
            }
          stringOut << "\n";
        }
      //NS_LOG_LOGIC("\t" << stringOut.str());
       
    }
    
  //printout the paths
  stringOut.str("");
  stringOut << "Paths\n";
  for(uint64_t i=0 ; i<shortestPaths.size() ; i++)
    {
      for(uint64_t j=0 ; j<shortestPaths.at(i).size() ; j++)
        {
          stringOut << shortestPaths.at(i).at(j) << "\t";          
        }
      stringOut << "\n";
    }
  //NS_LOG_LOGIC("\t" << stringOut.str());
 
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  // build the trees treeVertAdjMx selecting the equal cost path accoirding to SPB sorted tie breaking
  ////////////////////////////////////////////////////////////////////////////////////////////////////
  /*
    - it uses the paths identified in shortestPaths to build the tree vertex adjacencies matrices
    - it selects the path according to the SPB sorted tie breaking (see section 28.3)    
  */ 
  
  // select the best path for each t=>destination
  for(uint64_t d=0 ; d<m_numNodes ; d++)
    {
      if(connected[d]==true)
        {   
            NS_LOG_LOGIC("\t" << "BuildEqualCostTrees: Searching paths from " << t << " to " << d);

            std::vector<uint64_t> bestPathTmp(m_numNodes,999999); 
            std::vector<uint64_t> bestSortedTmp(m_numNodes,999999); // this will always be elected as worse because it starts with 999999
            std::vector<uint64_t> sortedPathTmp;

            // printing best path
            stringOut.str(""); stringOut << "BuildEqualCostTrees: best sorted path is: ";
            for(uint64_t i=0 ; i<bestSortedTmp.size() ; i++) stringOut << bestSortedTmp.at(i) << " ";
            NS_LOG_LOGIC("\t" << stringOut.str());
                    
            for(uint64_t p=0 ; p<shortestPaths.size() ; p++) 
              {    
                // select the paths that start at t and end in i 
                if(shortestPaths.at(p).back()==d)
                  {
                    // printing pre-sorted path
                    stringOut.str(""); stringOut << "BuildEqualCostTrees: working on path at index " << p << ": ";
                    for(uint64_t i=0 ; i<shortestPaths.at(p).size() ; i++) stringOut << shortestPaths.at(p).at(i) << " ";
                    NS_LOG_LOGIC("\t" << stringOut.str());

                    // computing and printing sorted path
                    sortedPathTmp = shortestPaths.at(p);
                    sort (sortedPathTmp.begin(), sortedPathTmp.end());
                    stringOut.str(""); stringOut << "BuildEqualCostTrees: sorted path is : ";
                    for(uint64_t i=0 ; i<sortedPathTmp.size() ; i++) stringOut << sortedPathTmp.at(i) << " ";
                    NS_LOG_LOGIC("\t" << stringOut.str());
                    
                    //compare to best so far
                    bool isBetter=false;
                    if( sortedPathTmp.size() < bestSortedTmp.size() )    //a smaller path is better
                      isBetter=true;
                    else if(sortedPathTmp.size() > bestSortedTmp.size()) //a larger path is worse
                      isBetter=false;
                    else                           //considering both have the same size, let's compare element to element
                      {
                        for(uint64_t e=0 ; e<sortedPathTmp.size() ; e++)
                          {
                            if(sortedPathTmp.at(e) < bestSortedTmp.at(e) ){ isBetter=true; break; }
                            else if(sortedPathTmp.at(e) > bestSortedTmp.at(e)){ isBetter=false; break; }
                            else{} //if equal, next element...
                          } 
                      }
                    
                    // if the new sorted is better...
                    if(isBetter==true)
                      {
                        NS_LOG_LOGIC("\t" << "BuildEqualCostTrees: new sorted path is better than stored");
                        bestSortedTmp = sortedPathTmp;
                        bestPathTmp   = shortestPaths.at(p);
                      }
                    else
                      {
                        NS_LOG_LOGIC("\t" << "BuildEqualCostTrees: new sorted path is worse than stored");
                      }
                  }  
              }

            // printing the best path
            stringOut.str(""); stringOut << "BuildEqualCostTrees: best path from " << t << " to " << d << ": ";
            for(uint64_t i=0 ; i<bestPathTmp.size() ; i++) stringOut << bestPathTmp.at(i) << " ";
            NS_LOG_LOGIC("\t" << stringOut.str()); 

            //saving the elected path to electedPathsForTree
            electedPathsForTree.push_back(bestPathTmp);   
       
        }       
    }

  //clear the treeVertAdjMx (it will be rebiuld immediately after)
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      for(uint64_t j=0 ; j<m_numNodes ; j++)
        {
          m_bridgeInfo.treeVertAdjMx.at(t).at(i).at(j) = 0;
          m_bridgeInfo.treeVertAdjMx.at(t).at(j).at(i) = 0;
        }
    }
     
  //update the treeVertAdjMx based on the elected paths in electedPathsForTree, and the port roles
  NS_LOG_LOGIC("\tBuildEqualCostTrees: update of treeVertAdjMx and select roles of links in the tree (start)");
  for(uint64_t p=0 ; p<electedPathsForTree.size() ; p++)
    {
      for(uint64_t e=0 ; e<electedPathsForTree.at(p).size()-1 ; e++)
        {
          // update the treeVertAdjMx
          m_bridgeInfo.treeVertAdjMx.at(t).at( electedPathsForTree.at(p).at(e) ).at( electedPathsForTree.at(p).at(e+1) ) = 1;
          m_bridgeInfo.treeVertAdjMx.at(t).at( electedPathsForTree.at(p).at(e+1) ).at( electedPathsForTree.at(p).at(e) ) = 1;
          
          //set the port going down the tree as a designated
          if(m_bridgeInfo.bridgeId==electedPathsForTree.at(p).at(e))
            {
              NS_LOG_LOGIC("\tBuildEqualCostTrees: port " << m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1)] << " elected as Designated");
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].role.at(t)               = DESIGNATED_ROLE;
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].designatedRootId.at(t)   = t;             
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].designatedCost.at(t)     = temporaryCost[m_bridgeInfo.bridgeId];
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].designatedBridgeId.at(t) = m_bridgeInfo.bridgeId;              
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].neighborRootId.at(t)     = t;             
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].neighborCost.at(t)       = temporaryCost[electedPathsForTree.at(p).at(e)];
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e+1) ] ].neighborBridgeId.at(t)   = electedPathsForTree.at(p).at(e);   
            }
          
          //set the port going up the tree as a root
          std::string previousRole;
          if(m_bridgeInfo.bridgeId==electedPathsForTree.at(p).at(e+1))
            {
              previousRole = m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].role.at(t);
              NS_LOG_LOGIC("\tBuildEqualCostTrees: port " << m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e)] << " elected as Root");
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].role.at(t)               = ROOT_ROLE;
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].designatedRootId.at(t)   = t;             
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].designatedCost.at(t)     = temporaryCost[electedPathsForTree.at(p).at(e)];
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].designatedBridgeId.at(t) = electedPathsForTree.at(p).at(e);
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].neighborRootId.at(t)     = t;             
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].neighborCost.at(t)       = temporaryCost[electedPathsForTree.at(p).at(e)];
              m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].neighborBridgeId.at(t)   = electedPathsForTree.at(p).at(e);              
              m_bridgeInfo.rootPort.at(t) = m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ];
              //if(m_portInfo[ m_bridgeInfo.topPorts[ electedPathsForTree.at(p).at(e) ] ].state.at(t) != DISCARDING_STATE ) MakeDiscarding(m_bridgeInfo.rootPort, t); //root ports always made discarding after being elected by computation  
            }
        }
    }
  NS_LOG_LOGIC("\tBuildEqualCostTrees: update of treeVertAdjMx and select roles of links in the tree (done)");
  
  //PrintTreeVertAdjMx(t);

  //update the bridge info
  m_bridgeInfo.cost.at(t)  = temporaryCost[m_bridgeInfo.bridgeId];
  
  //update the port roles of the links not in the elected shortest paths (decide Designated or Alternate)
  uint64_t row = m_bridgeInfo.bridgeId; //only look at own rows and cols
  NS_LOG_LOGIC("\tBuildEqualCostTrees: select roles of links not in the tree (start)");
  for(uint64_t i=0 ; i<m_numNodes ; i++)
    {
      // looking at the row - if there is a link in the physical topology, but it is not included in the tree
      if( m_bridgeInfo.topVertAdjMx.at(row).at(i)==1 && m_bridgeInfo.treeVertAdjMx.at(m_bridgeInfo.rootId.at(t)).at(row).at(i)==0 && connected[row]==true && connected[i]==true ) 
        {
          NS_LOG_LOGIC("\tBuildEqualCostTrees: deciding roles ports between " << row << " and " << i);
          //(if the cost of row is lower than the cost of i) or (if the cost of row is equal than the cost of i and row lower than i) => DESIGNATED         
          if( (temporaryCost[row] < temporaryCost[i]) || (temporaryCost[row] == temporaryCost[i] && row < i) )  
            {
              NS_LOG_LOGIC("\tBuildEqualCostTrees: port " << m_bridgeInfo.topPorts[i] << " elected as Designated");
              m_portInfo[ m_bridgeInfo.topPorts[i] ].role.at(t)               = DESIGNATED_ROLE;
              m_portInfo[ m_bridgeInfo.topPorts[i] ].designatedRootId.at(t)   = t;             
              m_portInfo[ m_bridgeInfo.topPorts[i] ].designatedCost.at(t)     = temporaryCost[m_bridgeInfo.bridgeId];
              m_portInfo[ m_bridgeInfo.topPorts[i] ].designatedBridgeId.at(t) = m_bridgeInfo.bridgeId; 
              m_portInfo[ m_bridgeInfo.topPorts[i] ].neighborRootId.at(t)     = t;             
              m_portInfo[ m_bridgeInfo.topPorts[i] ].neighborCost.at(t)       = temporaryCost[i];
              m_portInfo[ m_bridgeInfo.topPorts[i] ].neighborBridgeId.at(t)   = i; 
            }
          //(if the cost of row is higher than the cost of i) or (if the cost of row is equal than the cost of i and row higher than i) => ALTERNATE
          else if( (temporaryCost[row] > temporaryCost[i]) || (temporaryCost[row] == temporaryCost[i] && row > i) )  
            {
              NS_LOG_LOGIC("BuildEqualCostTrees: port " << m_bridgeInfo.topPorts[i] << " elected as Alternate");
              m_portInfo[ m_bridgeInfo.topPorts[i] ].role.at(t)               = ALTERNATE_ROLE;
              m_portInfo[ m_bridgeInfo.topPorts[i] ].state.at(t)              = DISCARDING_STATE;
              m_portInfo[ m_bridgeInfo.topPorts[i] ].designatedRootId.at(t)   = t;             
              m_portInfo[ m_bridgeInfo.topPorts[i] ].designatedCost.at(t)     = temporaryCost[i];
              m_portInfo[ m_bridgeInfo.topPorts[i] ].designatedBridgeId.at(t) = i;
              m_portInfo[ m_bridgeInfo.topPorts[i] ].neighborRootId.at(t)     = t;             
              m_portInfo[ m_bridgeInfo.topPorts[i] ].neighborCost.at(t)       = temporaryCost[m_bridgeInfo.bridgeId];
              m_portInfo[ m_bridgeInfo.topPorts[i] ].neighborBridgeId.at(t)   = m_bridgeInfo.bridgeId; 
              MakeDiscarding(m_bridgeInfo.topPorts[i], t);
            }
        }
       
      // since the matrices are symmetrical, i only need to look at the row
    }
  NS_LOG_LOGIC("\tBuildEqualCostTrees: select roles of links not in the tree (done)");


  //PrintTreeVertAdjMx(t);
}


//////////////////////////////////
// SPBISIS::ScheduleLinkFailure //
//////////////////////////////////
void
SPBISIS::ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port)
{  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBISIS: Setting Link Failure Physical Detection in port " << port << " at time " << linkFailureTimeUs);

  //configure messAge timer
  m_portInfo[port].phyLinkFailDetectTimer.Remove();
  m_portInfo[port].phyLinkFailDetectTimer.SetArguments(port);  
  m_portInfo[port].phyLinkFailDetectTimer.Schedule(linkFailureTimeUs); 
}


/////////////////////////////////////
// SPBISIS::PhyLinkFailureDetected //
/////////////////////////////////////
void SPBISIS::PhyLinkFailureDetected(uint16_t port)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)))
    { 
      //cancel timers
      m_portInfo[port].phyLinkFailDetectTimer.Remove();
                  
      //clear the port info of the expired port
      for(uint64_t n=0 ; n<m_numNodes ; n++)
        {
          m_portInfo[port].role.at(n)=DESIGNATED_ROLE;
          m_portInfo[port].state.at(n)=DISABLED_STATE;
          m_portInfo[port].designatedRootId.at(n) = n;
          m_portInfo[port].designatedCost.at(n) = m_bridgeInfo.cost.at(n);
          m_portInfo[port].designatedBridgeId.at(n) = m_bridgeInfo.bridgeId;
        }
        
      //infer the bridge at the other side of the failed link
      uint64_t bridgeIdFailure;
      for(uint64_t i=0 ; i<m_numNodes ; i++)
        {
          if(m_bridgeInfo.topPorts.at(i)==port)
            {
              bridgeIdFailure=i;
              break;
            }
        }

      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBISIS: Physical Link Failure Detected - Link to bridgeId " << bridgeIdFailure);      

      //Udate own LSP
      m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq = m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).seq + 1;      
      for(uint64_t i=0 ; i<m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.size() ; i++)
        {
          if(m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.at(i) == bridgeIdFailure)
            {
              m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.erase( m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjBridgeId.begin() + i );  
              m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjCost.erase( m_bridgeInfo.lspDB.at(m_bridgeInfo.bridgeId).adjCost.begin() + i );                
            }
        }
      //PrintLspDB();

      //update cost matrix
      m_bridgeInfo.topCostMx.at(m_bridgeInfo.bridgeId).at(bridgeIdFailure) = 999999;
      m_bridgeInfo.topCostMx.at(bridgeIdFailure).at(m_bridgeInfo.bridgeId) = 999999;
      PrintTopCostMx();
      
      //update vertex adjacency matrix
      m_bridgeInfo.topVertAdjMx.at(m_bridgeInfo.bridgeId).at(bridgeIdFailure) = 0;
      m_bridgeInfo.topVertAdjMx.at(bridgeIdFailure).at(m_bridgeInfo.bridgeId) = 0;
      PrintTopVertAdjMx();
      
      //update port matrix
      // ... nothing to be done?

      //notify the simGod so it can check for global transition to forwarding
      m_simGod->SetSpbIsisTopVertAdjMx(m_node->GetId(), &(m_bridgeInfo.topVertAdjMx));
      
      //Send LSPs because of update
      for(uint16_t p=0 ; p < m_ports.size() ; p++)
        {
          if(m_portInfo[p].state.at(0)!=DISABLED_STATE)
            SendLSP(p);
        }   
        
      //compute new topology and send BPDUs if necessary
      ReconfigureBridge();
    }
}

/////////////////////////////////
// SPBISIS::PrintTreeVertAdjMx //
/////////////////////////////////
void
SPBISIS::PrintTreeVertAdjMx (uint64_t t)
{
  std::stringstream stringOut;
  stringOut << "Tree " << t << "\n";
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      stringOut << "\t";
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          stringOut << m_bridgeInfo.treeVertAdjMx.at(t).at(i).at(j) << "\t";
        }
      stringOut << "\n";
    }
  NS_LOG_INFO(stringOut.str());
}

//////////////////////////////
// SPBISIS::PrintTopCostMx //
//////////////////////////////
void
SPBISIS::PrintTopCostMx ()
{
  std::stringstream stringOut;
  stringOut << "Topology Cost Mx\n";
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      stringOut << "\t";
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          stringOut << m_bridgeInfo.topCostMx.at(i).at(j) << "\t";
        }
      stringOut << "\n";
    }
  NS_LOG_LOGIC(stringOut.str());
}

////////////////////////////////
// SPBISIS::PrintTopVertAdjMx //
////////////////////////////////
void
SPBISIS::PrintTopVertAdjMx ()
{
  std::stringstream stringOut;
  stringOut << "Topology Vertex Adjacency Mx\n";
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      stringOut << "\t";
      for (uint64_t j=0 ; j < m_numNodes ; j++ )
        {
          stringOut << m_bridgeInfo.topVertAdjMx.at(i).at(j) << "\t";
        }
      stringOut << "\n";
    }
  NS_LOG_LOGIC(stringOut.str());
}

/////////////////////////
// SPBISIS::PrintLspDB //
/////////////////////////
void
SPBISIS::PrintLspDB ()
{
  std::stringstream stringOut;
  stringOut << "LSP DB\n";
  for (uint64_t i=0 ; i < m_numNodes ; i++ )
    {
      stringOut << "\tBrdg:" << m_bridgeInfo.lspDB.at(i).bridgeId << "|Seq:" << m_bridgeInfo.lspDB.at(i).seq << "\t=>\t";
      for (uint64_t j=0 ; j < m_bridgeInfo.lspDB.at(i).adjBridgeId.size() ; j++ )
        {
          stringOut << "[b:" << m_bridgeInfo.lspDB.at(i).adjBridgeId.at(j) << "|c:" << m_bridgeInfo.lspDB.at(i).adjCost.at(j) << "]\t";    
        }
      stringOut << "\n";
    }
  NS_LOG_LOGIC("\t" << stringOut.str());
}

void
SPBISIS::PrintBridgeInfo()
{
  std::stringstream stringOut;
  
  for (uint64_t n=0 ; n < m_numNodes ; n++ )
    {
      // printout the state of the bridge in a single line
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      Brdg Info   [R:" << m_bridgeInfo.rootId.at(n) << "|C:" << m_bridgeInfo.cost.at(n) << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort.at(n) << "]");
      
      // printout the state of each port in a single line
      for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
        {   
          NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]        Port Info [R:" << m_portInfo[p].designatedRootId.at(n) << "|C:" << m_portInfo[p].designatedCost.at(n) << "|B:" << m_portInfo[p].designatedBridgeId.at(n) << "]:[" << m_portInfo[p].role.at(n) << "-" << m_portInfo[p].state.at(n) << "]");
        }
    }
}


void
SPBISIS::PrintBridgeInfoSingleLine()
{

  std::stringstream stringOut;
  for (uint64_t n=0 ; n < m_numNodes ; n++ )
    {
      // printout the state of the bridge in a single line
      stringOut << "B" << m_bridgeInfo.bridgeId << " [R:" << m_bridgeInfo.rootId.at(n) << "|C:" << m_bridgeInfo.cost.at(n) << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort.at(n) << "]\n";

      // printout the state of each port in a single line
      for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
        {        
          stringOut << "    P[" << p << "] des[R:" << m_portInfo[p].designatedRootId.at(n) << "|C:" << m_portInfo[p].designatedCost.at(n) << "|B:" << m_portInfo[p].designatedBridgeId.at(n) << "]:nei[R:" << m_portInfo[p].neighborRootId.at(n) << "|C:" << m_portInfo[p].neighborCost.at(n) << "|B:" << m_portInfo[p].neighborBridgeId.at(n) << "]:[" << m_portInfo[p].role.at(n) << "-" << m_portInfo[p].state.at(n) << "]\n";
        }

    }        
  NS_LOG_LOGIC(stringOut.str());
}

void
SPBISIS::DoDispose (void)
{
  // bridge timers
    
  // port timers
  for(uint16_t p=0 ; p < m_ports.size() ; p++)
    {
    }
}  

} //namespace ns3
      
  
