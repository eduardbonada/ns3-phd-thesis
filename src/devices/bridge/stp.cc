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

#include "stp.h"
#include "bridge-net-device.h"
#include "bpdu-header.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include <stdlib.h>
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("STP");

namespace ns3 {

TypeId
STP::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::STP")
    .SetParent<Object> ()
    .AddConstructor<STP> ()
    .AddTraceSource ("StpRcvBPDU", 
                     "Trace source indicating a BPDU has been received by the STP module (still not used)",
                     MakeTraceSourceAccessor (&STP::m_stpRcvBpdu))
    ;
  return tid;
}

STP::STP()
{
  NS_LOG_FUNCTION_NOARGS ();
}
  
STP::~STP()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//////////////////
// STP::Install //
//////////////////
void
STP::Install (
    Ptr<Node> node,
    Ptr<BridgeNetDevice> bridge,
    NetDeviceContainer ports,
    uint64_t bridgeId,
    double initTime,
    uint16_t stpSimulationTimeSecs,
    uint16_t stpActivatePeriodicalBpdus,
    uint16_t stpActivateExpirationMaxAge,
    uint16_t stpActivateIncreaseMessAge,
    uint16_t stpActivateTopologyChange,
    uint16_t stpMaxAge,
    uint16_t stpHelloTime,
    uint16_t stpForwDelay,
    double   stpTxHoldPeriod,
    uint32_t stpMaxBpduTxHoldPeriod,
    Ptr<SimulationGod> god
    )
{
  m_node = node;
  m_bridge = bridge;
  
  m_bridgeInfo.messAgeInc = 1;
  
  m_simGod = god; 
  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] STP: Installing STP with bridge Id " << bridgeId << " starting at " << initTime);
  
  // link to each of the ports of the bridge to be able to directly send BPDUs to them
  for (NetDeviceContainer::Iterator i = ports.Begin (); i != ports.End (); ++i)
    {
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] STP: Link to port " << (*i)->GetIfIndex() << " of node " << (*i)->GetNode()->GetId());
      m_ports.push_back (*i);
      
      //Register the m_RxStpCallback for each net device in the protocol handler
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] STP: RegisterProtocolHandler for port " << (*i)->GetIfIndex());
      m_node->RegisterProtocolHandler (MakeCallback (&STP::ReceiveFromDevice, this), 9999, *i, true); // protocol = 9999 because it is the BPDUs protocol field (data is 8888)
  
      portInfo_t pI;
      m_portInfo.push_back(pI);
    }
  
  //general node variables
  m_simulationtime              = stpSimulationTimeSecs;
  m_activatePeriodicalBpdus     = stpActivatePeriodicalBpdus;
  m_stpActivateExpirationMaxAge = stpActivateExpirationMaxAge;
  m_stpActivateIncreaseMessAge  = stpActivateIncreaseMessAge;
  m_stpActivateTopologyChange   = stpActivateTopologyChange;
  m_initTime                    = initTime;

  //init bridge info to root
  m_bridgeInfo.rootId       = bridgeId;
  m_bridgeInfo.cost         = 0;
  m_bridgeInfo.bridgeId     = bridgeId;
  m_bridgeInfo.rootPort     = -1; //the root bridge has not root port
  m_bridgeInfo.maxAge       = stpMaxAge;
  m_bridgeInfo.helloTime    = stpHelloTime;
  m_bridgeInfo.forwDelay    = stpForwDelay;
  m_bridgeInfo.txHoldPeriod = stpTxHoldPeriod; //period accounting BPDUs
  m_bridgeInfo.maxBpduTxHoldPeriod = stpMaxBpduTxHoldPeriod; //number of bpdus per m_txHoldPeriod period
  m_bridgeInfo.topChangeDetected   = false;
  m_bridgeInfo.topChange           = false;
  
  //configure and schedule txHoldCountTimer
  m_bridgeInfo.txHoldCountTimer.SetFunction(&STP::ResetTxHoldCount,this);
  m_bridgeInfo.txHoldCountTimer.SetDelay(Seconds(m_bridgeInfo.txHoldPeriod));
  m_bridgeInfo.txHoldCountTimer.Schedule(Seconds(initTime));
  
  //configure and schedule HelloTimeTimer
  m_bridgeInfo.helloTimeTimer.SetFunction(&STP::SendHelloTimeBpdu,this);
  m_bridgeInfo.helloTimeTimer.SetDelay(Seconds(m_bridgeInfo.helloTime));
  m_bridgeInfo.helloTimeTimer.Schedule(Seconds(initTime));
  
  //configure topChangeNotifTimer
  m_bridgeInfo.topChangeNotifTimer.SetFunction(&STP::TopChangeNotifTimerExpired,this);
  m_bridgeInfo.topChangeNotifTimer.SetDelay(Seconds(m_bridgeInfo.helloTime));

  //configure topChangeTimer
  m_bridgeInfo.topChangeTimer.SetFunction(&STP::TopChangeTimerExpired,this);
  m_bridgeInfo.topChangeTimer.SetDelay(Seconds(m_bridgeInfo.maxAge + m_bridgeInfo.forwDelay));

  //init all port info to designated
  for (uint16_t i=0 ; i < m_portInfo.size () ; i++)
    {
      m_portInfo[i].state = LISTENING_STATE;
      m_portInfo[i].role  = DESIGNATED_ROLE;
      m_portInfo[i].designatedRootId = bridgeId;
      m_portInfo[i].designatedCost = 0;
      m_portInfo[i].designatedBridgeId = bridgeId;
      m_portInfo[i].designatedPortId = m_ports[i]->GetIfIndex();
      m_portInfo[i].messAge = 0;
      m_portInfo[i].receivedBpdu.rootId = bridgeId;;
      m_portInfo[i].receivedBpdu.cost = 0; 
      m_portInfo[i].receivedBpdu.bridgeId = bridgeId;
      m_portInfo[i].txHoldCount = 0;
      m_portInfo[i].topChangeAck = false;
      
      //configure learningTimer and listeningTimer
      m_portInfo[i].listeningTimer.SetFunction(&STP::MakeLearning,this);
      m_portInfo[i].listeningTimer.SetDelay(Seconds(m_bridgeInfo.forwDelay));
      m_portInfo[i].learningTimer.SetFunction(&STP::MakeForwarding,this);
      m_portInfo[i].learningTimer.SetDelay(Seconds(m_bridgeInfo.forwDelay));
      MakeListening(i);

      //configure messAge timer
      //if(m_stpActivateExpirationMaxAge == 1)
        //{
          m_portInfo[i].messAgeTimer.SetFunction(&STP::MessageAgeExpired,this);
          //m_portInfo[i].messAgeTimer.SetDelay(Seconds(m_bridgeInfo.maxAge)); the value depends on the received MessAge
        //}
      
      //configure physical link failure detection. it will be scheduled if function called from main simulation config file
      m_portInfo[i].phyLinkFailDetectTimer.SetFunction(&STP::PhyLinkFailureDetected,this);
    }
}


////////////////////////////
// STP::ReceiveFromBridge //
////////////////////////////
void
STP::ReceiveFromDevice (Ptr<NetDevice> netDevice, Ptr<const Packet> pktBpdu, uint16_t protocol, Address const &src, Address const &dst, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
 
  //extract BPDU from packet
  BpduHeader bpdu = BpduHeader ();
  Ptr<Packet> p = pktBpdu->Copy ();
  p->RemoveHeader(bpdu);
  
  uint16_t inPort = netDevice->GetIfIndex();
  
  // if it is a TCN received
  if(bpdu.GetType()==TCN_TYPE)
    {
      if(m_stpActivateTopologyChange==1)
        {
          // check if the birdge has been switched on
          if( Simulator::Now() > Seconds(m_initTime) )
            {
              if(m_portInfo[inPort].role==DESIGNATED_ROLE)
                {
                  //trace message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] STP: Recvs TCN in a Designated Port");

                  // process the notification
                  TopologyChangeDetected(netDevice);     
                }
              else
                {
                  //trace message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] STP: Recvs TCN not in a Desginated Port");
                }
            }
        }
      else
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Received TCN when toplogy change is disabled");
          exit(1);
        }
    }
  // if it is a BPDU
  else if(bpdu.GetType()==BPDU_TYPE)
    {
  
      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] STP: Recvs BPDU [R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "|A:" << bpdu.GetMessAge() << "|Tc:" << bpdu.GetTcFlag() << "|Tca:" << bpdu.GetTcaFlag() << "]");

      // check if the birdge has been switched on
      if( Simulator::Now() > Seconds(m_initTime) )
        {
      
          //store received bpdu in port database (actually, this information is not used, but kept for possible later use)
          m_portInfo[inPort].receivedBpdu.rootId = bpdu.GetRootId();
          m_portInfo[inPort].receivedBpdu.cost = bpdu.GetCost();
          m_portInfo[inPort].receivedBpdu.bridgeId = bpdu.GetBridgeId();  
          m_portInfo[inPort].receivedBpdu.portId = bpdu.GetPortId();
          m_portInfo[inPort].receivedBpdu.messAge = bpdu.GetMessAge();
          
          // error if BPDU received in a disabled port
          if( m_portInfo[inPort].state==DISABLED_STATE )
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Received BPDU in Disabled port");
              exit(1);      
            }
          
          // check it is not expired: messageAge>maxAge (since it should be dicarded at transmission, this is an error)
          if(m_portInfo[inPort].receivedBpdu.messAge >= m_bridgeInfo.maxAge && m_stpActivateExpirationMaxAge == 1 ) // discard BPDU if messAge>maxAge
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Discarded Message Age at reception    (MessAge: "<< bpdu.GetMessAge() << ")");
              exit(1);
            }
          else //process bpdu if messAge<maxAge
            {  
              int16_t bpduUpdates = UpdatesInfo(m_portInfo[inPort].receivedBpdu, inPort);
              if(bpduUpdates == 1)  //if the bpdu updates the port information
                {      
                  //trace message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      Better from Designated"); //[R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "] < [R:" << m_portInfo[inPort].designatedRootId << "|C:" << m_portInfo[inPort].designatedCost << "|B:" << m_portInfo[inPort].designatedBridgeId << "|P:" << m_portInfo[inPort].designatedPortId << "]" );      

                  // cancel old timer and set a new updated timer, if this feature is activated
                  //if(m_stpActivateExpirationMaxAge == 1)
                    //{
                      m_portInfo[inPort].messAgeTimer.Remove();
                      m_portInfo[inPort].messAgeTimer.SetArguments(inPort);
                      m_portInfo[inPort].messAgeTimer.Schedule(Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
                      m_portInfo[inPort].lastMessAgeSchedule = Simulator::Now();
                    //}
                  
                  //store the received bpdu in the port info database
                  m_portInfo[inPort].designatedRootId = m_portInfo[inPort].receivedBpdu.rootId;
                  m_portInfo[inPort].designatedCost = m_portInfo[inPort].receivedBpdu.cost;
                  m_portInfo[inPort].designatedBridgeId = m_portInfo[inPort].receivedBpdu.bridgeId;
                  m_portInfo[inPort].designatedPortId = m_portInfo[inPort].receivedBpdu.portId;
                  m_portInfo[inPort].messAge = m_portInfo[inPort].receivedBpdu.messAge;
                  
                  //reconfigure the bridge
                  ReconfigureBridge();
                  
                  //check and process TC flags
                  if(inPort == m_bridgeInfo.rootPort)
                    {
                      CheckProcessTcFlags(bpdu.GetTcaFlag(), bpdu.GetTcFlag());
                    }
                                   
                  //send BPDUs to designated ports
                  for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                    {
                      if(m_portInfo[i].role == DESIGNATED_ROLE)
                        {
                          SendBPDU(m_ports[i], "disseminate better BPDU");
                        }
                    }
                }

              if(bpduUpdates == -1)  //if the bpdu does not updates the port information (worse)
                {
                  //trace message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      Worse from Designated");//[R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "] > [R:" << m_portInfo[inPort].designatedRootId << "|C:" << m_portInfo[inPort].designatedCost << "|B:" << m_portInfo[inPort].designatedBridgeId << "|P:" << m_portInfo[inPort].designatedPortId << "]" );      
                          
                  //a reply is sent
                  if(m_portInfo[inPort].role == DESIGNATED_ROLE)
                    {
                      SendBPDU(m_ports[inPort], "reply worse BPDU");
                    }
                }

              if(bpduUpdates == 0)  //if the bpdu has the same information than the port database
                {       
                  //traces
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      Equal from Designated");//[R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "] = [R:" << m_portInfo[inPort].designatedRootId << "|C:" << m_portInfo[inPort].designatedCost << "|B:" << m_portInfo[inPort].designatedBridgeId << "|P:" << m_portInfo[inPort].designatedPortId << "]" );      

                  // cancel old timer and set a new updated timer
                  //if(m_stpActivateExpirationMaxAge == 1)
                    //{
                      m_portInfo[inPort].messAgeTimer.Remove();
                      m_portInfo[inPort].messAgeTimer.SetArguments(inPort);
                      m_portInfo[inPort].messAgeTimer.Schedule(Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
                      m_portInfo[inPort].lastMessAgeSchedule = Simulator::Now();
                    //}
                   
                  //check and process TC flags
                  if(inPort == m_bridgeInfo.rootPort)
                    {
                      CheckProcessTcFlags(bpdu.GetTcaFlag(), bpdu.GetTcFlag());
                    }
                    
                  // if it is received in the root port, a bpdu to designateds is sent
                  if(inPort==m_bridgeInfo.rootPort)
                    {
                      for(uint16_t i=0 ; i < m_ports.size() ; i++)
                        {
                          if(m_portInfo[i].role==DESIGNATED_ROLE)
                            {
                              SendBPDU(m_ports[i], "disseminate equal BPDU");
                            } 
                        }
                    }
                }
            }  
        }// if bridge has been turned on            
    } // if a BPDU has been received
}


//////////////////////////////
// STP::CheckProcessTcFlags //
//////////////////////////////
void
STP::CheckProcessTcFlags(bool tcaFlag, bool tcFlag)
{
                   
  //if the BPDU has the TopChangeAck flag set and was received in a root port
  if(tcaFlag==true)
    {
      if(m_stpActivateTopologyChange==1)
        {
          //if root
          if(m_bridgeInfo.rootId == m_bridgeInfo.bridgeId)
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: Received TopChangeAck in the Root node");
              exit(1);   
            }
          //if not root
          else
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Non-Root node leaves Topology Change Notification Procedure (going up)");
                    
              // reset the flag TopChangeDetected flag in the bridge
              m_bridgeInfo.topChangeDetected=false;
              
              // cancel the topChangeNotifTimer in the bridge
              m_bridgeInfo.topChangeNotifTimer.Remove();
            }
        }
      else
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: Received TCA flag when toplogy change is disabled");
          exit(1);
        }
    }
  
  //set the topChange flag according to the received BPDU value
  bool previous_topChange = m_bridgeInfo.topChange;
  m_bridgeInfo.topChange = tcFlag;
  if(m_bridgeInfo.rootId != m_bridgeInfo.bridgeId)
    {
      if(previous_topChange != m_bridgeInfo.topChange)
        {
          if(m_stpActivateTopologyChange==1)
            {
              if(m_bridgeInfo.topChange==true)
                {
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Non-Root node enters Topology Change Procedure (going down)");

                  //notify the bridge of the change in the flag (so it has to expire earlier)
                  m_bridge->ChangeExpirationTime(m_bridgeInfo.maxAge + m_bridgeInfo.forwDelay);          
                }
            
              if(m_bridgeInfo.topChange==false)
                {
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Non-Root node leaves Topology Change Procedure (going down)");          

                  //notify the bridge of the change in the flag (so it has to expire later)
                  m_bridge->ChangeExpirationTime(300);                   
                }
            }
          else
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: Received TC flag when toplogy change is disabled");
              exit(1);
            }
        }
    }
}

/////////////////////////////////
// STP::TopologyChangeDetected //
/////////////////////////////////
void
STP::TopologyChangeDetected(Ptr<NetDevice> port)
{
  
  if(m_stpActivateTopologyChange==1)
    {
      // if root node
      if(m_bridgeInfo.rootId == m_bridgeInfo.bridgeId)
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Root node enters Topology Change Procedure (going down)");
          
          // start topChange timer
          m_bridgeInfo.topChangeTimer.Remove();      
          m_bridgeInfo.topChangeTimer.Schedule();
          
          // set topChange flag indicating the node is in a process of topology change (going down)
          bool previousTopChange = m_bridgeInfo.topChange;
          m_bridgeInfo.topChange = true;
               
          // notify bridge that the bridge has just entered this state, so it will have to start expiring earlier
          if (previousTopChange == false)
            {
              //notify the bridge of the change in the flag (so it has to expire earlier)
              m_bridge->ChangeExpirationTime(m_bridgeInfo.maxAge + m_bridgeInfo.forwDelay); 
            }
          
          
        }
      else //if not root node
        { 
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Non-Root node enters Topology Change Notification Procedure (going up)");
        
          // set topChangeNotif flag indicating the node is in a process of notifying a topology change (going up)
          m_bridgeInfo.topChangeDetected = true;
          
          // send a TCN to the root port
          SendTCN(m_ports[m_bridgeInfo.rootPort]);
         
          // start topChangeNotif timer
          m_bridgeInfo.topChangeNotifTimer.Remove();
          m_bridgeInfo.topChangeNotifTimer.Schedule();
          
        }
      
      //if the function has been called by a TCN received, send ack
      if(port!=0)
        {
          //set the port TCA flag
          m_portInfo[port->GetIfIndex()].topChangeAck = true;
         
          /*send BPDU
          if(m_portInfo[port->GetIfIndex()].role==DESIGNATED_ROLE)
            {
              SendBPDU (port, "Send BPDU with TopChangeAck");
            }
          else
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port->GetIfIndex() << "]      ERROR: Trying to send a TopChangeAck to a not Designated port");
              exit(1);              
            }*/
        }
    }
  else
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port->GetIfIndex() << "]      ERROR: Called TopologyChangeDetected methid when toplogy change is disabled");
      exit(1);
    }
}


////////////////////
//STP::UpdatesInfo//
////////////////////
int16_t
STP::UpdatesInfo(bpduInfo_t bpdu, uint16_t port)
{
  NS_LOG_FUNCTION_NOARGS ();
  
/*
  - follows the 4-step comparison
    - best Root Id
    - if equal Root Id: best Cost
    - if equal Cost: best Bridge Id
    - if equal Bridge Id: best Port Id
*/

  int16_t returnValue=2;
  
  NS_LOG_LOGIC("UpdatesInfo[" << m_bridgeInfo.bridgeId << "]: BPDU [R:" << bpdu.rootId << "|C:" << bpdu.cost << "|B:" << bpdu.bridgeId << "|P:" << bpdu.portId << "] <=> Port[" << port << "]: [R:" << m_portInfo[port].designatedRootId << "|C:" << m_portInfo[port].designatedCost << "|B:" << m_portInfo[port].designatedBridgeId << "|P:" << m_portInfo[port].designatedPortId << "]");
  
  if(bpdu.rootId < m_portInfo[port].designatedRootId)
    {
      returnValue = 1;
    }
  else
    {
      if(bpdu.rootId == m_portInfo[port].designatedRootId && bpdu.cost < m_portInfo[port].designatedCost)
        {
          returnValue = 1;
        }
      else
        {
          if(bpdu.rootId == m_portInfo[port].designatedRootId && bpdu.cost == m_portInfo[port].designatedCost && bpdu.bridgeId < m_portInfo[port].designatedBridgeId)
            {
              returnValue = 1;
            }
          else
            {
              if(bpdu.rootId == m_portInfo[port].designatedRootId && bpdu.cost == m_portInfo[port].designatedCost && bpdu.bridgeId == m_portInfo[port].designatedBridgeId && bpdu.portId < m_portInfo[port].designatedPortId)
                {
                  returnValue = 1;
                }
              else
                {
                  if(bpdu.rootId == m_portInfo[port].designatedRootId && bpdu.cost == m_portInfo[port].designatedCost && bpdu.bridgeId == m_portInfo[port].designatedBridgeId && bpdu.portId == m_portInfo[port].designatedPortId)
                    {
                      returnValue = 0;
                    }
                }
            }
        }
    }
  
  if(returnValue==2) returnValue = -1; //not changhed
  
  switch(returnValue)
    {
      case 1:
        NS_LOG_LOGIC("UpdatesInfo[" << m_bridgeInfo.bridgeId << "]: returns 1 (better)");
        return 1;
        break;
      case 0:
        NS_LOG_LOGIC("UpdatesInfo[" << m_bridgeInfo.bridgeId << "]: returns 0 (equal)");
        return 0;
        break;
      case -1:
        NS_LOG_LOGIC("UpdatesInfo[" << m_bridgeInfo.bridgeId << "]: returns -1 (worse)");
        return -1;
        break;
      default:
        NS_LOG_ERROR("ERROR in UpdatesBpdu(). Not correct value.");
        exit(1);
        break;
    }
  
}

//////////////////////////
//STP::ReconfigureBridge//
//////////////////////////
void
STP::ReconfigureBridge()
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // printout the state of the bridge in a single line
  NS_LOG_LOGIC ("Bridge and port states before reconfiguration");
  NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      Brdg Info  [R:" << m_bridgeInfo.rootId << "|C:" << m_bridgeInfo.cost << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort << "]:[TCD:" << m_bridgeInfo.topChangeDetected << "|TC:" << m_bridgeInfo.topChange << "]");

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]      Port Info  [R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[TCA:" << m_portInfo[p].topChangeAck << "]");
    }
  
  //select new root port
  //////////////////////
  int32_t rootPortTmp=-1;
  uint64_t minRootId = 10000;
  uint32_t minCost = 10000;
  uint64_t minBridgeId = 10000;
  uint16_t minPortId = 10000;
  
  std::string previous_role;
  
  bool foundTmp=false;
  
  //select minimum root, from the port database
  for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
    {
      if(m_portInfo[i].designatedRootId < minRootId && m_portInfo[i].designatedBridgeId!=m_bridgeInfo.bridgeId && m_portInfo[i].state!=DISABLED_STATE)
        {
          minRootId=m_portInfo[i].designatedRootId;
          foundTmp = true;
        }
    }
 
  if(m_bridgeInfo.bridgeId < minRootId) // if i am the best bridge, then root
    {
      minRootId=m_bridgeInfo.bridgeId;
    }
  
  if(minRootId != m_bridgeInfo.bridgeId && m_bridgeInfo.rootId==m_bridgeInfo.bridgeId) // If I was the root, but not any more
    {
      if(m_bridgeInfo.topChange == true && m_stpActivateTopologyChange==1)
        {
          m_bridgeInfo.topChange = false;
          m_bridgeInfo.topChangeTimer.Remove();
      
          //notify the bridge of the change in the flag (so it has to expire later again)
          m_bridge->ChangeExpirationTime(300); 
        }
    }

  m_bridgeInfo.rootId = minRootId;
    
  NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: found root node " << m_bridgeInfo.rootId);       
  
  if( minRootId == m_bridgeInfo.bridgeId)   //if i'm the root, there is no need to look for a root port
    {
      m_bridgeInfo.rootPort=-1;
      m_bridgeInfo.cost=0;
      
      //set the periodical BPDUs
      if( m_activatePeriodicalBpdus==true )
        {
          if(m_bridgeInfo.helloTimeTimer.IsExpired())
            {
              m_bridgeInfo.helloTimeTimer.Schedule();
            }
        }
    }
  else //the root is someone else (or maybe i still don't know i am the root..., i will notice it once i see i only have designateds)
    {
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++) //from all ports...
        {
          if(m_portInfo[i].state!=DISABLED_STATE)
            {
              if(m_portInfo[i].designatedRootId == m_bridgeInfo.rootId && m_portInfo[i].designatedBridgeId!=m_bridgeInfo.bridgeId) //...that are aware of the right minimum root and are not one of the designated ports (because these have local info)
                { 
                  if(m_portInfo[i].designatedCost < minCost)
                    {
                      rootPortTmp = i;
                      minCost = m_portInfo[i].designatedCost;
                      minBridgeId = m_portInfo[i].designatedBridgeId;
                      minPortId = m_portInfo[i].designatedPortId;
                    }
                  else
                    {
                      if(m_portInfo[i].designatedCost == minCost && m_portInfo[i].designatedBridgeId < minBridgeId)
                        {
                          rootPortTmp = i;
                          minCost = m_portInfo[i].designatedCost;
                          minBridgeId = m_portInfo[i].designatedBridgeId;
                          minPortId = m_portInfo[i].designatedPortId; 
                        }
                      else
                        {
                          if(m_portInfo[i].designatedCost == minCost && m_portInfo[i].designatedBridgeId == minBridgeId && m_portInfo[i].designatedPortId < minPortId)
                            {
                              rootPortTmp = i;
                              minCost = m_portInfo[i].designatedCost;
                              minBridgeId = m_portInfo[i].designatedBridgeId;
                              minPortId = m_portInfo[i].designatedPortId;
                            }
                          else
                            {
                              if(m_portInfo[i].designatedCost == minCost && m_portInfo[i].designatedBridgeId == minBridgeId && m_portInfo[i].designatedPortId == minPortId && i < rootPortTmp)
                                {
                                  rootPortTmp = i;
                                  minCost = m_portInfo[i].designatedCost;
                                  minBridgeId = m_portInfo[i].designatedBridgeId;
                                  minPortId = m_portInfo[i].designatedPortId;
                                }
                            }
                        }
                    }
                } // if (...that are aware of the right minimum root
            } //if not disabled
        } //for (all ports)
        
      if(rootPortTmp == -1) //no minimum root has been found -> i'm the root bridge
        {  
          NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: I am the root (only Designateds)");   
          m_bridgeInfo.rootId   =m_bridgeInfo.bridgeId;
          m_bridgeInfo.rootPort =-1;
          m_bridgeInfo.cost     =0;
        }
      else
        {
          std::string previous_role_rootPort = m_portInfo[rootPortTmp].role;

          NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: I am not the root: store rootport and make it listening if it was alternate (" << previous_role_rootPort << ")");   
          
          m_bridgeInfo.rootPort = rootPortTmp;
          m_portInfo[m_bridgeInfo.rootPort].role = ROOT_ROLE;
          m_bridgeInfo.cost = minCost + 1;
          if(previous_role_rootPort == ALTERNATE_ROLE) 
            {
              MakeListening(rootPortTmp);
            }    
        }     
      
    } //else (the root is someone else)

  NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: found root port " << m_bridgeInfo.rootPort);

  //from others, select designated port(s)
  ////////////////////////////////////////
  for(uint16_t i=0 ; i < m_portInfo.size() ; i++) //for all ports
    {
      previous_role = m_portInfo[i].role;

      if(m_portInfo[i].state!=DISABLED_STATE)
        {
          if(i != m_bridgeInfo.rootPort)  //except the root port (above selected)
            { 
              if(m_portInfo[i].designatedRootId != m_bridgeInfo.rootId || m_portInfo[i].designatedBridgeId==m_bridgeInfo.bridgeId ) //not completely updated information, or info from the same bridge
                { 
                  //previous_role = m_portInfo[i].role;
                  m_portInfo[i].role=DESIGNATED_ROLE;
                }
              else
                {
                  if(m_portInfo[i].designatedCost > m_bridgeInfo.cost)
                    {
                      //previous_role = m_portInfo[i].role;
                      m_portInfo[i].role=DESIGNATED_ROLE;
                    }
                  else
                    {
                      if(m_portInfo[i].designatedCost < m_bridgeInfo.cost)
                        {
                          //previous_role = m_portInfo[i].role;
                          m_portInfo[i].role=ALTERNATE_ROLE;
                        }
                        
                      if(m_portInfo[i].designatedCost == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId > m_bridgeInfo.bridgeId)
                        {
                          //previous_role = m_portInfo[i].role;
                          m_portInfo[i].role=DESIGNATED_ROLE;
                        }
                      else
                        {
                          if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId < m_bridgeInfo.bridgeId)
                            {
                              //previous_role = m_portInfo[i].role;
                              m_portInfo[i].role=ALTERNATE_ROLE;
                            }
                          if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId == m_bridgeInfo.bridgeId && m_portInfo[i].designatedPortId > i)
                            {
                              //previous_role = m_portInfo[i].role;
                              m_portInfo[i].role=DESIGNATED_ROLE;
                            }
                          else
                            {
                              if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId == m_bridgeInfo.bridgeId && m_portInfo[i].designatedPortId < i)
                                {
                                  //previous_role = m_portInfo[i].role;
                                  m_portInfo[i].role=ALTERNATE_ROLE;
                                }
                              if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId == m_bridgeInfo.bridgeId && m_portInfo[i].designatedPortId == i) // tot igual, soc jo el designated
                                { 
                                  //previous_role = m_portInfo[i].role;
                                  m_portInfo[i].role=DESIGNATED_ROLE;        
                                }
                            }
                        }    
                    }   
                } //else (not completely updated information)
                      
              if(m_portInfo[i].role==DESIGNATED_ROLE) //if designated, save bridge information
                { 
                  NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: found designated port " << i);
                  m_portInfo[i].designatedPortId = i;
                  m_portInfo[i].designatedBridgeId = m_bridgeInfo.bridgeId;
                  m_portInfo[i].designatedRootId = m_bridgeInfo.rootId;
                  m_portInfo[i].designatedCost = m_bridgeInfo.cost;
                  m_portInfo[i].messAgeTimer.Remove(); // cancel messAge timer
                }
                        
            } //if (... except the root port)
            
            // apply state change, timers... in here for the port (designated or alternate) being looped now
            if(m_portInfo[i].role==ALTERNATE_ROLE)
              {
                NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: found alternate port " << i);
                //m_portInfo[i].messAgeTimer.Remove(); // cancel messAge timer. WHY??? it has info from the Designated in that LAN???
                MakeBlocking(i); //block this port
              }
            if(previous_role == ALTERNATE_ROLE && m_portInfo[i].role != ALTERNATE_ROLE) 
              {
                NS_LOG_LOGIC("ReconfigureBridge[" << m_bridgeInfo.bridgeId << "]: Port " << i << " from Alternate to not Alternate ");
                MakeListening(i);
              }
        } // if not disabled      
    } // for all ports...

  // printout the state of the bridge in a single line
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      Brdg Info  [R:" << m_bridgeInfo.rootId << "|C:" << m_bridgeInfo.cost << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort << "]:[TCD:" << m_bridgeInfo.topChangeDetected << "|TC:" << m_bridgeInfo.topChange << "]");
  
  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]      Port Info  [R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[TCA:" << m_portInfo[p].topChangeAck << "]");   
    }
}


///////////////////
// STP::SendBPDU //
///////////////////
void
STP::SendBPDU (Ptr<NetDevice> outPort, std::string debugComment)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t p = outPort->GetIfIndex();
  Time rooPortMessAge;
  Time rooPortMessAgeTimer;

  if(Simulator::Now() < Time(Seconds(m_simulationtime)))
    {
      if(m_portInfo[p].state!=DISABLED_STATE)
        {
          if(m_portInfo[p].txHoldCount < m_bridgeInfo.maxBpduTxHoldPeriod) //if no BPDUs send during last period, we can safley sent
            {
              uint16_t protocol = 9999; //should be the LLC protocol
              Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
              Mac48Address dst = Mac48Address("01:80:C2:00:00:00");

              // create the BPDU
              Ptr<Packet> bpdu = Create<Packet> (0);
              
              // Add BPDU header
              BpduHeader bpduFields = BpduHeader ();
              bpduFields.SetType(BPDU_TYPE);
              bpduFields.SetRootId(m_portInfo[p].designatedRootId);
              bpduFields.SetCost(m_portInfo[p].designatedCost);
              bpduFields.SetBridgeId(m_portInfo[p].designatedBridgeId);
              bpduFields.SetPortId(p);
              bpduFields.SetTcFlag(m_bridgeInfo.topChange);
              bpduFields.SetTcaFlag(m_portInfo[p].topChangeAck);

              if(m_bridgeInfo.rootId == m_bridgeInfo.bridgeId) bpduFields.SetMessAge(0); // it is the root
              else
                {
                  rooPortMessAge = Seconds(m_portInfo[m_bridgeInfo.rootPort].messAge);
                  rooPortMessAgeTimer = Simulator::Now() - m_portInfo[m_bridgeInfo.rootPort].lastMessAgeSchedule;      
                  /*NS_LOG_LOGIC("rooPortMessAge.GetSeconds(): " << rooPortMessAge.GetSeconds());
                  NS_LOG_LOGIC("rooPortMessAgeTimer.GetSeconds(): " << rooPortMessAgeTimer.GetSeconds());
                  NS_LOG_LOGIC("rooPortMessAge + rooPortMessAgeTimer: " << rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds());
                  NS_LOG_LOGIC("Previous + inc: " << rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc);
                  NS_LOG_LOGIC("floor of previous: " << floor ( rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc ) );
                  NS_LOG_LOGIC("uint16_t of previous: " << (uint16_t) floor ( rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc ) );*/
                  
                  if(m_stpActivateIncreaseMessAge==1)
                    {
                      bpduFields.SetMessAge( (uint16_t) floor ( rooPortMessAge.GetSeconds() + rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc ) ); // it is not the root
                    }
                  else
                    {
                      bpduFields.SetMessAge(0);
                    }
                    
                }
              
              if(bpduFields.GetMessAge() < m_bridgeInfo.maxAge)
                {            
                  bpdu->AddHeader(bpduFields);
                  
                  //update txHold counter and indicate the pending BPDU is not a TCN
                  m_portInfo[p].txHoldCount ++;

                  // debug message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] STP: Sends BPDU [R:" << bpduFields.GetRootId() << "|C:" << bpduFields.GetCost() << "|B:" << bpduFields.GetBridgeId() << "|P:" << bpduFields.GetPortId() << "|A:" << bpduFields.GetMessAge() << "|Tc:" << bpduFields.GetTcFlag() << "|Tca:" << bpduFields.GetTcaFlag() << "] (TxHoldCount:"  << m_portInfo[p].txHoldCount << ") - (" << debugComment << ")");
                  
                  // send to the out port
                  outPort->SendFrom (bpdu, src, dst, protocol);
                }
              else
                {
                  //mess age expired, discard before sending
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] STP: Discarded BPDU due to MessAge >= MaxAge [R:" << bpduFields.GetRootId() << "|C:" << bpduFields.GetCost() << "|B:" << bpduFields.GetBridgeId() << "|P:" << bpduFields.GetPortId() << "|A:" << bpduFields.GetMessAge() << "|Tc:" << bpduFields.GetTcFlag() << "|Tca:" << bpduFields.GetTcaFlag() << "]");
                }
                
              // reset because it is set just before calling SendBPDU
              m_portInfo[p].topChangeAck=false;
              
            }
          else
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] STP: Holds BPDU (TxCount: " << m_portInfo[p].txHoldCount << " => " <<  m_portInfo[p].txHoldCount+1 << ")");
              m_portInfo[p].txHoldCount ++; // even if we have not sent, keep counting to differentiate 1 sent from 1 sent and 1, or more, hold
            } 
        }
    }  
}


////////////////////////////
// STP::SendHelloTimeBpdu //
////////////////////////////
void
STP::SendHelloTimeBpdu()
{
  if( m_bridgeInfo.rootId == m_bridgeInfo.bridgeId && Simulator::Now() < Time(Seconds(m_simulationtime))) // if i am the root
    {
    
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Periodical BPDUs");
      
      //Mac48Address HwDestination = Mac48Address("00:11:22:33:44:55"); //this must be the 'to all bridges' mac address
     
      for(uint16_t i=0 ; i < m_ports.size() ; i++)
        {
          if(m_portInfo[i].role==DESIGNATED_ROLE){
              SendBPDU(m_ports[i], "periodical BPDU");

          } 
        }
      
      //schedule next periodical BPDUs (only if m_activatePeriodicalBpdus)
      if( m_activatePeriodicalBpdus==true ){
        if(m_bridgeInfo.helloTimeTimer.IsExpired())
          {
            m_bridgeInfo.helloTimeTimer.Schedule();
          }
        else
          {
            NS_LOG_ERROR("STP: Trying to schedule a helloTimeTimer while there is another scheduled");
            exit(1);
          }
        
      }
    }
}

//////////////////
// STP::SendTCN //
//////////////////
void
STP::SendTCN (Ptr<NetDevice> rootPort)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t p = rootPort->GetIfIndex();

  if(Simulator::Now() < Time(Seconds(m_simulationtime)))
    {
      if(m_portInfo[p].state==FORWARDING_STATE)
        {
          uint16_t protocol = 9999; //should be the LLC protocol
          Mac48Address src = Mac48Address("AA:AA:AA:AA:AA:AA");// whatever, testing, should be the management MAC of the sending bridge ?
          Mac48Address dst = Mac48Address("01:80:C2:00:00:00");

          // create the BPDU
          Ptr<Packet> bpdu = Create<Packet> (0);
          
          // Add TCN header
          BpduHeader bpduFields = BpduHeader ();
          bpduFields.SetType(TCN_TYPE);             
          bpdu->AddHeader(bpduFields);
              
          // debug message
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << rootPort->GetIfIndex() << "] STP: Sends TCN");
          
          // send to the out port
          rootPort->SendFrom (bpdu, src, dst, protocol);
        }
    }  
}

///////////////////////////
// STP::ResetTxHoldCount //
///////////////////////////
void STP::ResetTxHoldCount()
{
  if (Simulator::Now() < Time(Seconds(m_simulationtime)))
    {  
      uint16_t lastTxHoldCount;
      
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Clr TxHold");
      
      for(uint16_t p=0 ; p < m_ports.size() ; p++)
        { 
          if(m_portInfo[p].state!=DISABLED_STATE)
            {
              lastTxHoldCount = m_portInfo[p].txHoldCount;
              m_portInfo[p].txHoldCount = 0;
            
              NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "] STP: Clr TxHold (TxCount: " << lastTxHoldCount << "=>0)");
              
              if(m_portInfo[p].role==DESIGNATED_ROLE && lastTxHoldCount > m_bridgeInfo.maxBpduTxHoldPeriod) //more than the maximum number of BPDUs sent, so at least one is pending
                { 
                  SendBPDU(m_ports[p], "pending txHold"); //send pending bpdu now
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
          NS_LOG_ERROR("STP: Trying to schedule a txHoldCount while there is another scheduled");
          exit(1);
        }
    }
}

///////////////////////
// STP::MakeBlocking //
///////////////////////
void STP::MakeBlocking (uint16_t port)
{
  m_portInfo[port].state=BLOCKING_STATE;
  m_portInfo[port].listeningTimer.Remove();
  m_portInfo[port].learningTimer.Remove();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] STP: BLK State");
  
  //notify simGod
  m_simGod->SetActivePort(0, m_node->GetId(), port, false); // singel tree, so tree 0
  
  //notify bridge modele of the blocked port
  m_bridge->SetPortState(-1, port, false); // set to blocking - tree is -1 because single tree
  
  //start a topology change notification process
  if(m_stpActivateTopologyChange==1) TopologyChangeDetected(0);
}

////////////////////////
// STP::MakeListening //
////////////////////////
void STP::MakeListening (uint16_t port)
{
  m_portInfo[port].state=LISTENING_STATE;    
  m_portInfo[port].listeningTimer.Remove();
  m_portInfo[port].learningTimer.Remove();
  m_portInfo[port].listeningTimer.SetArguments(port);  
  m_portInfo[port].listeningTimer.Schedule();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] STP: LIS State");
  m_simGod->SetActivePort(0, m_node->GetId(), port, false); // singel tree, so tree 0
}

///////////////////////
// STP::MakeLearning //
///////////////////////
void STP::MakeLearning (uint16_t port)
{
  m_portInfo[port].state=LEARNING_STATE;    
  m_portInfo[port].listeningTimer.Remove();
  m_portInfo[port].learningTimer.Remove();
  m_portInfo[port].learningTimer.SetArguments(port);  
  m_portInfo[port].learningTimer.Schedule();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] STP: LRN State");
}

/////////////////////////
// STP::MakeForwarding //
/////////////////////////
void STP::MakeForwarding (uint16_t port)
{
  m_portInfo[port].state=FORWARDING_STATE;
  m_portInfo[port].listeningTimer.Remove();
  m_portInfo[port].learningTimer.Remove();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] STP: FRW State");
  
  //notify simGod 
  m_simGod->SetActivePort(0, m_node->GetId(), port, true); // singel tree, so tree 0

  //notify bridge modele of the blocked port
  m_bridge->SetPortState(-1, port, true); // set to forwarding - tree is -1 because single tree
  
  //start a topology change notification process
  if(m_stpActivateTopologyChange==1) TopologyChangeDetected(0);
  
}

////////////////////////////
// STP::MessageAgeExpired //
////////////////////////////
void STP::MessageAgeExpired(uint16_t port)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)) && m_stpActivateExpirationMaxAge==1 )
    {
      //cancel timer
      m_portInfo[port].messAgeTimer.Remove();
      
      //clear the port info of the expired port with the 'own' info (assuming it will be the root)
      m_portInfo[port].designatedRootId = m_bridgeInfo.rootId;
      m_portInfo[port].designatedCost = m_bridgeInfo.cost;
      m_portInfo[port].designatedBridgeId = m_bridgeInfo.bridgeId;
      m_portInfo[port].designatedPortId = m_ports[port]->GetIfIndex();
      //m_portInfo[port].messAge = 0;       ??????????

      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] STP: MessAgeExp [R:" << m_portInfo[port].designatedRootId << "|C:" << m_portInfo[port].designatedCost << "|B:" << m_portInfo[port].designatedBridgeId << "|P:" << m_portInfo[port].designatedPortId << "|A:" << m_portInfo[port].messAge << "]");      

      //reconfigure the bridge to select new port roles
      ReconfigureBridge();
        
      //send BPDU to designated ports
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          if(m_portInfo[i].role == DESIGNATED_ROLE)
            {
              SendBPDU(m_ports[i], "maxAge expiration");
            }
        }
    } 
}

/////////////////////////////////
// STP::PhyLinkFailureDetected //
/////////////////////////////////
void STP::PhyLinkFailureDetected(uint16_t port)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)))
    {
      //cancel timers
      m_portInfo[port].phyLinkFailDetectTimer.Remove();
      m_portInfo[port].messAgeTimer.Remove();
                  
      //clear the port info of the expired port with the 'own' info (assuming it will be the root)
      m_portInfo[port].state=DISABLED_STATE;
      m_portInfo[port].role=DESIGNATED_ROLE;
      m_portInfo[port].designatedRootId = m_bridgeInfo.rootId;
      m_portInfo[port].designatedCost = m_bridgeInfo.cost;
      m_portInfo[port].designatedBridgeId = m_bridgeInfo.bridgeId;
      m_portInfo[port].designatedPortId = m_ports[port]->GetIfIndex();
      //m_portInfo[port].messAge = 0;       ??????????

      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] STP: Physical Link Failure Detected [R:" << m_portInfo[port].designatedRootId << "|C:" << m_portInfo[port].designatedCost << "|B:" << m_portInfo[port].designatedBridgeId << "|P:" << m_portInfo[port].designatedPortId << "|A:" << m_portInfo[port].messAge << "]");      

      //reconfigure the bridge to select new port roles
      ReconfigureBridge();
        
      //send BPDU to designated ports
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          if(m_portInfo[i].role == DESIGNATED_ROLE)
            {
              SendBPDU(m_ports[i], "phy link failure detected");
            }
        }
    } 
}

//////////////////////////////
// STP::ScheduleLinkFailure //
//////////////////////////////
void
STP::ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port)
{  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] STP: Setting Link Failure Physical Detection in port " << port << " at time " << linkFailureTimeUs);

  //configure messAge timer
  m_portInfo[port].phyLinkFailDetectTimer.Remove();
  m_portInfo[port].phyLinkFailDetectTimer.SetArguments(port);  
  m_portInfo[port].phyLinkFailDetectTimer.Schedule(linkFailureTimeUs); 
}

////////////////////////////////
// STP::TopChangeTimerExpired //
////////////////////////////////
void STP::TopChangeTimerExpired()
{
  if(m_stpActivateTopologyChange==1)
    {
      // if root node
      if(m_bridgeInfo.rootId == m_bridgeInfo.bridgeId)
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] STP: Root node leaves Topology Change Procedure (going down)");
        
          //reset the topChange flag (not in the topology change situation any more)
          m_bridgeInfo.topChange = false;
         
          //notify the bridge of the change in the flag (so it has to expire later again)
          m_bridge->ChangeExpirationTime(300); 
        }
      //if not root node
      else
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: TopChangeTimer expired in non root node");
          exit(1); 
        }
    }
  else
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: TopChangeTimerExpired when toplogy change is disabled");
      exit(1);
    }
}

/////////////////////////////////////
// STP::TopChangeNotifTimerExpired //
/////////////////////////////////////
void STP::TopChangeNotifTimerExpired()
{
  if(m_stpActivateTopologyChange==1)
    {
      // if not root node
      if(m_bridgeInfo.rootId != m_bridgeInfo.bridgeId)
        {
          //send TCN to root port
          SendTCN(m_ports[m_bridgeInfo.rootPort]);
          
          //schedule next call to this function
          m_bridgeInfo.topChangeNotifTimer.Remove();
          m_bridgeInfo.topChangeNotifTimer.Schedule();
        }
      //if root node
      else
        {  
          //NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: TopChangeNotifTimer expired in root node");
          //exit(1); 
        }
    }
  else
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]      ERROR: TopChangeNotifTimerExpired when toplogy change is disabled");
      exit(1);
    }  
}

////////////////////
// STP::DoDispose //
////////////////////
void
STP::DoDispose (void)
{
  // bridge timers
  m_bridgeInfo.helloTimeTimer.Cancel();
  m_bridgeInfo.txHoldCountTimer.Cancel();
    
  // port timers
  for(uint16_t p=0 ; p < m_ports.size() ; p++)
    {
      m_portInfo[p].listeningTimer.Cancel();
      m_portInfo[p].learningTimer.Cancel();
      m_portInfo[p].messAgeTimer.Cancel();
    }
}  

} //namespace ns3
      
  
