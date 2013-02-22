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

#include "spb-dv.h"
#include "bridge-net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include <stdlib.h>
#include <cmath>
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("SPBDV");

namespace ns3 {

TypeId
SPBDV::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SPBDV")
    .SetParent<Object> ()
    .AddConstructor<SPBDV> ()
    .AddTraceSource ("SpbdvRcvBPDU", 
                     "Trace source indicating a BPDU has been received by the SPBDV module (still not used)",
                     MakeTraceSourceAccessor (&SPBDV::m_spbdvRcvBpdu))
    ;
  return tid;
}

SPBDV::SPBDV()
{
  NS_LOG_FUNCTION_NOARGS ();
}
  
SPBDV::~SPBDV()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//////////////////
// SPBDV::Install //
//////////////////
void
SPBDV::Install (
    Ptr<Node> node,
    Ptr<BridgeNetDevice> bridge,
    NetDeviceContainer ports,
    uint64_t bridgeId,
    double initTime,    
    uint16_t spbdvSimulationTimeSecs,
    uint16_t spbdvActivatePeriodicalBpdus,
    uint16_t spbdvActivateExpirationMaxAge,
    uint16_t spbdvActivateIncreaseMessAge,
    uint16_t spbdvActivateTopologyChange, 
    uint16_t spbdvActivatePathVectorDuplDetect,
    uint16_t spbdvActivatePeriodicalStpStyle,
    uint16_t spbdvActivateBpduMerging,
    uint16_t spbdvMaxAge,
    uint16_t spbdvHelloTime,
    uint16_t spbdvForwDelay,
    double   spbdvTxHoldPeriod,
    uint32_t spbdvMaxBpduTxHoldPeriod,
    uint32_t spbdvResetTxHoldPeriod,
    uint64_t numNodes,
    Ptr<SimulationGod> god
    )
{
  NS_LOG_FUNCTION_NOARGS ();

  m_node = node;
  m_bridge = bridge;
  
  m_bridgeInfo.messAgeInc = 1;
  
  m_numNodes = numNodes; 
  m_simGod   = god; 
  
  NS_LOG_INFO ("Node[ " << m_node->GetId() << " ] SPBDV: Installing SPBDV with bridge Id " << bridgeId << " starting at " << initTime);
          
  // link to each of the ports of the bridge to be able to directly send BPDUs to them
  for (NetDeviceContainer::Iterator i = ports.Begin (); i != ports.End (); ++i)
    {
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBDV: Link to port " << (*i)->GetIfIndex() << " of node " << (*i)->GetNode()->GetId());
      m_ports.push_back (*i);
      
      //Register the callback for each net device in the protocol handler
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBDV: RegisterProtocolHandler for " << (*i)->GetIfIndex());
      m_node->RegisterProtocolHandler (MakeCallback (&SPBDV::ReceiveFromBridge, this), 9999, *i, true); // protocol = 0 (any, because we use the field to pass portId), promisc = false
      
      portInfo_t pI;
      m_portInfo.push_back(pI);
    }
  
  //general node variables
  m_simulationtime                = spbdvSimulationTimeSecs;
  m_activatePeriodicalBpdus       = spbdvActivatePeriodicalBpdus;
  m_spbdvActivateExpirationMaxAge = spbdvActivateExpirationMaxAge;
  m_spbdvActivateIncreaseMessAge  = spbdvActivateIncreaseMessAge;
  m_spbdvActivateTopologyChange   = spbdvActivateTopologyChange;
  m_spbdvActivatePathVectorDuplDetect = spbdvActivatePathVectorDuplDetect;
  m_spbdvActivatePeriodicalStpStyle   = spbdvActivatePeriodicalStpStyle;
  m_spbdvActivateBpduMerging      = spbdvActivateBpduMerging;
  m_initTime                      = initTime;
  m_spbdvActivateTreeDeactivationV2 = 0;
  
  //init bridge info
  for(uint64_t n=0 ; n < m_numNodes ; n++)
    {
      m_bridgeInfo.rootId.push_back(n);
      m_bridgeInfo.cost.push_back(9999);
      m_bridgeInfo.rootPort.push_back(-1);
      m_bridgeInfo.activeTree.push_back(false);
    }
    
  //init bridge info to root (own tree)  
  m_bridgeInfo.cost.at(bridgeId) = 0;  
  m_bridgeInfo.activeTree.at(bridgeId) = true;
  
  m_bridgeInfo.bridgeId     = bridgeId;
  m_bridgeInfo.maxAge       = spbdvMaxAge;
  m_bridgeInfo.helloTime    = spbdvHelloTime;
  m_bridgeInfo.forwDelay    = spbdvForwDelay;
  m_bridgeInfo.txHoldPeriod        = spbdvTxHoldPeriod; //period accounting BPDUs
  m_bridgeInfo.maxBpduTxHoldPeriod = spbdvMaxBpduTxHoldPeriod; //number of bpdus per m_txHoldPeriod period
  m_bridgeInfo.resetTxHoldPeriod   = spbdvResetTxHoldPeriod; // whether the value is reset or decremented
  m_bridgeInfo.pathVector.resize(m_numNodes);
  
  //init all port info to designated - discarding
  for (uint16_t i=0 ; i < m_portInfo.size () ; i++)
    {
      // init and resize vectors
      m_portInfo[i].toLearningTimer.resize(m_numNodes);
      m_portInfo[i].toForwardingTimer.resize(m_numNodes);
      m_portInfo[i].recentRootTimer.resize(m_numNodes);
      m_portInfo[i].messAgeTimer.resize(m_numNodes);
      m_portInfo[i].lastMessAgeSchedule.resize(m_numNodes);
      m_portInfo[i].pathVector.resize(m_numNodes);
      m_portInfo[i].receivedTreeDeactivation.resize(m_numNodes);
      m_portInfo[i].consolidatedRole.resize(m_numNodes);
      m_portInfo[i].setConsolidatedRolesTimer.resize(m_numNodes);
              
      for(uint64_t n=0 ; n < m_numNodes ; n++)
        { 
          // all ports to Designated and Discarding       
          m_portInfo[i].state.push_back(DISCARDING_STATE);
          m_portInfo[i].role.push_back(DESIGNATED_ROLE);
          m_portInfo[i].designatedRootId.push_back(n);
          m_portInfo[i].designatedCost.push_back(9999);
          m_portInfo[i].designatedBridgeId.push_back(bridgeId);
          m_portInfo[i].designatedPortId.push_back(m_ports[i]->GetIfIndex());
          m_portInfo[i].messAge.push_back(0);
          m_portInfo[i].atLeastOneHold.push_back(false);

          bpduInfo_t bpduTmp; bpduTmp.rootId=n; bpduTmp.cost=9999; bpduTmp.bridgeId=bridgeId; 
          m_portInfo[i].receivedBpdu.push_back(bpduTmp);

          //configure port variables
          m_portInfo[i].proposed.push_back(false);
          m_portInfo[i].proposing.push_back(true);
          m_portInfo[i].sync.push_back(false);
          m_portInfo[i].synced.push_back(true);
          m_portInfo[i].agreed.push_back(false);
          m_portInfo[i].newInfo.push_back(false);
          
          //configure toLearningTimer and toForwardingTimer and recentRootTimer values
            //m_portInfo[i].toLearningTimer.at(n).SetFunction(&SPBDV::MakeLearning,this);
            //m_portInfo[i].toLearningTimer.at(n).SetDelay(Seconds(m_bridgeInfo.forwDelay));

            //m_portInfo[i].toForwardingTimer.at(n).SetFunction(&SPBDV::MakeForwarding,this);
            //m_portInfo[i].toForwardingTimer.at(n).SetDelay(Seconds(m_bridgeInfo.forwDelay));
          
          m_portInfo[i].recentRootTimer.at(n).SetFunction(&SPBDV::RecentRootTimerExpired,this);
          m_portInfo[i].recentRootTimer.at(n).SetDelay(Seconds(m_bridgeInfo.forwDelay));
          
          // set state and start toLearningTimer
          MakeDiscarding(i,n);
            //m_portInfo[i].toLearningTimer.at(n).SetArguments(i,n);
            //m_portInfo[i].toLearningTimer.at(n).Schedule();
          
          // configure messAgeTimer
          m_portInfo[i].messAgeTimer.at(n).SetFunction(&SPBDV::MessageAgeExpired,this);
          m_portInfo[i].messAgeTimer.at(n).SetDelay(Seconds(3*m_bridgeInfo.helloTime)); //in SPBDV(RSTP), the value does not directly depend on the received MessAge   
          m_portInfo[i].lastMessAgeSchedule.at(n)=Seconds(0);
          
          // reset the tree deactivation signal and the port consolidated role
          m_portInfo[i].receivedTreeDeactivation.at(n)=false;
          m_portInfo[i].consolidatedRole.at(n)="";
          m_portInfo[i].setConsolidatedRolesTimer.at(n).SetFunction(&SPBDV::SetConsolidatedRoles,this);
          m_portInfo[i].setConsolidatedRolesTimer.at(n).SetDelay(Seconds(3*m_bridgeInfo.helloTime));
        }
      
      //set own tree cost
      m_portInfo[i].designatedCost.at(bridgeId) = 0; 
      
      // tx Hold stuff - port level - common to all trees
      m_portInfo[i].txHoldCount = 0;

      // top Change stuff - port level - common to all trees
      m_portInfo[i].topChange = false;
      m_portInfo[i].topChangeAck = false;    
      m_portInfo[i].topChangeTimer.SetFunction(&SPBDV::TopChangeTimerExpired,this);
      m_portInfo[i].topChangeTimer.SetDelay(Seconds(m_bridgeInfo.helloTime + 1));      
            
      //configure physical link failure detection. it will be scheduled if function called from main simulation config file
      m_portInfo[i].phyLinkFailDetectTimer.SetFunction(&SPBDV::PhyLinkFailureDetected,this);
    }
    
  //configure and schedule txHoldCountTimer (bridge level)
  m_bridgeInfo.txHoldCountTimer.SetFunction(&SPBDV::ResetTxHoldCount,this);
  m_bridgeInfo.txHoldCountTimer.SetDelay(Seconds(m_bridgeInfo.txHoldPeriod));
  m_bridgeInfo.txHoldCountTimer.Schedule(Seconds(initTime));
  
  //configure and schedule HelloTimeTimer (bridge level)
  m_bridgeInfo.helloTimeTimer.SetFunction(&SPBDV::SendHelloTimeBpdu,this);
  m_bridgeInfo.helloTimeTimer.SetDelay(Seconds(m_bridgeInfo.helloTime));
  m_bridgeInfo.helloTimeTimer.Schedule(Seconds(initTime));
  
}

////////////////////////////
// SPBDV::ReceiveFromBridge //
////////////////////////////
void
SPBDV::ReceiveFromBridge (Ptr<NetDevice> netDevice, Ptr<const Packet> pktBpdu, uint16_t protocol, Address const &src, Address const &dst, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  //extract BPDU from packet
  BpduMergedHeader bpdu = BpduMergedHeader ();
  Ptr<Packet> p = pktBpdu->Copy ();
  p->RemoveHeader(bpdu);
  
  uint16_t inPort = netDevice->GetIfIndex();
  uint64_t numBytes=54; //34 ethernet fixed + 20 common to all trees + 16 per tree + 8 per path vector element per tree
  for(uint64_t nb=0 ; nb < bpdu.GetNumBpdusMerged() ; nb++)
    {
      numBytes = numBytes + 16;
      for(uint64_t npv=0 ; npv < bpdu.GetPathVectorNumBridgeIds(nb) ; npv++)
        {
          numBytes = numBytes + 8;
        }
      numBytes = numBytes - 16; // the last element of the path vector is always the bridge Id, the firts one is always the root id
    }  
  NS_LOG_INFO(Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBDV: Recvs BPDU with " << bpdu.GetNumBpdusMerged() << " merged bpdus (bytes: " << numBytes << " )");
  
  //PrintBpdu(bpdu);
    
  for(uint64_t bpduInd = 0 ; bpduInd < bpdu.GetNumBpdusMerged() ; bpduInd++ )
    {
      uint64_t tree = bpdu.GetRootId(bpduInd);

      // init temp array to store elements in path vector
      bool duplicatedPathVector=false;
      uint16_t pathVectorTmp[m_numNodes];
      for(uint16_t i=0 ; i<m_numNodes ; i++) pathVectorTmp[i]=0;
      pathVectorTmp[m_bridgeInfo.bridgeId]=1; //count local as visited

      //trace message
      std::stringstream stringOut;
      stringOut << Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] SPBDV: Recvs BPDU [R:" << bpdu.GetRootId(bpduInd) << "|C:" << bpdu.GetCost(bpduInd) << "|B:" << bpdu.GetBridgeId(bpduInd) << "|P:" << bpdu.GetPortId(bpduInd) << "|A:" << bpdu.GetMessAge(bpduInd) << "|Tc:" << bpdu.GetTcFlag(bpduInd) << "|Tca:" << bpdu.GetTcaFlag(bpduInd) << "|" << bpdu.GetRoleFlags(bpduInd) << (bpdu.GetPropFlag(bpduInd)?"P":"-") << (bpdu.GetAgrFlag(bpduInd)?"A":"-") << (bpdu.GetLrnFlag(bpduInd)?"L":"-") << (bpdu.GetFrwFlag(bpduInd)?"F":"-") << "] PV(" << bpdu.GetPathVectorNumBridgeIds(bpduInd) << ")[";
      for(uint16_t i=0 ; i < bpdu.GetPathVectorNumBridgeIds(bpduInd) ; i++)
        {
          stringOut << bpdu.GetPathVectorBridgeId(i, bpduInd);
          if(i<bpdu.GetPathVectorNumBridgeIds(bpduInd)-1) stringOut << ">";
          
          pathVectorTmp[bpdu.GetPathVectorBridgeId(i, bpduInd)]++;
          if(pathVectorTmp[bpdu.GetPathVectorBridgeId(i, bpduInd)] > 1) duplicatedPathVector=true;         
        }    
      stringOut << "] (link: " << m_ports[inPort]->GetChannel()->GetId() << ")";
      NS_LOG_INFO(stringOut.str());

      // check if the bridge has been switched on
      if( Simulator::Now() > Seconds(m_initTime) )
        {
        
          //store received bpdu in port database
          m_portInfo[inPort].receivedBpdu.at(tree).rootId = bpdu.GetRootId(bpduInd);
          m_portInfo[inPort].receivedBpdu.at(tree).cost = bpdu.GetCost(bpduInd);
          m_portInfo[inPort].receivedBpdu.at(tree).bridgeId = bpdu.GetBridgeId(bpduInd);  
          m_portInfo[inPort].receivedBpdu.at(tree).portId = bpdu.GetPortId(bpduInd);
          m_portInfo[inPort].receivedBpdu.at(tree).messAge = bpdu.GetMessAge(bpduInd);
          
          // error if BPDU received in a disabled port
          if( m_portInfo[inPort].state.at(tree)==DISABLED_STATE )
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Received BPDU in Disabled port");
              exit(1);      
            } 
          
          // check it is not expired: messageAge>maxAge
          if(m_portInfo[inPort].receivedBpdu.at(tree).messAge >= m_bridgeInfo.maxAge /* && m_spbdvActivateExpirationMaxAge == 1*/) // discard BPDU if messAge>maxAge
            {
             NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Discarded Message Age at reception    (MessAge: "<< bpdu.GetMessAge(bpduInd) << ")");
              exit(1);  
            }
          
          //check the path vector looking for duplicated elements - avoidance of count to infinity
          if(duplicatedPathVector==true && m_spbdvActivatePathVectorDuplDetect==1)
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      Path Vector with duplicated element detected, discard BPDU.");
            }
          else //process bpdu if messAge<maxAge
            {      
              int16_t bpduUpdates = UpdatesInfo(bpdu, inPort, tree, bpduInd);
                 
              // if bpdu is better from designated
              if(bpduUpdates == 1 && bpdu.GetRoleFlags(bpduInd)==DESIGNATED_ROLE)
                {      
                  //trace message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]        Better from Designated");
          
                  // cancel old timer and set a new updated timer, if this feature is activated
                  m_portInfo[inPort].messAgeTimer.at(tree).Remove();
                  m_portInfo[inPort].messAgeTimer.at(tree).SetArguments(inPort, tree);
                  m_portInfo[inPort].messAgeTimer.at(tree).Schedule();//Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
                  m_portInfo[inPort].lastMessAgeSchedule.at(tree) = Simulator::Now();
                  
                  //store the received bpdu in the port info database
                  m_portInfo[inPort].designatedRootId.at(tree)   = m_portInfo[inPort].receivedBpdu.at(tree).rootId;
                  m_portInfo[inPort].designatedCost.at(tree)     = m_portInfo[inPort].receivedBpdu.at(tree).cost;
                  m_portInfo[inPort].designatedBridgeId.at(tree) = m_portInfo[inPort].receivedBpdu.at(tree).bridgeId;
                  m_portInfo[inPort].designatedPortId.at(tree)   = m_portInfo[inPort].receivedBpdu.at(tree).portId;
                  m_portInfo[inPort].messAge.at(tree)            = m_portInfo[inPort].receivedBpdu.at(tree).messAge;
                  
                  //update port variables
                  m_portInfo[inPort].synced.at(tree)     = false;
                  m_portInfo[inPort].proposing.at(tree)  = false;
                  m_portInfo[inPort].agreed.at(tree)     = false;
                  if(bpdu.GetPropFlag(bpduInd)) m_portInfo[inPort].proposed.at(tree)  = true;
                  else m_portInfo[inPort].proposed.at(tree)=false;

                  //store the path vector
                  m_portInfo[inPort].pathVector.at(tree).clear();
                  for(uint16_t i=0 ; i < bpdu.GetPathVectorNumBridgeIds(bpduInd) ; i++)
                    {
                      m_portInfo[inPort].pathVector.at(tree).push_back(bpdu.GetPathVectorBridgeId(i, bpduInd));
                    }
                  
                  // check for tree deactivation policy (9999 cost)
                  if(m_spbdvActivateTreeDeactivationV2==1)
                    {
                      if(m_portInfo[inPort].designatedCost.at(tree) == 9999)
                        {
                          m_portInfo[inPort].receivedTreeDeactivation.at(tree)=true;
                        }
                      else
                        {
                          m_portInfo[inPort].receivedTreeDeactivation.at(tree)=false;
                        }
                    }
                                  
                  //reconfigure the bridge
                  ReconfigureBridge(tree);
                  
                  // check and process Tc flags
                  CheckProcessTcFlags(bpdu.GetTcaFlag(bpduInd), bpdu.GetTcFlag(bpduInd), inPort, m_portInfo[inPort].role.at(tree));
               
                }
              
              //if the bpdu does not updates the port information (worse) and comes from a Designated
              if(bpduUpdates == -1 && bpdu.GetRoleFlags(bpduInd)==DESIGNATED_ROLE)  
                {
                  //when it is worse information received in a root port (that is, the parent bridge has new lower info) we accept it as it was better
                  if(m_portInfo[inPort].role.at(tree)==ROOT_ROLE)
                    {
                      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Worse in Root Port (accept form parent)");
                      
                      ///////////////////////////////////////////////////////////
                      // I THINK WE NEVER ENTER HERE, PUT AN ERROR AND CHECK
                      ///////////////////////////////////////////////////////////
                      
                      // cancel old timer and set a new updated timer, if this feature is activated
                      m_portInfo[inPort].messAgeTimer.at(tree).Remove();
                      m_portInfo[inPort].messAgeTimer.at(tree).SetArguments(inPort, tree);
                      m_portInfo[inPort].messAgeTimer.at(tree).Schedule();//Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
                      m_portInfo[inPort].lastMessAgeSchedule.at(tree) = Simulator::Now();
                      
                      //store the received bpdu in the port info database
                      m_portInfo[inPort].designatedRootId.at(tree)   = m_portInfo[inPort].receivedBpdu.at(tree).rootId;
                      m_portInfo[inPort].designatedCost.at(tree)     = m_portInfo[inPort].receivedBpdu.at(tree).cost;
                      m_portInfo[inPort].designatedBridgeId.at(tree) = m_portInfo[inPort].receivedBpdu.at(tree).bridgeId;
                      m_portInfo[inPort].designatedPortId.at(tree)   = m_portInfo[inPort].receivedBpdu.at(tree).portId;
                      m_portInfo[inPort].messAge.at(tree)            = m_portInfo[inPort].receivedBpdu.at(tree).messAge;

                      //update port variables
                      m_portInfo[inPort].synced.at(tree)     = false;
                      m_portInfo[inPort].proposing.at(tree)  = false;
                      m_portInfo[inPort].agreed.at(tree)     = false;
                      if(bpdu.GetPropFlag(bpduInd)) m_portInfo[inPort].proposed.at(tree)  = true;
                      else m_portInfo[inPort].proposed.at(tree)=false;
                      
                      //store the path vector
                      m_portInfo[inPort].pathVector.at(tree).clear();
                      for(uint16_t i=0 ; i < bpdu.GetPathVectorNumBridgeIds(bpduInd) ; i++)
                        {
                          m_portInfo[inPort].pathVector.at(tree).push_back(bpdu.GetPathVectorBridgeId(i, bpduInd));
                        }
                                    
                      //reconfigure the bridge
                      ReconfigureBridge(tree);
                    }
                  else
                    {
                      //trace message
                      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Worse from Designated");
                      
                      // reply ???
                      //m_portInfo[inPort].newInfo.at(tree) = true;
                    }
                }

              //if the bpdu has the same information than the port database and comes from a Designated port
              if(bpduUpdates == 0 && bpdu.GetRoleFlags(bpduInd)==DESIGNATED_ROLE)  
                {       
                  //traces
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]        Equal from Designated");
                      
                  //update variables
                  if(bpdu.GetPropFlag(bpduInd)) m_portInfo[inPort].proposed.at(tree)=true;
                  else m_portInfo[inPort].proposed.at(tree)=false;
                  
                  //store the path vector
                  m_portInfo[inPort].pathVector.at(tree).clear();
                  for(uint16_t i=0 ; i < bpdu.GetPathVectorNumBridgeIds(bpduInd) ; i++)
                    {
                      m_portInfo[inPort].pathVector.at(tree).push_back(bpdu.GetPathVectorBridgeId(i, bpduInd));
                    }
                        
                  //update message age field (this must be updated even if the BPDU us considered equal)
                  m_portInfo[inPort].messAge.at(tree) = m_portInfo[inPort].receivedBpdu.at(tree).messAge;
                  
                  // cancel old timer and set a new updated timer
                  m_portInfo[inPort].messAgeTimer.at(tree).Remove();
                  m_portInfo[inPort].messAgeTimer.at(tree).SetArguments(inPort,tree);
                  m_portInfo[inPort].messAgeTimer.at(tree).Schedule();//Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
                  m_portInfo[inPort].lastMessAgeSchedule.at(tree) = Simulator::Now();
                  
                  // check for quick state transitions
                  if(m_bridgeInfo.activeTree.at(tree)==true) QuickStateTransitions(tree);
                  
                  // check and process Tc flags
                  CheckProcessTcFlags(bpdu.GetTcaFlag(bpduInd), bpdu.GetTcFlag(bpduInd), inPort, m_portInfo[inPort].role.at(tree));
                  
                  // disseminate if STP style: only the root of the tree sends periodical BPDUs
                  if(m_spbdvActivatePeriodicalStpStyle==1)
                    {
                      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                        {
                          if(m_portInfo[i].role.at(tree)==DESIGNATED_ROLE) m_portInfo[i].newInfo.at(tree)=true;
                        }          
                    }          
                }

              //if the bpdu is better and comes from a Root-Alternate (can this happen?)
              if( bpduUpdates == 1 && ( bpdu.GetRoleFlags(bpduInd)==ROOT_ROLE || bpdu.GetRoleFlags(bpduInd)==ALTERNATE_ROLE ) && bpdu.GetAgrFlag(bpduInd) )
                {
                  //traces
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Better from Root-Alternate (invalid agreement)");
                  
                  // reply - CHANGED!!!!!!
                  //m_portInfo[inPort].proposing.at(tree) = true;
                  //m_portInfo[inPort].newInfo.at(tree) = true;
                }
                        
              //if the equal the port information and comes from a Root-Alternate and contains an agreement flag set
              if( bpduUpdates == 0 && ( bpdu.GetRoleFlags(bpduInd)==ROOT_ROLE || bpdu.GetRoleFlags(bpduInd)==ALTERNATE_ROLE ) && bpdu.GetAgrFlag(bpduInd) )
                {
                  //traces
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Equal from Root-Alternate with Agreement");
                }

              //if the bpdu is worse and comes from a Root-Alternate (but about the same root...)
              if( bpduUpdates == -1 && ( bpdu.GetRoleFlags(bpduInd)==ROOT_ROLE || bpdu.GetRoleFlags(bpduInd)==ALTERNATE_ROLE ) && bpdu.GetAgrFlag(bpduInd) && bpdu.GetRootId(bpduInd)==m_bridgeInfo.rootId.at(tree) )
                {

                  if(m_portInfo[inPort].role.at(tree)==DESIGNATED_ROLE)
                    {                      
                      //traces
                      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Worse from Root-Alternate with Agreement (received in designated)");
                      
                      //port variables
                      m_portInfo[inPort].proposing.at(tree)=false;
                      m_portInfo[inPort].agreed.at(tree)=true;
                      //m_portInfo[inPort].newInfo.at(tree) = true;
                      
                      // check fopr quick state transitions
                      if(m_bridgeInfo.activeTree.at(tree)==true) QuickStateTransitions(tree);

                      // check and process Tc flags
                      CheckProcessTcFlags(bpdu.GetTcaFlag(bpduInd), bpdu.GetTcFlag(bpduInd), inPort, m_portInfo[inPort].role.at(tree));
                    }
                  if(m_portInfo[inPort].role.at(tree)==ROOT_ROLE || m_portInfo[inPort].role.at(tree)==ALTERNATE_ROLE)
                    {
                      //traces
                      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Worse from Root-Alternate with Agreement (received in root/alternate)");
                      
                      //port variables
                      //m_portInfo[inPort].proposing.at(tree) = true;
                      if(m_spbdvActivateTreeDeactivationV2==1) m_portInfo[inPort].newInfo.at(tree) = true;
                      
                    }
                }
              
            }//process received BPDU
          
          if(m_spbdvActivateBpduMerging==0)
            {
              for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                {
                  SendBPDU(m_ports[i], "disseminate_BPDU", tree);
                }
            }
          
        } // if bridge has been turned on
        
    } // loop for all merged BPDUs
    
  //disseminate - send BPDUs to those ports where newInfo is set
  if(m_spbdvActivateBpduMerging==1)
    {
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          SendBPDU(m_ports[i], "disseminate_BPDU", 0);
        }
    }
}


//////////////////////////////
// SPBDV::CheckProcessTcFlags //
//////////////////////////////
void
SPBDV::CheckProcessTcFlags(bool tcaFlag, bool tcFlag, uint16_t port, string role)
{
  
  // if received a BPDU with TC flag
  if (tcFlag==true)
    {
      if(m_spbdvActivateTopologyChange==1)
        {
          // if it comes from designated
          if(role == DESIGNATED_ROLE)
            {
              // set the topChangeAck so it replies with tcAck on next Bpdu, we only acknowledge the notification going uo to the root
              m_portInfo[port].topChangeAck = true;
            }
            
          // for each active port except where TC is received
          for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
            {
              if(i != port)
                {
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << i << "]      Port enters Topology Change Procedure");
                  
                  // set m_portInfo.topChange in the port so it starts sending BPDUs with TC
                  m_portInfo[i].topChange = true;
                  
                  // flush the entries of the port
                  m_bridge->FlushEntriesOfPort(i);
                  
                  // start timer of the port if is not running
                  if (m_portInfo[i].topChangeTimer.IsRunning() == false)
                    {
                      m_portInfo[i].topChangeTimer.SetArguments(i);
                      m_portInfo[i].topChangeTimer.Schedule();
                    }
                }
            }
        }
      else
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: Received TC flag when toplogy change is disabled");
          exit(1);
        }
    }
    
  // if received a BPDU with TCA flag
  if(tcaFlag==true)
    {
      if(m_spbdvActivateTopologyChange==1)
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "]      Port leaves Topology Change Procedure");
          
          // cancel port timer
          m_portInfo[port].topChangeTimer.Cancel();
          
          // reset the topChange flag of the port to stop sending BPDUs with TC
          m_portInfo[port].topChange = false;
        }
      else
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: Received TCAs flag when toplogy change is disabled");
          exit(1);
        }
    }
    
}

////////////////////
//SPBDV::UpdatesInfo// NEW
////////////////////
int16_t
SPBDV::UpdatesInfo(BpduMergedHeader bpduHdr, uint16_t port, uint64_t tree, uint64_t bpduInd)
{
  NS_LOG_FUNCTION_NOARGS ();
  
/*
  - follows the 4-step comparison, bu extended to check the path vector
    - best Root Id (nonsense in one tree per node)
    - if equal Root Id: best Cost 
    - if equal Cost: best path vector
    
    - (SPBDV)it returns as better if it received an a root port and the sender is the same designasted port, even if it is worse
*/

  int16_t returnValue=2;

  if(m_bridgeInfo.activeTree.at(tree) == false && m_portInfo[port].consolidatedRole.at(tree)=="")
    {
      NS_LOG_LOGIC("UpdatesInfo: Tree not yet activated, BPDU automatically considered better");
      return 1;
    }

  //////////////////////////  
  // trace info to compare
  //////////////////////////
  NS_LOG_LOGIC("UpdatesInfo: BPDU [R:" << bpduHdr.GetRootId(bpduInd) << "|C:" << bpduHdr.GetCost(bpduInd) << "|B:" << bpduHdr.GetBridgeId(bpduInd) << "|P:" << bpduHdr.GetPortId(bpduInd) << "] <=> Port[" << port << "]: [R:" << m_portInfo[port].designatedRootId.at(tree) << "|C:" << m_portInfo[port].designatedCost.at(tree) << "|B:" << m_portInfo[port].designatedBridgeId.at(tree) << "|P:" << m_portInfo[port].designatedPortId.at(tree) << "]");

  std::stringstream stringOut;
  if(bpduHdr.GetPathVectorNumBridgeIds(bpduInd) == 0) // no path vector received, this is a BPDU going up with agreement
    {
      stringOut << "BPDU without path vector";
    }
  else
    {
      stringOut << "BPDU Path Vector    (" << bpduHdr.GetPathVectorNumBridgeIds(bpduInd) << "): [";
      for(uint16_t i=0 ; i < bpduHdr.GetPathVectorNumBridgeIds(bpduInd) ; i++)
        {
          stringOut << bpduHdr.GetPathVectorBridgeId(i, bpduInd);
          if(i<bpduHdr.GetPathVectorNumBridgeIds(bpduInd)-1) stringOut << ">";
        }    
      stringOut << "]\n";

      if(m_portInfo[port].role.at(tree) != DESIGNATED_ROLE) // if recieved in not designated, it has a received path vector
        {
          stringOut << "Port Path Vector    (" << m_portInfo[port].pathVector.at(tree).size() << "): [";
          for(uint16_t i=0 ; i < m_portInfo[port].pathVector.at(tree).size() ; i++)
            {
              stringOut << m_portInfo[port].pathVector.at(tree).at(i);
              if(i<m_portInfo[port].pathVector.at(tree).size()-1) stringOut << ">";
            }    
          stringOut << "]";
        }
      else // if received in designated, the path vector is the root port + local
        {
          stringOut << "Port Path Vector[D] (" << m_portInfo[port].pathVector.at(tree).size() << "): [";
          for(uint16_t i=0 ; i < m_portInfo[port].pathVector.at(tree).size() ; i++)
            {
              stringOut << m_portInfo[port].pathVector.at(tree).at(i);
              if(i<m_portInfo[port].pathVector.at(tree).size()-1) stringOut << ">";
            }    
          stringOut << "]";     
        }
    }
        
  NS_LOG_LOGIC(stringOut.str());

  /////////////////////
  // start comparing
  ////////////////////      
  if(bpduHdr.GetRootId(bpduInd) < m_portInfo[port].designatedRootId.at(tree))
    {
      ///////////////////////////////////////////////////////////
      // I THINK WE NEVER ENTER HERE, PUT AN ERROR AND CHECK
      ///////////////////////////////////////////////////////////
      returnValue = 1; //useless, we should never enter here in the none tree per node mode
    }
  else
    {
      if(bpduHdr.GetRootId(bpduInd) == m_portInfo[port].designatedRootId.at(tree) && bpduHdr.GetCost(bpduInd) < m_portInfo[port].designatedCost.at(tree))
        {
          returnValue = 1;
        }
      else
        {
          if(bpduHdr.GetRootId(bpduInd) == m_portInfo[port].designatedRootId.at(tree) && bpduHdr.GetCost(bpduInd) == m_portInfo[port].designatedCost.at(tree) && BetterPathVector(true, bpduHdr, bpduInd, port, 9999, tree)==1)
            {
              returnValue = 1;
            }
          else
            {
              if(bpduHdr.GetRootId(bpduInd) == m_portInfo[port].designatedRootId.at(tree) && bpduHdr.GetCost(bpduInd) == m_portInfo[port].designatedCost.at(tree) && BetterPathVector(true, bpduHdr, bpduInd, port, 9999, tree)==0)
                {
                  returnValue = 0;
                }
            }
        }
    }
    
  //returnValue not changhed, so it is worse
  if(returnValue==2)  
    {
      returnValue = -1;
      // check if it was sent by the same designated, in this case we accept the message info (as better) - the last equality checks if it is not the own bridge (in the received agreements the designated bridge of the BPDU is the own bridge)
      if(bpduHdr.GetBridgeId(bpduInd) == m_portInfo[port].designatedBridgeId.at(tree) &&  bpduHdr.GetPortId(bpduInd) == m_portInfo[port].designatedPortId.at(tree) && m_portInfo[port].designatedBridgeId.at(tree)!=m_bridgeInfo.bridgeId && bpduHdr.GetRoleFlags(bpduInd)==DESIGNATED_ROLE)
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "]        Worse from same designated as before, so read as better...");
          returnValue = 1;
        }
    }
  
  switch(returnValue)
    {
      case 1:
        NS_LOG_LOGIC("UpdatesInfo: returns 1 (better)");
        return 1;
        break;
      case 0:
        NS_LOG_LOGIC("UpdatesInfo: returns 0 (equal)");
        return 0;
        break;
      case -1:
        NS_LOG_LOGIC("UpdatesInfo: returns -1 (worse)");
        return -1;
        break;
      default:
        NS_LOG_ERROR("ERROR in UpdatesBpdu(). Not correct value.");
        exit(1);
        break;
    }
  
  NS_LOG_ERROR("ERROR in UpdatesBpdu(). Should not get to this point.");
  exit(1);  
  return -2;
  
}

///////////////////////////
//SPBDV::BetterPathVector//
///////////////////////////
int16_t
SPBDV::BetterPathVector(bool bpduRcvd, BpduMergedHeader bpduHdr, uint64_t bpduInd, uint16_t portA, uint16_t portB, uint64_t tree)
{
  // compares two peath vectors depending on the boolean bpduRcvd:
  //    true:  bpdu  <=> portA
  //    false: portA <=> portB
  // returns wether the first PV is:
  //    1(better), 0(same), -1(worse)

  int16_t returnValue=2;     
  std::vector < uint64_t > pv1;
  std::vector < uint64_t > pv2;
       
  //////////////////////////////////////////////////////
  // store and sort the path vectors in temp variables
  //////////////////////////////////////////////////////
  if(bpduRcvd==true)
    {
      NS_LOG_LOGIC("BetterPathVector: Comparing Received BPDU and port " << portA);

      // if the received frame is about the own tree (going up, agreement)
      if(bpduHdr.GetRootId(bpduInd) == m_bridgeInfo.bridgeId) 
        {
          NS_LOG_INFO("ERROR: BetterPathVector trying to compare a frame of the own bridge (" << m_bridgeInfo.bridgeId << ")");
          exit(0);
        }

      // store, and sort, the BPDU path vector in a temp variable
      pv1.resize(bpduHdr.GetPathVectorNumBridgeIds(bpduInd));
      for(uint16_t i=0 ; i < bpduHdr.GetPathVectorNumBridgeIds(bpduInd) ; i++)
        {
          pv1.at(i) = bpduHdr.GetPathVectorBridgeId(i, bpduInd);
        }
      //pv1.push_back(m_bridgeInfo.bridgeId);
      std::sort(pv1.begin(), pv1.end());
            
      // store, and sort, the portA path vector in a temp variable (to be sorted)
      pv2.resize(m_portInfo[portA].pathVector.at(tree).size());
      pv2 = m_portInfo[portA].pathVector.at(tree);
      std::sort(pv2.begin(), pv2.end());      
    }
  else
    {
      NS_LOG_LOGIC("BetterPathVector: Comparing port " << portA << " and port " << portB);
      
      // store, and sort, the portA path vector in a temp variable
      pv1.resize(m_portInfo[portA].pathVector.at(tree).size());
      pv1 = m_portInfo[portA].pathVector.at(tree);
      //pv1.push_back(m_bridgeInfo.bridgeId);
      std::sort(pv1.begin(), pv1.end());  
      
      if(m_bridgeInfo.rootPort.at(tree) == portB) // use bridge path vector
        {          
          NS_LOG_LOGIC("BetterPathVector: portB " << portB << " is the root port (PV2 is the bridge path vector)");
          // store, and sort, the portB path vector in a temp variable (to be sorted)
          pv2.resize(m_bridgeInfo.pathVector.at(tree).size());
          pv2 = m_bridgeInfo.pathVector.at(tree);
          std::sort(pv2.begin(), pv2.end());            
        }
      else // use port path vector
        {        
          NS_LOG_LOGIC("BetterPathVector: portB " << portB << " is not the root port (PV2 is the port path vector)");
          // store, and sort, the portB path vector in a temp variable (to be sorted)
          pv2.resize(m_portInfo[portB].pathVector.at(tree).size());
          pv2 = m_portInfo[portB].pathVector.at(tree);
          std::sort(pv2.begin(), pv2.end());            
        }      
    }

  // print out the vectors fo debugging
  std::stringstream stringOut;
  stringOut << "PV1 (" << pv1.size() << "): [";
  for(uint16_t i=0 ; i < pv1.size() ; i++)
    {
      stringOut << pv1.at(i);
      if(i<pv1.size()-1) stringOut << ">";
    }    
  stringOut << "]\n";
  stringOut << "PV2 (" << pv2.size() << "): [";
  for(uint16_t i=0 ; i < pv2.size() ; i++)
    {
      stringOut << pv2.at(i);
      if(i<pv2.size()-1) stringOut << ">";
    }    
  stringOut << "]\n";
  NS_LOG_LOGIC(stringOut.str());       
  
  ////////////////////////////////////////////////
  // compare the path vectors
  ////////////////////////////////////////////////
  if(pv1.size() == 0 && pv2.size()==0) // if there is no PV...
    {
      returnValue = 0;
    }
  else if(pv1.size() == 0)
    {
      returnValue = -1;
    }
  else if(pv2.size() == 0)
    {
      returnValue = 1;
    }
  else if(pv1.size() > pv2.size()) // if the length of the BPDU's PV is longer, return -1 
    {
      returnValue = -1;
    }
  else if (pv1.size() < pv2.size()) // if the length of the ports's PV is longer, return 1 
    {
      returnValue = 1;
    }
  else  // if lengths are equal
    {        
      // loop all the sorted vectors
      for(uint64_t i=0 ; i < pv1.size() ; i++)
        {
          // compare element to element
          if(pv1.at(i) > pv2.at(i))
            {
              returnValue=-1;
              break;
            }
          if(pv1.at(i) < pv2.at(i))
            {
              returnValue=1;
              break;
            }
        }
    }
    
  // if returnValue remains 2, the path vectors are equal...
  if(returnValue==2)
    {
      returnValue=0;
    }
  
  switch(returnValue)
  {
    case 1:
      NS_LOG_LOGIC("BetterPathVector: returns 1 (pv1 is better)");
      return 1;
      break;
    case 0:
      NS_LOG_LOGIC("BetterPathVector: returns 0 (pv1 and pv2 are equal)");
      return 0;
      break;
    case -1:
      NS_LOG_LOGIC("BetterPathVector: returns -1 (pv1 is worse)");
      return -1;
      break;
    default:
      NS_LOG_ERROR("ERROR in BetterPathVector(). Not correct value.");
      exit(1);
      break;
  }

  NS_LOG_ERROR("ERROR in BetterPathVector(). Should not get to this point.");
  exit(1);  
  return -2;
}


//////////////////////////
//SPBDV::ReconfigureBridge//
//////////////////////////
void
SPBDV::ReconfigureBridge(uint64_t tree)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // activate tree
  if(m_spbdvActivateTreeDeactivationV2==1)
    {    
      // check deactivation signals received in consolidated ports
      NS_LOG_LOGIC("SPBDV: Checking the tree deactivation policy");
                
      // if consolidated is not set in one (any) of the ports (cold start situation)
      if(m_portInfo[0].consolidatedRole.at(tree)=="")
        {
          NS_LOG_LOGIC("SPBDV: Consolidated port roles are not set, assuming cold start");
          if(m_bridgeInfo.activeTree.at(tree) == false)
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Activate tree " << tree);
              m_bridgeInfo.activeTree.at(tree) = true;
              for(uint16_t p=0 ; p < m_portInfo.size() ; p++) m_portInfo[p].receivedTreeDeactivation.at(tree)=false;
            }
        }
        
      // if consolidated is set in one (any) of the ports (failure situation)
      if(m_portInfo[0].consolidatedRole.at(tree)!="")
        {
          NS_LOG_LOGIC("SPBDV: Consolidated port roles are set, assuming failure recovery");
          bool allRANotMarkedForDeactivation=true;
          bool allDNotMarkedForDeactivation=true;
          bool allRAMarkedForDeactivation=true;
          uint16_t counterRA=0, counterD=0;
          
          //bool allDMarkedForDeactivation=true;          
          for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
            {
              NS_LOG_LOGIC("SPBDV: checking whether port " << p << " is consolidated (role" << m_portInfo[p].consolidatedRole.at(tree) << ")");
              
              // a consolidated root or alternate
              if( m_portInfo[p].consolidatedRole.at(tree)==ROOT_ROLE || m_portInfo[p].consolidatedRole.at(tree)==ALTERNATE_ROLE ) 
                {
                  counterRA++;
                  
                  NS_LOG_LOGIC("SPBDV: port " << p << " is a consolidated " << m_portInfo[p].consolidatedRole.at(tree) << " and is marked (" << m_portInfo[p].receivedTreeDeactivation.at(tree) << ")");
                  
                  if( m_portInfo[p].receivedTreeDeactivation.at(tree)==false ) allRANotMarkedForDeactivation = allRANotMarkedForDeactivation && true;
                  else allRANotMarkedForDeactivation = allRANotMarkedForDeactivation && false;
                  
                  if( m_portInfo[p].receivedTreeDeactivation.at(tree)==true ) allRAMarkedForDeactivation = allRAMarkedForDeactivation && true;
                  else allRAMarkedForDeactivation = allRAMarkedForDeactivation && false;
                }
              
              // a consolidated designated
              else if(m_portInfo[p].consolidatedRole.at(tree)==DESIGNATED_ROLE)
                {
                  counterD++;
                  
                  if( m_portInfo[p].receivedTreeDeactivation.at(tree)==false ) allDNotMarkedForDeactivation = allDNotMarkedForDeactivation && true;
                  else allDNotMarkedForDeactivation = allDNotMarkedForDeactivation && false;  
                }
            }

          NS_LOG_LOGIC("SPBDV: allRANotMarkedForDeactivation(" << allRANotMarkedForDeactivation << ")");
          NS_LOG_LOGIC("SPBDV: allDNotMarkedForDeactivation (" << allDNotMarkedForDeactivation << ")");
          NS_LOG_LOGIC("SPBDV: allRAMarkedForDeactivation(" << allRAMarkedForDeactivation << ")");
          NS_LOG_LOGIC("SPBDV: counterRA(" << counterRA << ")");          
          NS_LOG_LOGIC("SPBDV: counterD(" << counterD << ")");          
          //NS_LOG_LOGIC("SPBDV: allDNotMarkedForDeactivation (" << allDNotMarkedForDeactivation << ")");

          if( (allRANotMarkedForDeactivation==true && counterRA!=0) || (allDNotMarkedForDeactivation==true && counterD!=0) )
            {
              if(m_bridgeInfo.activeTree.at(tree) == false)
                {
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Activate tree " << tree);
                  m_bridgeInfo.activeTree.at(tree) = true;
                  //for(uint16_t p=0 ; p < m_portInfo.size() ; p++) m_portInfo[p].receivedTreeDeactivation.at(tree)=false;
                }          
            }
          else if( (allRAMarkedForDeactivation==true && counterRA!=0) )
            {
              if(m_bridgeInfo.activeTree.at(tree) == true)
                {
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Deactivate tree " << tree);
                  m_bridgeInfo.activeTree.at(tree)=false;        
                
                  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
                    {
                      for(uint64_t t=0 ; t < m_numNodes ; t++)
                        {
                          m_portInfo[p].toLearningTimer.at(tree).Remove();
                          m_portInfo[p].toForwardingTimer.at(tree).Remove();
                          m_portInfo[p].recentRootTimer.at(tree).Remove();
                          m_portInfo[p].messAgeTimer.at(tree).Remove();                          
                        }
                    }

                  // set cost of 9999 to the bridge cost and to designated port costs
                  m_bridgeInfo.cost.at(tree)=9999;
                  for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                    {
                      if(m_portInfo[i].role.at(tree)==DESIGNATED_ROLE)
                        {
                          m_portInfo[i].designatedCost.at(tree) = 9999;
                          m_portInfo[i].newInfo.at(tree) = true; // mark new info to disseminate infinite cost
                        }
                      m_portInfo[i].receivedTreeDeactivation.at(tree)=true;
                    }  
                }
            }
        }      
    }
  else
    {
      // just activate
      if(m_bridgeInfo.activeTree.at(tree) == false)
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Activate tree " << tree);
          m_bridgeInfo.activeTree.at(tree) = true;
        }
    }
  
  if(m_bridgeInfo.activeTree.at(tree) == true)
    {           
      //temp variables to select new root port
      int32_t rootPortTmp=-1;
      uint64_t minRootId = 10000;
      uint32_t minCost = 10000;
      uint64_t minBridgeId = 10000;
      uint16_t minPortId = 10000;
      
      std::string previous_role;    
      
      BpduMergedHeader BpduHdrTmp = BpduMergedHeader ();
          
      //check if the root of the tree is considered dead (based on ports with cost of 9999)
      bool rootAlive=false;
      bool allDesignateds=true;  
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          if(m_portInfo[i].designatedRootId.at(tree) <= minRootId && m_portInfo[i].designatedBridgeId.at(tree)!=m_bridgeInfo.bridgeId)
            {
              allDesignateds = false;
              if(m_portInfo[i].designatedCost.at(tree) < 9999) // if the cost is 9999, the root of the tree is considered dead through that port
                {
                  minRootId=m_portInfo[i].designatedRootId.at(tree);
                  rootAlive = rootAlive || true;
                }
              else
                {
                  rootAlive = rootAlive || false;
                }
            }
        }
        
      if(rootAlive == false && allDesignateds==false) // if no more trace of the root, the root of the tree is considered dead
        { 
          if(m_bridgeInfo.activeTree.at(tree) == true)
            {
              // set cost of 9999 to the bridge cost and to designated port costs
              m_bridgeInfo.cost.at(tree)=9999;
              for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                {
                  if(m_portInfo[i].role.at(tree)==DESIGNATED_ROLE)
                    {
                      m_portInfo[i].designatedCost.at(tree) = 9999;
                      m_portInfo[i].newInfo.at(tree) = true; // mark new info to disseminate infinite cost
                    }
                  m_portInfo[i].receivedTreeDeactivation.at(tree)=true;
                }
              
              // deactivate the tree until a new frame with better info is received (and cancel timers for that tree)
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Deactivate tree " << tree);
              m_bridgeInfo.activeTree.at(tree)=false;
              for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
                {
                  m_portInfo[p].toLearningTimer.at(tree).Remove();
                  m_portInfo[p].toForwardingTimer.at(tree).Remove();
                  m_portInfo[p].recentRootTimer.at(tree).Remove();
                  m_portInfo[p].messAgeTimer.at(tree).Remove();
                }
            }
        }
      else
        { 
          NS_LOG_LOGIC("ReconfigureBridge: Selecting root port...");
          
          //////////////////////////////
          //select the root port
          //////////////////////////////
          if(minRootId == m_bridgeInfo.bridgeId)   //computing our tree, so no root port
            {
              m_bridgeInfo.rootPort.at(tree)=-1;
              m_bridgeInfo.cost.at(tree)=0; 
            }
          else //computing other trees, select root port
            {
              m_bridgeInfo.rootPort.at(tree)=-1; // initial reset
              
              for(uint16_t i=0 ; i < m_portInfo.size() ; i++) //from all ports...
                {
                  if(m_portInfo[i].state.at(tree)!=DISABLED_STATE)
                    {
                      if(m_portInfo[i].designatedRootId.at(tree) == m_bridgeInfo.rootId.at(tree) && m_portInfo[i].designatedBridgeId.at(tree)!=m_bridgeInfo.bridgeId) //...that are aware of the right minimum root and are not one of the designated ports (because these have local info)
                        { 
                          if(m_portInfo[i].designatedCost.at(tree) < minCost) // cost is lower
                            {
                              rootPortTmp = i;
                              minCost = m_portInfo[i].designatedCost.at(tree);
                              minBridgeId = m_portInfo[i].designatedBridgeId.at(tree);
                              minPortId = m_portInfo[i].designatedPortId.at(tree);
                            }
                          else // cost is higher or equal
                            {
                              if(m_portInfo[i].designatedCost.at(tree) == minCost && BetterPathVector(false, BpduHdrTmp, 0, i, (uint16_t)rootPortTmp, tree)==1) // cost is equal and path vector of port i is better than pathVector in rootPortTmp
                                {
                                  rootPortTmp = i;
                                  minCost = m_portInfo[i].designatedCost.at(tree);
                                  minBridgeId = m_portInfo[i].designatedBridgeId.at(tree);
                                  minPortId = m_portInfo[i].designatedPortId.at(tree); 
                                }
                              // here I removed stuff related to select based on the portIds
                            }
                        } // if (...that are aware of the right minimum root
                    }
                } //for (all ports)
                    
              if(rootPortTmp == -1) //no minimum root has been found => tree is dead?
                {  
                  if(m_bridgeInfo.bridgeId != m_bridgeInfo.rootId.at(tree))
                    {
                      if(m_bridgeInfo.activeTree.at(tree) == true)
                        {
                          // deactivate the tree until a new frame with better info is received (and cancel timers for that tree)
                          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Deactivate tree " << tree);
                          m_bridgeInfo.activeTree.at(tree)=false;
                          for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
                            {
                              for(uint64_t t=0 ; t < m_numNodes ; t++)
                                {
                                  m_portInfo[p].toLearningTimer.at(tree).Remove();
                                  m_portInfo[p].toForwardingTimer.at(tree).Remove();
                                  m_portInfo[p].recentRootTimer.at(tree).Remove();
                                  m_portInfo[p].messAgeTimer.at(tree).Remove();
                                }
                            }
                          
                          // set cost of 9999 to the bridge cost and to designated port costs
                          m_bridgeInfo.cost.at(tree)=9999;
                          for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                            {
                              if(m_portInfo[i].role.at(tree)==DESIGNATED_ROLE)
                                {
                                  m_portInfo[i].designatedCost.at(tree) = 9999;
                                  m_portInfo[i].newInfo.at(tree) = true; // mark new info to disseminate infinite cost
                                }
                              m_portInfo[i].receivedTreeDeactivation.at(tree)=true;
                            }  
                        }
                    }
                }
              else
                {
                  NS_LOG_INFO( Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]        Reconfiguring Tree " << tree );
                  NS_LOG_LOGIC("ReconfigureBridge: I am not the root, so I set root port (" << rootPortTmp << ") info");             
                  std::string previous_role_rootPort = m_portInfo[rootPortTmp].role.at(tree);
                  m_bridgeInfo.rootPort.at(tree) = rootPortTmp;
                  m_portInfo[m_bridgeInfo.rootPort.at(tree)].role.at(tree) = ROOT_ROLE;
                  m_bridgeInfo.cost.at(tree) = minCost + 1;
                  if(previous_role_rootPort == ALTERNATE_ROLE) 
                    {
                      NS_LOG_LOGIC("ReconfigureBridge: The new root port(" << rootPortTmp << ") was alternate, so start toLearning timer");             
                      MakeDiscarding(rootPortTmp, tree);
                      //m_portInfo[rootPortTmp].toLearningTimer.at(tree).SetArguments((uint16_t)rootPortTmp, tree);  
                      //m_portInfo[rootPortTmp].toLearningTimer.at(tree).Schedule();
                    }
                  if(m_portInfo[rootPortTmp].recentRootTimer.at(tree).IsRunning()) // if the port is elected again as root, reset the recent root timer
                    {
                      m_portInfo[rootPortTmp].recentRootTimer.at(tree).Remove();             
                    }
                  m_bridgeInfo.pathVector.at(tree) = m_portInfo[m_bridgeInfo.rootPort.at(tree)].pathVector.at(tree);
                  m_bridgeInfo.pathVector.at(tree).push_back(m_bridgeInfo.bridgeId);
                }    
              
            } //else (the root is someone else)

          NS_LOG_LOGIC("ReconfigureBridge: found root port " << m_bridgeInfo.rootPort.at(tree));

          ////////////////////////////////////////
          //from others, select designated port(s)
          ////////////////////////////////////////
          for(uint16_t i=0 ; i < m_portInfo.size() ; i++) //for all ports
            {
              if(m_portInfo[i].state.at(tree)!=DISABLED_STATE && m_bridgeInfo.activeTree.at(tree)==true)
                {
                  if(i != m_bridgeInfo.rootPort.at(tree))  //except the root port (above selected)
                    { 
                      if(m_portInfo[i].designatedRootId.at(tree) != m_bridgeInfo.rootId.at(tree) || m_portInfo[i].designatedBridgeId.at(tree)==m_bridgeInfo.bridgeId ) //not completely updated information, or info from the same bridge
                        { 
                          previous_role = m_portInfo[i].role.at(tree);
                          m_portInfo[i].role.at(tree)=DESIGNATED_ROLE;
                        }
                      else
                        {
                          if(m_portInfo[i].designatedCost.at(tree) > m_bridgeInfo.cost.at(tree)) // if cost stored in port is larger than bridge cost, designated port
                            {
                              previous_role = m_portInfo[i].role.at(tree);
                              m_portInfo[i].role.at(tree)=DESIGNATED_ROLE;
                            }
                          else
                            {
                              if(m_portInfo[i].designatedCost.at(tree) < m_bridgeInfo.cost.at(tree)) // if cost stored in ports is smaller than bridge cost, alternate port
                                {
                                  previous_role = m_portInfo[i].role.at(tree);
                                  m_portInfo[i].role.at(tree)=ALTERNATE_ROLE;
                                }
                                
                              if(m_portInfo[i].designatedCost.at(tree) == m_bridgeInfo.cost.at(tree) && BetterPathVector(false, BpduHdrTmp, 0, i, m_bridgeInfo.rootPort.at(tree), tree)==-1 ) // if cost is equal and path vector stored in the port is worse than local (root port)
                                {
                                  previous_role = m_portInfo[i].role.at(tree);
                                  m_portInfo[i].role.at(tree)=DESIGNATED_ROLE;
                                }
                              else
                                {
                                  if(m_portInfo[i].designatedCost.at(tree)  == m_bridgeInfo.cost.at(tree) && BetterPathVector(false, BpduHdrTmp, 0, i, m_bridgeInfo.rootPort.at(tree), tree)==1 ) // if cost is equal and path vector stored in the port is better than local (root port)
                                    {
                                      previous_role = m_portInfo[i].role.at(tree);
                                      m_portInfo[i].role.at(tree)=ALTERNATE_ROLE;
                                    }
                                  if(m_portInfo[i].designatedCost.at(tree)  == m_bridgeInfo.cost.at(tree) && BetterPathVector(false, BpduHdrTmp, 0, i, m_bridgeInfo.rootPort.at(tree), tree)==0 ) // cost and path vector are equal
                                    {
                                      previous_role = m_portInfo[i].role.at(tree);
                                      m_portInfo[i].role.at(tree)=DESIGNATED_ROLE;                                  
                                    }
                                    
                                  // here I removed some stuff related to portId tie-breaking
                                  
                                }    
                            }   
                        } //else (not completely updated information)
                              
                      if(m_portInfo[i].role.at(tree)==DESIGNATED_ROLE) //if designated, edit port variables and save bridge information
                        { 
                          NS_LOG_LOGIC("ReconfigureBridge: found designated port " << i);
                          
                          //for those Designateds 'newly' elected
                          if( m_portInfo[i].designatedRootId.at(tree) != m_bridgeInfo.rootId.at(tree) || m_portInfo[i].designatedCost.at(tree) != m_bridgeInfo.cost.at(tree) || m_portInfo[i].designatedBridgeId.at(tree) != m_bridgeInfo.rootId.at(tree))// || m_portInfo[i].designatedPortId != i)
                            {
                              NS_LOG_LOGIC("ReconfigureBridge:      reset variables " << i);
                              m_portInfo[i].synced.at(tree)    = false;
                              m_portInfo[i].proposing.at(tree) = false;
                              m_portInfo[i].proposed.at(tree)  = false;
                              m_portInfo[i].agreed.at(tree)    = false;
                              m_portInfo[i].newInfo.at(tree)   = true;
                            }
                          
                          // save info from bridge to port
                          NS_LOG_LOGIC("ReconfigureBridge:      save info from bridge to port" << i);
                          m_portInfo[i].designatedPortId.at(tree) = i;
                          m_portInfo[i].designatedBridgeId.at(tree) = m_bridgeInfo.bridgeId;
                          m_portInfo[i].designatedRootId.at(tree) = m_bridgeInfo.rootId.at(tree);
                          m_portInfo[i].designatedCost.at(tree) = m_bridgeInfo.cost.at(tree);
                          m_portInfo[i].messAgeTimer.at(tree).Remove(); // cancel messAge timer
                          
                          // update the path vector, but what to store? let's store the root port path vector... + local!!!
                          m_portInfo[i].pathVector.at(tree).clear();
                          if(m_bridgeInfo.rootPort.at(tree)!=-1)
                            {
                              for(uint16_t j=0 ; j < m_portInfo[m_bridgeInfo.rootPort.at(tree)].pathVector.at(tree).size() ; j++)
                                {
                                  m_portInfo[i].pathVector.at(tree).push_back(m_portInfo[m_bridgeInfo.rootPort.at(tree)].pathVector.at(tree).at(j));
                                }
                              m_portInfo[i].pathVector.at(tree).push_back(m_bridgeInfo.bridgeId);
                            }   
                        }
                                
                    } //if (... except the root port)
                    
                    // apply state change, timers... in here for the port (designated or alternate) being looped now
                    if(previous_role == ROOT_ROLE && m_portInfo[i].role.at(tree) != ROOT_ROLE)
                      {
                        NS_LOG_LOGIC("ReconfigureBridge: Old Root port " << i << " is not the the root port any more (start RecentRootTimer)");
                        m_portInfo[i].recentRootTimer.at(tree).SetArguments(i,tree);
                        m_portInfo[i].recentRootTimer.at(tree).Schedule();
                      }
                    if(previous_role == DESIGNATED_ROLE && m_portInfo[i].role.at(tree) == DESIGNATED_ROLE)
                      {
                        NS_LOG_LOGIC("ReconfigureBridge: Designated port " << i << " keeps the role of Designated port");
                      }
                    if(previous_role == ALTERNATE_ROLE && m_portInfo[i].role.at(tree) != ALTERNATE_ROLE) 
                      {
                        NS_LOG_LOGIC("ReconfigureBridge: Alternate port elected as ROOT/DES " << i);
                        MakeDiscarding(i,tree);
                        //m_portInfo[i].toLearningTimer.at(tree).SetArguments(i,tree);  
                        //m_portInfo[i].toLearningTimer.at(tree).Schedule();
                      }
                    if(m_portInfo[i].role.at(tree)==ALTERNATE_ROLE)
                      {
                        NS_LOG_LOGIC("ReconfigureBridge: found alternate port " << i);
                        //m_portInfo[i].messAgeTimer.Remove(); // cancel messAge timer => why? it has info from the Designated in that LAN!!!
                        m_portInfo[i].recentRootTimer.at(tree).Remove(); // i guess because the port is synced, so it can not be problematic
                        MakeDiscarding(i,tree); //block this port
                        m_portInfo[i].proposed.at(tree)=false;               
                        m_portInfo[i].synced.at(tree)=true;    //synced because it is blocked
                        m_portInfo[i].newInfo.at(tree)=true;   // to send up an agreement
                        
                        if( m_spbdvActivateTopologyChange==1 )
                          {
                            // Topology Change Procedure (just flush, does not trigger any procedure)
                            m_bridge->FlushEntriesOfPort(i); // Flush entries in this port
                            m_portInfo[i].topChangeTimer.Cancel(); // Cancel port timer
                            m_portInfo[i].topChange = false;
                            m_portInfo[i].topChangeAck = false; // reset tcAck flag, just in case
                          }                
                      }
                }
            } // for all ports...
            
          // check for quick state transitions
          if(m_bridgeInfo.activeTree.at(tree)==true) QuickStateTransitions(tree);
        
          // printout the state of the bridge in a single line
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]       Brdg Info  [R:" << m_bridgeInfo.rootId.at(tree) << "|C:" << m_bridgeInfo.cost.at(tree) << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort.at(tree) << "]");

          // printout the state of each port in a single line
          for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
            {
              int16_t ageTmp;
              if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE)
                {
                  ageTmp=-1;
                }
              else
                {
                  Time portMessAge = Seconds(m_portInfo[p].messAge.at(tree));
                  Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule.at(tree); 
                  ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
                }
              
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]       Port Info  [R:" << m_portInfo[p].designatedRootId.at(tree) << "|C:" << m_portInfo[p].designatedCost.at(tree) << "|B:" << m_portInfo[p].designatedBridgeId.at(tree) << "|P:" << m_portInfo[p].designatedPortId.at(tree) << "|A:" << ageTmp << "|Tc:" << m_portInfo[p].topChange <<  "|Tca:" << m_portInfo[p].topChangeAck << "]:[" << m_portInfo[p].role.at(tree) << "-" << m_portInfo[p].state.at(tree) << "]:[" << (m_portInfo[p].proposed.at(tree)?"P'ed ":"---- ") << (m_portInfo[p].proposing.at(tree)?"P'ng ":"---- ") << (m_portInfo[p].sync.at(tree)?"Sync ":"---- ") << (m_portInfo[p].synced.at(tree)?"S'ed ":"---- ") << (m_portInfo[p].agreed.at(tree)?"A'ed ":"---- ") << (m_portInfo[p].newInfo.at(tree)?"New":"---") << "]");   
            }
        }
    }
}  
    

///////////////////////////////////////////
// SPBDV::AllDesignatedsRecentRootExpired //
///////////////////////////////////////////
bool
SPBDV::AllDesignatedsRecentRootExpired(uint64_t tree)
{
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE && m_portInfo[p].recentRootTimer.at(tree).IsRunning() && m_portInfo[p].state.at(tree)!=DISABLED_STATE)
        {
          return false;
        }
    }
  return true;
}

//////////////////////////////////////////
// SPBDV::SetSyncToAllRootAndDesignateds //
//////////////////////////////////////////
void
SPBDV::SetSyncToAllRest(uint16_t port, uint64_t tree)
{
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if((m_portInfo[p].role.at(tree)==DESIGNATED_ROLE || p!=port) && m_portInfo[p].state.at(tree)!=DISABLED_STATE)
        {
          m_portInfo[p].sync.at(tree)=true;
        }
    }
}

///////////////////////////////////////
// SPBDV::AllRootAndDesignatedsSynced //
///////////////////////////////////////
bool
SPBDV::AllRestSynced(uint16_t port, uint64_t tree)
{
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if( p!= port && !m_portInfo[p].synced.at(tree)  && m_portInfo[p].state.at(tree)!=DISABLED_STATE )
        {
         return false;
        }
    }
  return true;
}


////////////////////////////////
// SPBDV::QuickStateTransitions //
////////////////////////////////
void
SPBDV::QuickStateTransitions(uint64_t tree)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // printout the state of the bridge in a single line
  NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]       Brdg Info  [R:" << m_bridgeInfo.rootId.at(tree) << "|C:" << m_bridgeInfo.cost.at(tree) << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort.at(tree) << "]");

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      int16_t ageTmp;
      if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge.at(tree));
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule.at(tree); 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
        
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]       Port Info  [R:" << m_portInfo[p].designatedRootId.at(tree) << "|C:" << m_portInfo[p].designatedCost.at(tree) << "|B:" << m_portInfo[p].designatedBridgeId.at(tree) << "|P:" << m_portInfo[p].designatedPortId.at(tree) << "|A:" << ageTmp << "|Tc:" << m_portInfo[p].topChange <<  "|Tca:" << m_portInfo[p].topChangeAck << "]:[" << m_portInfo[p].role.at(tree) << "-" << m_portInfo[p].state.at(tree) << "]:[" << (m_portInfo[p].proposed.at(tree)?"P'ed ":"---- ") << (m_portInfo[p].proposing.at(tree)?"P'ng ":"---- ") << (m_portInfo[p].sync.at(tree)?"Sync ":"---- ") << (m_portInfo[p].synced.at(tree)?"S'ed ":"---- ") << (m_portInfo[p].agreed.at(tree)?"A'ed ":"---- ") << (m_portInfo[p].newInfo.at(tree)?"New":"---") << "]");     
    }
  

  // Quick transition of the Root port
  NS_LOG_LOGIC("QuickStateTransitions: Quick transition of the Root port " << m_bridgeInfo.rootPort.at(tree) << "?");
  if(m_bridgeInfo.rootId.at(tree) != m_bridgeInfo.bridgeId) //if we are not root bridge (so there is a root port)
    {
       //if the root port is not forwarding
      if(m_portInfo[m_bridgeInfo.rootPort.at(tree)].state.at(tree) != FORWARDING_STATE)
        {
          NS_LOG_LOGIC("QuickStateTransitions:     root port not forwarding");
          
          //if all current designated ports have expired their recent root timer
          if( AllDesignatedsRecentRootExpired(tree) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:         all current designated ports have expired their recent root timer");
              MakeForwarding(m_bridgeInfo.rootPort.at(tree), tree);
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].newInfo.at(tree)=true;
            }
          else
            {
              NS_LOG_LOGIC("QuickStateTransitions:         some recent root timer is still running (make it discarding, and make the new root port forwarding)");
              
              for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
                {
                  if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE && m_portInfo[p].recentRootTimer.at(tree).IsRunning() && m_portInfo[p].state.at(tree)!=DISABLED_STATE)
                    {
                      MakeDiscarding(p, tree);
                      //m_portInfo[p].toLearningTimer.at(tree).SetArguments(p,tree);
                      //m_portInfo[p].toLearningTimer.at(tree).Schedule();
                    }
                }

              MakeForwarding(m_bridgeInfo.rootPort.at(tree), tree);
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].newInfo.at(tree)=true;              
                            
            }
            
        }
      else
        {
          NS_LOG_LOGIC("QuickStateTransitions:     Root port already FRW");

          //if the root port is proposed, agreement sent anyway because the other is proposing
          if( m_portInfo[m_bridgeInfo.rootPort.at(tree)].proposed.at(tree) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:         the root port is proposed and forwarding. Send Agreement");
              //m_portInfo[m_bridgeInfo.rootPort.at(tree)].synced=true;
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].newInfo.at(tree)=true;
            }
        }
              
      //if the root port is proposed and not synced
      if( m_portInfo[m_bridgeInfo.rootPort.at(tree)].proposed.at(tree) && !m_portInfo[m_bridgeInfo.rootPort.at(tree)].synced.at(tree) )
        {
          NS_LOG_LOGIC("QuickStateTransitions:         the root port is proposed and not synced. Set sync to the rest");
          m_portInfo[m_bridgeInfo.rootPort.at(tree)].proposed.at(tree)=false;
          m_portInfo[m_bridgeInfo.rootPort.at(tree)].sync.at(tree)=false;
          m_portInfo[m_bridgeInfo.rootPort.at(tree)].synced.at(tree)=true;
          m_portInfo[m_bridgeInfo.rootPort.at(tree)].newInfo.at(tree)=true;
          SetSyncToAllRest(m_bridgeInfo.rootPort.at(tree), tree);
        }
    }

  
  // Quick transition of the Designated ports
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE)
        {
          NS_LOG_LOGIC("QuickStateTransitions: Quick transition of the Designated port " << p << "?");
          
          //check the conditions under the port cannot go to FRW yet
          //cannot go to FRW because it has been told to sync || cannot go to FRW because it is still recent root
          if( (/*m_portInfo[p].state!=DISCARDING_STATE &&*/ m_portInfo[p].sync.at(tree)/* && !m_portInfo[p].synced*/) || ( (m_bridgeInfo.rootId.at(tree)!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort.at(tree)].state.at(tree)!=FORWARDING_STATE) && m_portInfo[p].recentRootTimer.at(tree).IsRunning() ) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:     cannot go to FRW because it has been told to sync(" << (/*m_portInfo[p].state!=DISCARDING_STATE && */m_portInfo[p].sync.at(tree)/* && !m_portInfo[p].synced*/) << ") or it is still recent root(" <<  ( (m_bridgeInfo.rootId.at(tree)!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort.at(tree)].state.at(tree)!=FORWARDING_STATE) && m_portInfo[p].recentRootTimer.at(tree).IsRunning() ) << ")");
              MakeDiscarding(p, tree);
              //m_portInfo[p].toLearningTimer.at(tree).SetArguments(p, tree);
              //m_portInfo[p].toLearningTimer.at(tree).Schedule();
            }
          
          //update sync and synced
          // told to sync, but already synced || not synced but discarding || was not synced but got an agreement
          if ( (m_portInfo[p].synced.at(tree) && m_portInfo[p].sync.at(tree)) || (!m_portInfo[p].synced.at(tree) && m_portInfo[p].state.at(tree)==DISCARDING_STATE) || (!m_portInfo[p].synced.at(tree) && m_portInfo[p].agreed.at(tree)) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:     told to sync, but already synced(" << (m_portInfo[p].synced.at(tree) && m_portInfo[p].sync.at(tree)) << ") || not synced but discarding(" << (!m_portInfo[p].synced.at(tree) && m_portInfo[p].state.at(tree)==DISCARDING_STATE) << ") || was not synced but got an agreement(" << (!m_portInfo[p].synced.at(tree) && m_portInfo[p].agreed.at(tree)) << ")");
              m_portInfo[p].synced.at(tree)=true;
              m_portInfo[p].sync.at(tree)=false;
              m_portInfo[p].recentRootTimer.at(tree).Remove(); // i guess because the port is synced, so it can not be problematic
            }
          
          //start a handshake down the tree
          // not forwarding and not proposing and not agreed
          if(m_portInfo[p].state.at(tree)!=FORWARDING_STATE && !m_portInfo[p].proposing.at(tree) && !m_portInfo[p].agreed.at(tree))
            {
              NS_LOG_LOGIC("QuickStateTransitions:     not forwarding and not proposing and not agreed (start a handshake down the tree)");
              m_portInfo[p].proposing.at(tree)=true;
              m_portInfo[p].newInfo.at(tree)=true;
            }
          
          //check whether the Designated can be finally set to FRW
          //no one said to sync and it is agreed from downstream root-alternate
          if( m_portInfo[p].state.at(tree)!=FORWARDING_STATE && !m_portInfo[p].sync.at(tree) && m_portInfo[p].agreed.at(tree))
            {
              NS_LOG_LOGIC("QuickStateTransitions:     finally FRW? no one said to sync and it is agreed from downstream root-alternate");
              
              //not recent root (safe to FRW) || root bridge (no problem with the root port because it does not exist) || root port already forwarding (no problem with the root port because it is already forwarding)
              if( m_portInfo[p].recentRootTimer.at(tree).IsExpired() || m_bridgeInfo.rootId.at(tree)==m_bridgeInfo.bridgeId || ( m_bridgeInfo.rootId.at(tree)!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort.at(tree)].state.at(tree)==FORWARDING_STATE ) )
                {
                  NS_LOG_LOGIC("QuickStateTransitions:         not recent root(" << (m_portInfo[p].recentRootTimer.at(tree).IsExpired()) << ") or root bridge(" << (m_bridgeInfo.rootId.at(tree)==m_bridgeInfo.bridgeId) << ") or root port already forwarding(" << ( m_bridgeInfo.rootId.at(tree)!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort.at(tree)].state.at(tree)==FORWARDING_STATE ) << ")");
                  MakeForwarding(p, tree);
                  // mark newInfo????
                }
              else
                {
                  NS_LOG_LOGIC("QuickStateTransitions:         tbd. somehow move it to forwarding when the recent root timer expires");
                  //somehow move it to forwarding when the recent root timer expires ???
                }
            }
        }
      
      // recheck the root port for possible changes in designated variables
      NS_LOG_LOGIC("QuickStateTransitions: recheck the root port for possible changes in designated variables");
      if(m_bridgeInfo.rootId.at(tree) != m_bridgeInfo.bridgeId)
        {
          if( (m_portInfo[m_bridgeInfo.rootPort.at(tree)].state.at(tree) != FORWARDING_STATE) && AllDesignatedsRecentRootExpired(tree))
            {
              NS_LOG_LOGIC("QuickStateTransitions:     Root port not forwarding and AllDesignatedsRecentRootExpired");
              MakeForwarding(m_bridgeInfo.rootPort.at(tree), tree);
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].newInfo.at(tree)=true;
            }

          // recheck the root port for possible changes in designated variables
          if(m_portInfo[m_bridgeInfo.rootPort.at(tree)].proposed.at(tree) && AllRestSynced(m_bridgeInfo.rootPort.at(tree), tree) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:     root port proposed and all the rest are synced");
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].proposed.at(tree)=false;
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].sync.at(tree)=false;
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].synced.at(tree)=true;
              m_portInfo[m_bridgeInfo.rootPort.at(tree)].newInfo.at(tree)=true;
            }         
        }
    }
    
  // check the alternates as well
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if(m_portInfo[p].role.at(tree)==ALTERNATE_ROLE)
        {
          if(m_portInfo[p].proposed.at(tree))
            {  
              m_portInfo[p].sync.at(tree)=false;
              m_portInfo[p].newInfo.at(tree)=true;
            }
          if(m_portInfo[p].sync.at(tree)==true && m_portInfo[p].synced.at(tree)==true)
            {
              m_portInfo[p].sync.at(tree)=false;
            }
        }
    }

  // printout the state of the bridge in a single line
  NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]       Brdg Info  [R:" << m_bridgeInfo.rootId.at(tree) << "|C:" << m_bridgeInfo.cost.at(tree) << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort.at(tree) << "]");

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    { 
      int16_t ageTmp;
      if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge.at(tree));
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule.at(tree); 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
        
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]       Port Info  [R:" << m_portInfo[p].designatedRootId.at(tree) << "|C:" << m_portInfo[p].designatedCost.at(tree) << "|B:" << m_portInfo[p].designatedBridgeId.at(tree) << "|P:" << m_portInfo[p].designatedPortId.at(tree) << "|A:" << ageTmp << "|Tc:" << m_portInfo[p].topChange <<  "|Tca:" << m_portInfo[p].topChangeAck << "]:[" << m_portInfo[p].role.at(tree) << "-" << m_portInfo[p].state.at(tree) << "]:[" << (m_portInfo[p].proposed.at(tree)?"P'ed ":"---- ") << (m_portInfo[p].proposing.at(tree)?"P'ng ":"---- ") << (m_portInfo[p].sync.at(tree)?"Sync ":"---- ") << (m_portInfo[p].synced.at(tree)?"S'ed ":"---- ") << (m_portInfo[p].agreed.at(tree)?"A'ed ":"---- ") << (m_portInfo[p].newInfo.at(tree)?"New":"---") << "]");     
    }

}



///////////////////
// SPBDV::SendBPDU //
///////////////////
void
SPBDV::SendBPDU (Ptr<NetDevice> outPort, std::string debugComment, uint64_t tree)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t p = outPort->GetIfIndex();
  uint64_t numBpdusMerged=0;
  uint64_t numBytes=54; //34 ethernet fixed + 20 common to all trees + 16 per tree + 8 per path vector element per tree
  
  std::vector <uint16_t> messAgeTmp;
  std::vector <bool>     messAgeLowerMaxAge;
  
  if (Simulator::Now() < Time(Seconds(m_simulationtime)) )
    {    
      NS_LOG_LOGIC("SendBPDU(): Port " << p); 
      //check how many bpdus have to be merged
      for( uint64_t t=0 ; t<m_numNodes ; t++ )
        {
          if(m_spbdvActivateBpduMerging==1 || t==tree) // all trees if m_spbdvActivateBpduMerging is activated, or when it is not activated and the t=tree
            {
              NS_LOG_LOGIC("SendBPDU(): Checking MessAge for tree " << t << ": newinfo " << m_portInfo[p].newInfo.at(t) << " - disabled " << (m_portInfo[p].state.at(t)!=DISABLED_STATE) );
              if( m_portInfo[p].newInfo.at(t)==true && m_portInfo[p].state.at(t)!=DISABLED_STATE)
                {
                  //compute MessAge to check if the bpdu of 't' is merged
                  messAgeTmp.resize(m_numNodes);
                  messAgeLowerMaxAge.resize(m_numNodes);
                  if(m_bridgeInfo.rootId.at(t) == m_bridgeInfo.bridgeId || m_bridgeInfo.rootPort.at(t)==-1 )
                    {
                      NS_LOG_LOGIC("SendBPDU(): Checking MessAge of tree " << t << "(0, i am the root)");
                      messAgeTmp.at(t)=0; // it is the root
                    }
                  else
                    {
                      // SPBDV sets the MessAge value exactly to the last value received in the root port
                      if(m_spbdvActivateIncreaseMessAge==1)
                        {
                          messAgeTmp.at(t) = (uint16_t) ( m_portInfo[m_bridgeInfo.rootPort.at(t)].messAge.at(t) + m_bridgeInfo.messAgeInc ); // it is not the root
                        }
                      else
                        {
                          messAgeTmp.at(t)=0;
                        }
                    }
                  if(messAgeTmp.at(t) < m_bridgeInfo.maxAge)
                    {
                      NS_LOG_LOGIC("SendBPDU(): Checking MessAge of tree " << t << "(" << messAgeTmp.at(t) << ", BPDU elested to merge)");
                      messAgeLowerMaxAge.at(t) = true;
                      numBpdusMerged++;
                    }
                  else
                    {
                      messAgeLowerMaxAge.at(t) = false;
                      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] SPBDV: Discarded BPDU of tree " << t << " due to MessAge >= MaxAge");
                    }       
                }               
            }
        }// end of for loop


      if(numBpdusMerged>0) // if there are bpdus to merge
        {       
          //if no BPDUs send during last period, we can safely sent
          if(m_portInfo[p].txHoldCount < m_bridgeInfo.maxBpduTxHoldPeriod) 
            {
              NS_LOG_LOGIC("SendBPDU(): TxHoldCount(" << m_portInfo[p].txHoldCount << ") allows us to send");
              
              uint16_t protocol = 9999; //should be the LLC protocol
              Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
              Mac48Address dst = Mac48Address("01:80:C2:00:00:00");

              // create the BPDU
              Ptr<Packet> bpdu = Create<Packet> (0);
              
              // Create BPDU header
              BpduMergedHeader bpduFields = BpduMergedHeader ();
              bpduFields.SetType(BPDU_PV_MERGED_TYPE);                
              
              // add fields to bpdu header
              bpduFields.SetNumBpdusMerged(numBpdusMerged);
              uint64_t bpduInd=0;
              for(uint64_t t=0 ; t < m_numNodes ; t++ )
                {
                  if(m_spbdvActivateBpduMerging==1 || t==tree) // all trees if m_spbdvActivateBpduMerging is activated, or when it is not activated and the t=tree
                    {
                      NS_LOG_LOGIC("SendBPDU(): Adding fields of merged BPDU for tree " << t << " (newInfo: " << m_portInfo[p].newInfo.at(t) << " - disabled: " << (m_portInfo[p].state.at(t)!=DISABLED_STATE) << " - lowerMessAge: " << (messAgeLowerMaxAge.at(t) == true) << ")");
                      // if the bpdu of the tree 't' is to be merged            
                      if( m_portInfo[p].newInfo.at(t)==true && m_portInfo[p].state.at(t)!=DISABLED_STATE && messAgeLowerMaxAge.at(t) == true)
                        {      
                          numBytes = numBytes + 16;
                                        
                          bpduFields.SetRootId(m_portInfo[p].designatedRootId.at(t), bpduInd);
                          bpduFields.SetCost(m_bridgeInfo.cost.at(t), bpduInd);//m_portInfo[p].designatedCost);
                          bpduFields.SetBridgeId(m_bridgeInfo.bridgeId, bpduInd);//m_portInfo[p].designatedBridgeId);
                          bpduFields.SetPortId(m_portInfo[p].designatedPortId.at(t), bpduInd);
                          NS_LOG_LOGIC("SendBPDU(): Priority Vector added");
                          
                          //set flags
                          if(m_portInfo[p].proposing.at(t)) bpduFields.SetPropFlag(true, bpduInd); else bpduFields.SetPropFlag(false, bpduInd);
                          bpduFields.SetRoleFlags(m_portInfo[p].role.at(t), bpduInd);
                          if(m_portInfo[p].state.at(t)==DISCARDING_STATE) bpduFields.SetLrnFlag(false, bpduInd); else bpduFields.SetLrnFlag(true, bpduInd);
                          if(m_portInfo[p].state.at(t)==FORWARDING_STATE) bpduFields.SetFrwFlag(true, bpduInd); else bpduFields.SetFrwFlag(false, bpduInd);          
                          if(m_portInfo[p].synced.at(t) && (m_portInfo[p].role.at(t)==ROOT_ROLE || m_portInfo[p].role.at(t)==ALTERNATE_ROLE) ) bpduFields.SetAgrFlag(true, bpduInd); else bpduFields.SetAgrFlag(false, bpduInd);
                          bpduFields.SetTcFlag(m_portInfo[p].topChange, bpduInd);          
                          bpduFields.SetTcaFlag(m_portInfo[p].topChangeAck, bpduInd); 
                          NS_LOG_LOGIC("SendBPDU(): Flags added");
                          
                          // set the path vector
                          if(m_portInfo[p].role.at(t)==DESIGNATED_ROLE) // path vectors only added in BPDUs following Des ports
                            {
                              if(m_bridgeInfo.rootId.at(t) == m_bridgeInfo.bridgeId) //if the local node id the root of the tree
                                {
                                  //add the local bridgeId as the path vector
                                  bpduFields.SetPathVectorNumBridgeIds(1, bpduInd);
                                  bpduFields.AddPathVectorBridgeId(m_bridgeInfo.bridgeId, bpduInd);
                                  numBytes = numBytes + 8;
                                }
                              else
                                {
                                  //add the path vector in the bridge
                                  bpduFields.SetPathVectorNumBridgeIds( m_bridgeInfo.pathVector.at(t).size() , bpduInd);
                                  for( uint16_t i=0 ; i < m_bridgeInfo.pathVector.at(t).size() ; i++)
                                    {
                                      bpduFields.AddPathVectorBridgeId( m_bridgeInfo.pathVector.at(t).at(i) , bpduInd); 
                                      numBytes = numBytes + 8;
                                    }
                                }
                              numBytes = numBytes - 16; // the first elements is always the Root Id and the last element of the PV is always the bridgeId
                            }
                          NS_LOG_LOGIC("SendBPDU(): Path Vector added");
                          
                          //set MessAge
                          bpduFields.SetMessAge(messAgeTmp.at(t), bpduInd);
                          NS_LOG_LOGIC("SendBPDU(): Message Age added");
                                            
                          bpduInd++;
                          
                          //if the BPDU is merged, unset newInfo for that tree
                          m_portInfo[p].newInfo.at(t)=false;
                        
                        }//if bpdu merged because messAge lower then maxage
                        
                    }//if merging activated or tree=t
                
                }// end of for loop
              
              //additional check
              NS_LOG_LOGIC("SendBPDU() - additional check between bpduInd(" << bpduInd << ") and numBpdusMerged(" << numBpdusMerged << ")");
              if(bpduInd != numBpdusMerged )
                {
                  NS_LOG_ERROR("ERROR: SendBPDU() - bpduInd(" << bpduInd << ") and numBpdusMerged(" << numBpdusMerged << ") are different");
                  exit(0);
                }
                 
              //additional check
              if(m_spbdvActivateBpduMerging==0 && bpduInd>1 )
                {
                  NS_LOG_ERROR("ERROR: SendBPDU() - BPDU merging is not activated and more than one (" << bpduInd << ") BPDUs are merged");
                  exit(0);
                }
                
              //add bpdu header to packet
              bpdu->AddHeader(bpduFields);
              NS_LOG_LOGIC("SendBPDU() - Header added to packet");    
                  
              //update txHold counter
              m_portInfo[p].txHoldCount ++;
              NS_LOG_LOGIC("SendBPDU() - txHoldCount incremented");    

              // debug message
              
              std::stringstream stringOut;
              stringOut << Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] SPBDV: Sends BPDU with " << numBpdusMerged << " merged BPDUs (link: " << outPort->GetChannel()->GetId() << ") (bytes: " << numBytes << " )";
              NS_LOG_INFO(stringOut.str());
              PrintBpdu(bpduFields);
              
              // send to the out port
              outPort->SendFrom (bpdu, src, dst, protocol);
              
              // reset because it is set just before calling SendBPDU
              m_portInfo[p].topChangeAck=false;    

            }// if txHoldCount
          else
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] SPBDV: Holds BPDU (TxCount: " << m_portInfo[p].txHoldCount << " => " <<  m_portInfo[p].txHoldCount+1 << ")");
              m_portInfo[p].txHoldCount ++; // even if we have not sent, keep counting to differentiate 1 sent from 1 sent and 1, or more, hold

              // mark atLeastOneHold for the trees that were to be merged in the BPDU
              for(uint64_t t=0 ; t < m_numNodes ; t++)
                {
                  if( m_portInfo[p].newInfo.at(t)==true && m_portInfo[p].state.at(t)!=DISABLED_STATE && messAgeLowerMaxAge.at(t) == true )
                    {
                      m_portInfo[p].atLeastOneHold.at(t)=true;
                    }
                }
            } 
          
        } //if numBpdusMerged>0
        
    }//if simulation time   
}

////////////////////////////
// SPBDV::SendHelloTimeBpdu //
////////////////////////////
void
SPBDV::SendHelloTimeBpdu()
{
  // RSTP style: everyone sends periodical BPDUs 
  if(m_spbdvActivatePeriodicalStpStyle==false)
    {
      if( Simulator::Now() < Time(Seconds(m_simulationtime))) 
        {
        
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] SPBDV: Periodical BPDUs");
          
          //Mac48Address HwDestination = Mac48Address("00:11:22:33:44:55"); //this must be the 'to all bridges' mac address
         
          for(uint16_t i=0 ; i < m_ports.size() ; i++)
            {
              for(uint64_t t=0 ; t < m_numNodes ; t++)
                {
                  if(m_portInfo[i].role.at(t)==DESIGNATED_ROLE && m_bridgeInfo.activeTree.at(t)==true) //send a BPDU to each tree (except first set of messages)
                    {
                      m_portInfo[i].newInfo.at(t)=true;
                      if(m_spbdvActivateBpduMerging == 0) SendBPDU(m_ports[i], "periodical_BPDUs", t);
                    } 
                }
              // call SendBPDU after all newInfo's have been marked, in order to merge BPDUs 
              if(m_spbdvActivateBpduMerging == 1) SendBPDU(m_ports[i], "periodical_BPDUs", 0);
            }
          
          //schedule next periodical BPDUs (only if m_activatePeriodicalBpdus)
          if( m_activatePeriodicalBpdus==true )
            {
              if(m_bridgeInfo.helloTimeTimer.IsExpired())
                {
                  m_bridgeInfo.helloTimeTimer.Schedule();
                }
              else
                {
                  NS_LOG_ERROR("SPBDV: Trying to schedule a helloTimeTimer while there is another scheduled");
                  exit(1);
                } 
            }
        }
    }  

  // STP style: only the root of the tree sends periodical BPDUs (in this case the equal BPDUs must be disseminated)
  if(m_spbdvActivatePeriodicalStpStyle==true)
    {
      NS_LOG_ERROR("SPBDV: spbdvActivatePeriodicalStpStyle not ready for merged BPDUs");
      exit(1);

      if(Simulator::Now() < Time(Seconds(m_simulationtime)))
        {
                            
          uint64_t tree = m_bridgeInfo.bridgeId;
          
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] SPBDV: Periodical BPDUs");
          
          //Mac48Address HwDestination = Mac48Address("00:11:22:33:44:55"); //this must be the 'to all bridges' mac address
         
          for(uint16_t i=0 ; i < m_ports.size() ; i++)
            {
              //for(uint64_t t=0 ; t < m_numNodes ; t++)
                //{
                  if(m_portInfo[i].role.at(tree)==DESIGNATED_ROLE && m_bridgeInfo.activeTree.at(tree)==true) //send a BPDU to each tree (except first set of messages)
                    {
                      m_portInfo[i].newInfo.at(tree)=true;
                      std::stringstream debugComment;
                      debugComment << "periodical_BPDU_tree_" << tree;
                      SendBPDU(m_ports[i], debugComment.str(), 0);
                    } 
                //}
            }
          
          //schedule next periodical BPDUs (only if m_activatePeriodicalBpdus)
          if( m_activatePeriodicalBpdus==true )
            {
              if(m_bridgeInfo.helloTimeTimer.IsExpired())
                {
                  m_bridgeInfo.helloTimeTimer.Schedule();
                }
              else
                {
                  NS_LOG_ERROR("SPBDV: Trying to schedule a helloTimeTimer while there is another scheduled");
                  exit(1);
                }        
            }
        }
    }
}

///////////////////////////
// SPBDV::ResetTxHoldCount //
///////////////////////////
void SPBDV::ResetTxHoldCount()
{
  uint16_t amountToDecrement=1;
  
  if (Simulator::Now() < Time(Seconds(m_simulationtime)))
    {  
      uint16_t lastTxHoldCount;
           
      for(uint16_t p=0 ; p < m_ports.size() ; p++)
        { 
          if(m_portInfo[p].state.at(0)!=DISABLED_STATE)
            {
              lastTxHoldCount = m_portInfo[p].txHoldCount;
              if(m_bridgeInfo.resetTxHoldPeriod==1)
                {
                  m_portInfo[p].txHoldCount = 0; // if the input parameter says to reset, set ot 0 ...
                }
              else
                {
                   if(m_portInfo[p].txHoldCount == 0)
                     {
                     }
                   else
                     {
                       if(m_portInfo[p].txHoldCount >= m_bridgeInfo.maxBpduTxHoldPeriod)
                         {
                           m_portInfo[p].txHoldCount = m_bridgeInfo.maxBpduTxHoldPeriod - amountToDecrement ; // ... otherwise decrement one (case where much more than permitted have benn sent)
                         }
                        else
                         {
                           m_portInfo[p].txHoldCount = m_portInfo[p].txHoldCount - amountToDecrement ; // ... otherwise decrement one (case where much less than permitted have benn sent)
                         }
                     }

                }
              NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "] SPBDV: Clr TxHold (TxCount: " << lastTxHoldCount << "=>0)");
              
              if( lastTxHoldCount > m_bridgeInfo.maxBpduTxHoldPeriod ) //more than the maximum number of BPDUs sent, so at least one is pending
                {
                  for(uint64_t t=0 ; t < m_numNodes ; t++)
                    {
                      if(m_portInfo[p].atLeastOneHold.at(t)==true )// && m_bridgeInfo.activeTree.at(t)==true)
                        {
                          m_portInfo[p].newInfo.at(t)=true;
                          m_portInfo[p].atLeastOneHold.at(t)=false;
                          if(m_spbdvActivateBpduMerging == 0) SendBPDU(m_ports[p], "pending_txHold", t);
                        }
                    }
                  
                  // call SendBPDU after all newInfo's have been marked, in order to merge BPDUs 
                  if(m_spbdvActivateBpduMerging == 1) SendBPDU(m_ports[p], "pending_txHold", 0);
                }
            }
        }

      //reschedule call to this function at next period
      if(m_bridgeInfo.txHoldCountTimer.IsExpired())
        {
          m_bridgeInfo.txHoldCountTimer.Schedule();
        }
      else
        {
          NS_LOG_ERROR("SPBDV: Trying to schedule a txHoldCount while there is another scheduled");
          exit(1);
        }
    }
}

//////////////////////////
// SPBDV::MakeDiscarding //
//////////////////////////
void SPBDV::MakeDiscarding (uint16_t port, uint64_t tree)
{
  if(m_portInfo[port].state.at(tree)!=DISABLED_STATE)
    {
      m_portInfo[port].state.at(tree)=DISCARDING_STATE;
      m_portInfo[port].toLearningTimer.at(tree).Remove();
      m_portInfo[port].toForwardingTimer.at(tree).Remove();
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBDV: DSC State (tree " << tree << ")");
      
      //notify simGod (if simulation already running, avoid computations during Init)
      if( Simulator::Now() != Seconds(0) ) m_simGod->SetActivePort(tree, m_node->GetId(), port, false); 
      
      //notify bridge modele of the blocked port
      m_bridge->SetPortState(tree, port, false);
      
      //PrintBridgeInfoSingleLine(tree);
    }
}

///////////////////////
// SPBDV::MakeLearning //
///////////////////////
void SPBDV::MakeLearning (uint16_t port, uint64_t tree)
{
  if(m_portInfo[port].state.at(tree)!=DISABLED_STATE)
    {
      m_portInfo[port].state.at(tree)=LEARNING_STATE;    
      m_portInfo[port].toLearningTimer.at(tree).Remove();
      //m_portInfo[port].toForwardingTimer.at(tree).Remove();
      //m_portInfo[port].toForwardingTimer.at(tree).SetArguments(port, tree);  
      //m_portInfo[port].toForwardingTimer.at(tree).Schedule();
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBDV: LRN State (tree " << tree << ")");
    }
}

/////////////////////////
// SPBDV::MakeForwarding //
/////////////////////////
void SPBDV::MakeForwarding (uint16_t port, uint64_t tree)
{
  if(m_portInfo[port].state.at(tree)!=DISABLED_STATE)
    {
      m_portInfo[port].state.at(tree)=FORWARDING_STATE;
      m_portInfo[port].toLearningTimer.at(tree).Remove();
      m_portInfo[port].toForwardingTimer.at(tree).Remove();
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBDV: FRW State (tree " << tree << " )");

      //notify simGod 
      m_simGod->SetActivePort(tree, m_node->GetId(), port, true);

      //notify bridge model of the blocked port
      m_bridge->SetPortState(tree, port, true);

      // since there is a topology change, mark all ports to send BPDUs
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          m_portInfo[i].newInfo.at(tree)=true;
          //SendBPDU(m_ports[i], "disseminate BPDU");
        }

      // Topology Change Procedure
      if( m_spbdvActivateTopologyChange==1 )
        {
          if(m_portInfo[port].topChangeTimer.IsRunning() == false) // start timer of this port if not running
            {
              m_portInfo[port].topChangeTimer.SetArguments(port);
              m_portInfo[port].topChangeTimer.Schedule();
            }
          
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "]      Port enters Topology Change Procedure");  
          m_portInfo[port].topChange = true; // set the topChange flag of the port
          
          // for all active ports except the one going forwarding
          for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
            {
              if(i != port)
                {
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << i << "]      Port enters Topology Change Procedure");
                  
                  // set topChange of the port
                  m_portInfo[i].topChange = true;
                  
                  // flush entries of the port
                  m_bridge->FlushEntriesOfPort(i);
                  
                  // start timer of the port if not running
                  if (m_portInfo[i].topChangeTimer.IsRunning() == false)
                    {
                      m_portInfo[i].topChangeTimer.SetArguments(i);
                      m_portInfo[i].topChangeTimer.Schedule();
                    }
                }
            }
        }
        
      // set the timer to consolidate roles after Xsec
      if(m_spbdvActivateTreeDeactivationV2==1)
        {
          m_portInfo[port].setConsolidatedRolesTimer.at(tree).Remove();
          m_portInfo[port].setConsolidatedRolesTimer.at(tree).SetArguments(tree);      
          m_portInfo[port].setConsolidatedRolesTimer.at(tree).Schedule();
        }
      
      PrintBridgeInfoSingleLine(tree);
    }
}

////////////////////////////
// SPBDV::MessageAgeExpired //
////////////////////////////
void SPBDV::MessageAgeExpired(uint16_t port, uint64_t tree)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)) && m_spbdvActivateExpirationMaxAge==1 && m_bridgeInfo.activeTree.at(tree)==true)
    {
      //cancel timer
      m_portInfo[port].messAgeTimer.at(tree).Remove();
      
      //clear the port info of the expired port with the 'own' info (assuming it will be the root)
        //m_portInfo[port].role  = DESIGNATED_ROLE;
        //m_portInfo[port].state = DISABLED_STATE;
      m_portInfo[port].designatedRootId.at(tree) = m_bridgeInfo.rootId.at(tree);
      m_portInfo[port].designatedCost.at(tree) = m_bridgeInfo.cost.at(tree);
      m_portInfo[port].designatedBridgeId.at(tree) = m_bridgeInfo.bridgeId;
      m_portInfo[port].designatedPortId.at(tree) = m_ports[port]->GetIfIndex();
      //m_portInfo[port].messAge = 0;       ??????????

      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBDV: MessAgeExp [R:" << m_portInfo[port].designatedRootId.at(tree) << "|C:" << m_portInfo[port].designatedCost.at(tree) << "|B:" << m_portInfo[port].designatedBridgeId.at(tree) << "|P:" << m_portInfo[port].designatedPortId.at(tree) << "|A:" << m_portInfo[port].messAge.at(tree) << "]");      

      //reconfigure the bridge to select new port roles
      ReconfigureBridge(tree);
    
      //send BPDU to designated ports
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          if(m_portInfo[i].role.at(tree) == DESIGNATED_ROLE && m_bridgeInfo.activeTree.at(tree)==true)
            {
              m_portInfo[i].newInfo.at(tree)=true;
              SendBPDU(m_ports[i], "mess_age_expiration", tree);
            }
        }

    } 
}

//////////////////////////////////
// SPBDV::RecentRootTimerExpired //
//////////////////////////////////
void SPBDV::RecentRootTimerExpired(uint16_t port, uint64_t)
{
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] WARNING: RecentRootTimer Expires => Something to do?");
}

//////////////////////////////////
// SPBDV::PhyLinkFailureDetected //
//////////////////////////////////
void SPBDV::PhyLinkFailureDetected(uint16_t port)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)))
    {
      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBDV: Physical Link Failure Detected");  
      
      //cancel timers
      m_portInfo[port].phyLinkFailDetectTimer.Remove();
      for(uint64_t t=0 ; t < m_numNodes ; t++)
        {
          m_portInfo[port].messAgeTimer.at(t).Remove();
          
          // clear the path vector in that port, int that tree
          m_portInfo[port].pathVector.at(t).clear();
            
          //clear the port info of the expired port with the 'own' info
          m_portInfo[port].designatedRootId.at(t) = t;
          if(m_portInfo[port].role.at(t)==ROOT_ROLE)
            {
              m_portInfo[port].designatedCost.at(t) = 9999;
              m_bridgeInfo.cost.at(t) = 9999;
            }
          else
            {
              m_portInfo[port].designatedCost.at(t) = m_bridgeInfo.cost.at(t);
            }
          //m_portInfo[port].designatedBridgeId.at(t) = m_bridgeInfo.bridgeId;
          //m_portInfo[port].designatedPortId.at(t) = m_ports[port]->GetIfIndex();
          //m_portInfo[port].messAge = 0;       ??????????   

          m_portInfo[port].state.at(t)=DISABLED_STATE; 
          //m_portInfo[port].role.at(t)=DESIGNATED_ROLE;
          m_portInfo[port].agreed.at(t)=false;
          m_simGod->SetActivePort(t, m_node->GetId(), port, false);
         
          //mark the port for tree deactivation
          if(m_portInfo[port].role.at(t) == ROOT_ROLE || m_portInfo[port].role.at(t) == ALTERNATE_ROLE) m_portInfo[port].receivedTreeDeactivation.at(t)=true; 
         
          //reconfigure the bridge to select new port roles
          ReconfigureBridge(t);
                           
          //send BPDU to designated ports
          for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
            {
              if(m_portInfo[i].role.at(t) == DESIGNATED_ROLE)
                {
                  m_portInfo[i].newInfo.at(t)=true;
                  SendBPDU(m_ports[i], "phy_link_failure_detected", t);
                }
            }
        }
        
      // clear txHold counters to start counting the failure messages (force synchronization)
      m_bridgeInfo.txHoldCountTimer.Remove();
      ResetTxHoldCount();
      
    } 
}


/////////////////////////////////
// SPBDV::SetConsolidatedRoles //
/////////////////////////////////
void SPBDV::SetConsolidatedRoles(uint64_t tree)
{
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] Setting consolidated port roles");
  
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      m_portInfo[p].consolidatedRole.at(tree)=m_portInfo[p].role.at(tree);
      m_portInfo[p].receivedTreeDeactivation.at(tree)=false;
    }
}


////////////////////////////////
// SPBDV::TopChangeTimerExpired //
////////////////////////////////
void SPBDV::TopChangeTimerExpired(uint16_t port)
{
  if(m_spbdvActivateTopologyChange==1)
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "]      Port leaves Topology Change Procedure");
      
      // set the topChange of the port to false, so it stops sending BPDUs with TC flag
      m_portInfo[port].topChange = false;
    }
  else
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: TopChangeTimerExpired when toplogy change is disabled");
      exit(1);
    }
}

//////////////////////////////
// SPBDV::ScheduleLinkFailure //
//////////////////////////////
void
SPBDV::ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port)
{  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] SPBDV: Setting Link Failure Physical Detection in port " << port << " at time " << linkFailureTimeUs);

  //configure messAge timer
  m_portInfo[port].phyLinkFailDetectTimer.Remove();
  m_portInfo[port].phyLinkFailDetectTimer.SetArguments(port);  
  m_portInfo[port].phyLinkFailDetectTimer.Schedule(linkFailureTimeUs); 
}


//////////////////////
// SPBDV::PrintBpdu //
//////////////////////
void
SPBDV::PrintBpdu(BpduMergedHeader bpdu)
{
  for(uint64_t bpduInd=0 ; bpduInd<bpdu.GetNumBpdusMerged() ; bpduInd++)
    {
      std::stringstream stringOut;
      stringOut << "                                     [BPDU] => [R:" << bpdu.GetRootId(bpduInd) << "|" << "C:" << bpdu.GetCost(bpduInd) << "|" << "B:" << bpdu.GetBridgeId(bpduInd) << "|" << "P:" << bpdu.GetPortId(bpduInd) << "] ";
      stringOut << "PV(" << bpdu.GetPathVectorNumBridgeIds(bpduInd) << ")[";
      for(uint64_t elemPV=0 ; elemPV<bpdu.GetPathVectorNumBridgeIds(bpduInd) ; elemPV++)
        {
          stringOut << bpdu.GetPathVectorBridgeId(elemPV, bpduInd);
          if((int64_t)elemPV<bpdu.GetPathVectorNumBridgeIds(bpduInd)-1) stringOut << ">";
        }
      stringOut << "]";
      NS_LOG_INFO(stringOut.str());
    }	
}


////////////////////////////////////
// SPBDV::PrintBridgeInfoSingleLine //
////////////////////////////////////
void
SPBDV::PrintBridgeInfoSingleLine(uint64_t tree)
{

  std::stringstream stringOut;

  // printout the state of the bridge in a single line
  stringOut << "====================\n"; 
  stringOut << "B" << m_bridgeInfo.bridgeId << "-T" << tree << " [R:" << m_bridgeInfo.rootId.at(tree) << "|C:" << m_bridgeInfo.cost.at(tree) << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort.at(tree) << "]\n";

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      int16_t ageTmp;
      if(m_portInfo[p].role.at(tree)==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge.at(tree));
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule.at(tree); 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
      stringOut << "    P" << p << "[R:" << m_portInfo[p].designatedRootId.at(tree) << "|C:" << m_portInfo[p].designatedCost.at(tree) << "|B:" << m_portInfo[p].designatedBridgeId.at(tree) << "|P:" << m_portInfo[p].designatedPortId.at(tree) << "|A:" << ageTmp << "]:[" << m_portInfo[p].role.at(tree) << "-" << m_portInfo[p].state.at(tree) << "]:[" << (m_portInfo[p].proposed.at(tree)?"P'ed ":"---- ") << (m_portInfo[p].proposing.at(tree)?"P'ng ":"---- ") << (m_portInfo[p].sync.at(tree)?"Sync ":"---- ") << (m_portInfo[p].synced.at(tree)?"S'ed ":"---- ") << (m_portInfo[p].agreed.at(tree)?"A'ed ":"---- ") << (m_portInfo[p].newInfo.at(tree)?"New":"---") << "]\n";     
    }   
  stringOut << "===================="; 
  NS_LOG_INFO(stringOut.str());

}


uint64_t
SPBDV::GetBridgeId()
{
  return m_bridgeInfo.bridgeId;
}

void
SPBDV::DoDispose (void)
{
  // bridge timers
  m_bridgeInfo.helloTimeTimer.Cancel();
  m_bridgeInfo.txHoldCountTimer.Cancel();
    
  // port timers
  for(uint16_t p=0 ; p < m_ports.size() ; p++)
    { 
      m_portInfo[p].phyLinkFailDetectTimer.Cancel();
      m_portInfo[p].topChangeTimer.Cancel();
      for(uint16_t t=0 ; t < m_numNodes ; t++)
        {
          m_portInfo[p].toLearningTimer.at(t).Cancel();
          m_portInfo[p].toForwardingTimer.at(t).Cancel();
          m_portInfo[p].messAgeTimer.at(t).Cancel();
          m_portInfo[p].recentRootTimer.at(t).Cancel();
          m_portInfo[p].setConsolidatedRolesTimer.at(t).Cancel();
        }
    }
}   
} //namespace ns3
