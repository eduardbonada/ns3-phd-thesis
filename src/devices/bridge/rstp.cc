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

#include "rstp.h"
#include "bridge-net-device.h"
#include "bpdu-header.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include <stdlib.h>
#include <cmath>

NS_LOG_COMPONENT_DEFINE ("RSTP");

namespace ns3 {

TypeId
RSTP::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RSTP")
    .SetParent<Object> ()
    .AddConstructor<RSTP> ()
    .AddTraceSource ("RstpRcvBPDU", 
                     "Trace source indicating a BPDU has been received by the RSTP module (still not used)",
                     MakeTraceSourceAccessor (&RSTP::m_rstpRcvBpdu))
    ;
  return tid;
}

RSTP::RSTP()
{
  NS_LOG_FUNCTION_NOARGS ();
}
  
RSTP::~RSTP()
{
  NS_LOG_FUNCTION_NOARGS ();
}

//////////////////
// RSTP::Install //
//////////////////
void
RSTP::Install (
    Ptr<Node> node,
    Ptr<BridgeNetDevice> bridge,
    NetDeviceContainer ports,
    uint64_t bridgeId,
    double initTime,    
    uint16_t rstpSimulationTimeSecs,
    uint16_t rstpActivatePeriodicalBpdus,
    uint16_t rstpActivateExpirationMaxAge,
    uint16_t rstpActivateIncreaseMessAge,
    uint16_t rstpActivateTopologyChange, 
    uint16_t rstpMaxAge,
    uint16_t rstpHelloTime,
    uint16_t rstpForwDelay,
    double   rstpTxHoldPeriod,
    uint32_t rstpMaxBpduTxHoldPeriod,
    uint32_t rstpResetTxHoldPeriod,
    Ptr<SimulationGod> god
    )
{
  m_node = node;
  m_bridge = bridge;
  
  m_bridgeInfo.messAgeInc = 1;
  
  m_simGod = god; 
  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] RSTP: Installing RSTP with bridge Id " << bridgeId << " starting at " << initTime);
          
  // link to each of the ports of the bridge to be able to directly send BPDUs to them
  for (NetDeviceContainer::Iterator i = ports.Begin (); i != ports.End (); ++i)
    {
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] RSTP: Link to port " << (*i)->GetIfIndex() << " of node " << (*i)->GetNode()->GetId());
      m_ports.push_back (*i);
      
      //Register the m_RxRstpCallback for each net device in the protocol handler
      NS_LOG_INFO ("Node[" << m_node->GetId() << "] RSTP: RegisterProtocolHandler for " << (*i)->GetIfIndex());
      m_node->RegisterProtocolHandler (MakeCallback (&RSTP::ReceiveFromBridge, this), 9999, *i, true); // protocol = 0 (any, because we use the field to pass portId), promisc = false
      
      portInfo_t pI;
      m_portInfo.push_back(pI);
    }
  
  //general node variables
  m_simulationtime               = rstpSimulationTimeSecs;
  m_activatePeriodicalBpdus      = rstpActivatePeriodicalBpdus;
  m_rstpActivateExpirationMaxAge = rstpActivateExpirationMaxAge;
  m_rstpActivateIncreaseMessAge  = rstpActivateIncreaseMessAge;
  m_rstpActivateTopologyChange   = rstpActivateTopologyChange;
  m_initTime                    = initTime;
  
  //init bridge info to root
  m_bridgeInfo.rootId       = bridgeId;
  m_bridgeInfo.cost         = 0;
  m_bridgeInfo.bridgeId     = bridgeId;
  m_bridgeInfo.rootPort     = -1; //the root bridge has not root port
  m_bridgeInfo.maxAge       = rstpMaxAge;
  m_bridgeInfo.helloTime    = rstpHelloTime;
  m_bridgeInfo.forwDelay    = rstpForwDelay;
  m_bridgeInfo.txHoldPeriod        = rstpTxHoldPeriod; //period accounting BPDUs
  m_bridgeInfo.maxBpduTxHoldPeriod = rstpMaxBpduTxHoldPeriod; //number of bpdus per m_txHoldPeriod period
  m_bridgeInfo.resetTxHoldPeriod   = rstpResetTxHoldPeriod; // whether the value is reset or decremented

  //init all port info to designated - discarding
  for (uint16_t i=0 ; i < m_portInfo.size () ; i++)
    {
      m_portInfo[i].state = DISCARDING_STATE;
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
      m_portInfo[i].topChange = false;
      m_portInfo[i].topChangeAck = false;
      
      //configure port variables
      m_portInfo[i].proposed  = false;
      m_portInfo[i].proposing = true;
      m_portInfo[i].sync      = false;
      m_portInfo[i].synced    = true;
      m_portInfo[i].agreed    = false;
      m_portInfo[i].newInfo   = true;
     
      //configure toLearningTimer and toForwardingTimer and recentRootTimer values
      m_portInfo[i].toLearningTimer.SetFunction(&RSTP::MakeLearning,this);
      m_portInfo[i].toLearningTimer.SetDelay(Seconds(m_bridgeInfo.forwDelay));
      m_portInfo[i].toForwardingTimer.SetFunction(&RSTP::MakeForwarding,this);
      m_portInfo[i].toForwardingTimer.SetDelay(Seconds(m_bridgeInfo.forwDelay));
      m_portInfo[i].recentRootTimer.SetFunction(&RSTP::RecentRootTimerExpired,this);
      m_portInfo[i].recentRootTimer.SetDelay(Seconds(m_bridgeInfo.forwDelay));

      //configure topChange value
      m_portInfo[i].topChangeTimer.SetFunction(&RSTP::TopChangeTimerExpired,this);
      m_portInfo[i].topChangeTimer.SetDelay(Seconds(m_bridgeInfo.helloTime + 1));      
            
      // set state and start toLearningTimer
      MakeDiscarding(i);
      m_portInfo[i].toLearningTimer.SetArguments(i);
      m_portInfo[i].toLearningTimer.Schedule();

      //configure messAge timer
      //if(m_rstpActivateExpirationMaxAge == 1)
        //{
          m_portInfo[i].messAgeTimer.SetFunction(&RSTP::MessageAgeExpired,this);
          m_portInfo[i].messAgeTimer.SetDelay(Seconds(3*m_bridgeInfo.helloTime)); //in RSTP, the value does not directly depend on the received MessAge (it is either 3xHelloTime, if received MessAge<MaxAge or 0, otherwise)
        //}
      
      //configure physical link failure detection. it will be scheduled if function called from main simulation config file
      m_portInfo[i].phyLinkFailDetectTimer.SetFunction(&RSTP::PhyLinkFailureDetected,this);
      
      //init txCoutn to 0
      m_portInfo[i].txHoldCount=0;
    }
    
  //configure and schedule txHoldCountTimer
  m_bridgeInfo.txHoldCountTimer.SetFunction(&RSTP::ResetTxHoldCount,this);
  m_bridgeInfo.txHoldCountTimer.SetDelay(Seconds(m_bridgeInfo.txHoldPeriod));
  m_bridgeInfo.txHoldCountTimer.Schedule(Seconds(initTime));
  
  //configure and schedule HelloTimeTimer
  m_bridgeInfo.helloTimeTimer.SetFunction(&RSTP::SendHelloTimeBpdu,this);
  m_bridgeInfo.helloTimeTimer.SetDelay(Seconds(m_bridgeInfo.helloTime));
  m_bridgeInfo.helloTimeTimer.Schedule(Seconds(initTime));
  
}

////////////////////////////
// RSTP::ReceiveFromBridge //
////////////////////////////
void
RSTP::ReceiveFromBridge (Ptr<NetDevice> netDevice, Ptr<const Packet> pktBpdu, uint16_t protocol, Address const &src, Address const &dst, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  //extract BPDU from packet
  BpduHeader bpdu = BpduHeader ();
  Ptr<Packet> p = pktBpdu->Copy ();
  p->RemoveHeader(bpdu);
  
  uint16_t inPort = netDevice->GetIfIndex();
  
  //trace message
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "] RSTP: Recvs BPDU [R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "|A:" << bpdu.GetMessAge() << "|Tc:" << bpdu.GetTcFlag() << "|Tca:" << bpdu.GetTcaFlag() << "|" << bpdu.GetRoleFlags() << (bpdu.GetPropFlag()?"P":"-") << (bpdu.GetAgrFlag()?"A":"-") << (bpdu.GetLrnFlag()?"L":"-") << (bpdu.GetFrwFlag()?"F":"-") << "] - (link id: " << m_ports[inPort]->GetChannel()->GetId() << ")");

  // check if the birdge has been switched on
  if( Simulator::Now() > Seconds(m_initTime) )
  {

  //store received bpdu in port database (actually, this information is not used, but kept for possible later use)
  m_portInfo[inPort].receivedBpdu.rootId = bpdu.GetRootId();
  m_portInfo[inPort].receivedBpdu.cost = bpdu.GetCost();
  m_portInfo[inPort].receivedBpdu.bridgeId = bpdu.GetBridgeId();  
  m_portInfo[inPort].receivedBpdu.portId = bpdu.GetPortId();
  m_portInfo[inPort].receivedBpdu.messAge = bpdu.GetMessAge();
  
  // errot if BPDU received in a disabled port
  if( m_portInfo[inPort].state==DISABLED_STATE )
    {
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Received BPDU in Disabled port");
      exit(1);      
    }

  // CHANGED FROM SPBDV      
  // check it is not expired: messageAge>maxAge
  if(m_portInfo[inPort].receivedBpdu.messAge >= m_bridgeInfo.maxAge /* && m_rstpActivateExpirationMaxAge == 1*/) // discard BPDU if messAge>maxAge
    {
       if(m_portInfo[inPort].role==ROOT_ROLE || m_portInfo[inPort].role==ALTERNATE_ROLE)
         {

          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      Received Message Age higher than MaxAge (MessAge: "<< bpdu.GetMessAge() << ") in R/A port");
       
           // set variables
           m_portInfo[inPort].messAge = bpdu.GetMessAge();
           //m_portInfo[inPort].synced.at(tree)     = false;
           //m_portInfo[inPort].proposing.at(tree)  = false;
           //m_portInfo[inPort].agreed.at(tree)     = false;
           //m_portInfo[inPort].proposed.at(tree)=false;
           
           // expire the information
           MessageAgeExpired(inPort);               
         }
       else
         {
           NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      Received Message Age higher than MaxAge (MessAge: "<< bpdu.GetMessAge() << ") in D port");               
         }

      //NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]      ERROR: Discarded Message Age at reception    (MessAge: "<< bpdu.GetMessAge() << ")");
      //exit(1);  
    }
  else //process bpdu if messAge<maxAge
    {      
      int16_t bpduUpdates = UpdatesInfo(m_portInfo[inPort].receivedBpdu, inPort);
         
      // if bpdu is better from designated
      if(bpduUpdates == 1 && bpdu.GetRoleFlags()==DESIGNATED_ROLE)
        {
          //trace message
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Better from Designated");     //;[R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "] < [R:" << m_portInfo[inPort].designatedRootId << "|C:" << m_portInfo[inPort].designatedCost << "|B:" << m_portInfo[inPort].designatedBridgeId << "|P:" << m_portInfo[inPort].designatedPortId << "]" );      

          // cancel old timer and set a new updated timer, if this feature is activated
          //if(m_rstpActivateExpirationMaxAge == 1)
            //{
              m_portInfo[inPort].messAgeTimer.Remove();
              m_portInfo[inPort].messAgeTimer.SetArguments(inPort);
              m_portInfo[inPort].messAgeTimer.Schedule();//Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
              m_portInfo[inPort].lastMessAgeSchedule = Simulator::Now();
            //}
          
          //store the received bpdu in the port info database
          m_portInfo[inPort].designatedRootId   = m_portInfo[inPort].receivedBpdu.rootId;
          m_portInfo[inPort].designatedCost     = m_portInfo[inPort].receivedBpdu.cost;
          m_portInfo[inPort].designatedBridgeId = m_portInfo[inPort].receivedBpdu.bridgeId;
          m_portInfo[inPort].designatedPortId   = m_portInfo[inPort].receivedBpdu.portId;
          m_portInfo[inPort].messAge            = m_portInfo[inPort].receivedBpdu.messAge;

          //update port variables
          m_portInfo[inPort].synced     = false;
          m_portInfo[inPort].proposing  = false;
          m_portInfo[inPort].agreed     = false;
          if(bpdu.GetPropFlag()) m_portInfo[inPort].proposed  = true;
          else m_portInfo[inPort].proposed=false;
         
          // CHANGED FROM SPBDV      
          //store current port role
          std::string previous_role=m_portInfo[inPort].role;
           
          //reconfigure the bridge
          ReconfigureBridge();
          
          // check and process Tc flags
          CheckProcessTcFlags(bpdu.GetTcaFlag(), bpdu.GetTcFlag(), inPort);

          // CHANGED FROM SPBDV      
          //make sure we disseminate if the inPort was Root port before configuring, or it is root port after configuring
          if(previous_role==ROOT_ROLE || m_portInfo[inPort].role==ROOT_ROLE)
            {
              for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
                {
                  if(m_portInfo[i].role==DESIGNATED_ROLE) m_portInfo[i].newInfo=true;
                }
            }       
        }
      
      // CHANGED FROM SPBDV      
      //if the bpdu does not updates the port information (worse) and comes from a Designated
      if(bpduUpdates == -1 && bpdu.GetRoleFlags()==DESIGNATED_ROLE)  
        {
          //trace message
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Worse from Designated");
              
          // reply?
          if(bpdu.GetFrwFlag()==false) m_portInfo[inPort].newInfo = true;
        }

      //if the bpdu has the same information than the port database and comes from a Designated port
      if(bpduUpdates == 0 && bpdu.GetRoleFlags()==DESIGNATED_ROLE)  
        {       
          //traces
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Equal from Designated"); //     [R:" << bpdu.GetRootId() << "|C:" << bpdu.GetCost() << "|B:" << bpdu.GetBridgeId() << "|P:" << bpdu.GetPortId() << "] = [R:" << m_portInfo[inPort].designatedRootId << "|C:" << m_portInfo[inPort].designatedCost << "|B:" << m_portInfo[inPort].designatedBridgeId << "|P:" << m_portInfo[inPort].designatedPortId << "]" );      

          // CHANGED FROM SPBDV
          //update variables
          //if(bpdu.GetPropFlag()) m_portInfo[inPort].proposed=true;
          //else m_portInfo[inPort].proposed=false;
          
          //update message age field (this must be updated even if the BPDU us considered equal)
          m_portInfo[inPort].messAge = m_portInfo[inPort].receivedBpdu.messAge;
          
          // cancel old timer and set a new updated timer
          //if(m_rstpActivateExpirationMaxAge == 1)
            //{
              m_portInfo[inPort].messAgeTimer.Remove();
              m_portInfo[inPort].messAgeTimer.SetArguments(inPort);
              m_portInfo[inPort].messAgeTimer.Schedule();//Seconds(m_bridgeInfo.maxAge - m_portInfo[inPort].receivedBpdu.messAge));
              m_portInfo[inPort].lastMessAgeSchedule = Simulator::Now();
            //}
          
          // CHANGED FROM SPBDV
          // check for quick state transitions
          // QuickStateTransitions();
          
          // check and process Tc flags
          CheckProcessTcFlags(bpdu.GetTcaFlag(), bpdu.GetTcFlag(), inPort);
        }

      //if the bpdu is better and comes from a Root-Alternate (can this happen?)
      if( bpduUpdates == 1 && ( bpdu.GetRoleFlags()==ROOT_ROLE || bpdu.GetRoleFlags()==ALTERNATE_ROLE ) && bpdu.GetAgrFlag() )
        {
          //traces
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Better from Root-Alternate (invalid agreement)");
        }
                
      //if the equal the port information and comes from a Root-Alternate and contains an agreement flag set
      if( bpduUpdates == 0 && ( bpdu.GetRoleFlags()==ROOT_ROLE || bpdu.GetRoleFlags()==ALTERNATE_ROLE ) && bpdu.GetAgrFlag() )
        {
          //traces
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Equal from Root-Alternate with Agreement");
        }

      //if the bpdu is worse and comes from a Root-Alternate (but about the same root...)
      if( bpduUpdates == -1 && ( bpdu.GetRoleFlags()==ROOT_ROLE || bpdu.GetRoleFlags()==ALTERNATE_ROLE ) && bpdu.GetAgrFlag() )
        {
          if( bpdu.GetRootId()==m_bridgeInfo.rootId )
            {
              //traces
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Worse from Root-Alternate with Agreement");
              
              //port variables
              m_portInfo[inPort].proposing=false;
              m_portInfo[inPort].agreed=true;
              
              // check fopr quick state transitions
              QuickStateTransitions();

              // check and process Tc flags
              CheckProcessTcFlags(bpdu.GetTcaFlag(), bpdu.GetTcFlag(), inPort);
            }
          else
            {
              //traces
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << inPort << "]       Discarded Agreement");              
            }
        }
                     
      //disseminate - send BPDUs to those ports where newInfo is set
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          SendBPDU(m_ports[i], "disseminate BPDU");
        }
    }
    
  } // if bridge has been turned on
}


//////////////////////////////
// RSTP::CheckProcessTcFlags //
//////////////////////////////
void
RSTP::CheckProcessTcFlags(bool tcaFlag, bool tcFlag, uint16_t port)
{
  
  // if received a BPDU with TC flag
  if (tcFlag==true)
    {
      if(m_rstpActivateTopologyChange==1)
        {
          // if it comes from designated
          if(m_portInfo[port].role == DESIGNATED_ROLE)
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
      if(m_rstpActivateTopologyChange==1)
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
//RSTP::UpdatesInfo//
////////////////////
int16_t
RSTP::UpdatesInfo(bpduInfo_t bpdu, uint16_t port)
{
  NS_LOG_FUNCTION_NOARGS ();
  
/*
  - follows the 4-step comparison
    - best Root Id
    - if equal Root Id: best Cost
    - if equal Cost: best Bridge Id
    - if equal Bridge Id: best Port Id
    
    - (RSTP)it returns as better if it received an a root port and the sender is the same designasted port, even if it is worse
*/

  int16_t returnValue=2;
  
  NS_LOG_LOGIC("UpdatesInfo: BPDU [R:" << bpdu.rootId << "|C:" << bpdu.cost << "|B:" << bpdu.bridgeId << "|P:" << bpdu.portId << "] <=> Port[" << port << "]: [R:" << m_portInfo[port].designatedRootId << "|C:" << m_portInfo[port].designatedCost << "|B:" << m_portInfo[port].designatedBridgeId << "|P:" << m_portInfo[port].designatedPortId << "]");
  
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
              if(bpdu.rootId == m_portInfo[port].designatedRootId && bpdu.cost == m_portInfo[port].designatedCost && bpdu.bridgeId == m_portInfo[port].designatedBridgeId)// && bpdu.portId < m_portInfo[port].designatedPortId)
                {
                  // this last comparison is removed to avoid comparing the port and provide strict superior/inferior information
                  /*returnValue = 1;
                }
              else
                {
                  if(bpdu.rootId == m_portInfo[port].designatedRootId && bpdu.cost == m_portInfo[port].designatedCost && bpdu.bridgeId == m_portInfo[port].designatedBridgeId && bpdu.portId == m_portInfo[port].designatedPortId)
                    {*/
                      returnValue = 0;
                    //}
                }
            }
        }
    }
  
  if(returnValue==2)  //not changhed, so it is worse
    {
      returnValue = -1;
      // check if it was sent by the same designated, in this case we accept the message info (as better) - the last equality checks if it is not the own bridge (in the received agreements the designated bridge of the BPDU is the own bridge)
      if(bpdu.bridgeId == m_portInfo[port].designatedBridgeId && bpdu.portId == m_portInfo[port].designatedPortId && m_portInfo[port].designatedBridgeId!=m_bridgeInfo.bridgeId )
        {
          NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "]       Worse from same designated as before, so read as better...");
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
  
}

//////////////////////////
//RSTP::ReconfigureBridge//
//////////////////////////
void
RSTP::ReconfigureBridge()
{
  NS_LOG_FUNCTION_NOARGS ();
  
  //select new root port
  //////////////////////
  int32_t rootPortTmp=-1;
  uint64_t minRootId = 10000;
  uint32_t minCost = 10000;
  uint64_t minBridgeId = 10000;
  uint16_t minPortId = 10000;
  
  std::string previous_role;
 
  bool foundTmp=false;
  
  //select minimum root, from the port database (those ports where the designated is another bridge)
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

  m_bridgeInfo.rootId = minRootId;  
  NS_LOG_LOGIC("ReconfigureBridge: found root node " << m_bridgeInfo.rootId);
   
  if(minRootId == m_bridgeInfo.bridgeId)   //if i'm the root, there is no need to look for a root port
    {
      m_bridgeInfo.rootPort=-1;
      m_bridgeInfo.cost=0;
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
            }
        } //for (all ports)
            
      if(rootPortTmp == -1) //no minimum root has been found -> i'm the root bridge
        {  
          NS_LOG_LOGIC("ReconfigureBridge: I am the root (only Designateds)");   
          m_bridgeInfo.rootId   =m_bridgeInfo.bridgeId;
          m_bridgeInfo.rootPort =-1;
          m_bridgeInfo.cost     =0;
        }
      else
        {
          NS_LOG_LOGIC("ReconfigureBridge: I am not the root, so I set root port (" << rootPortTmp << ") info");             
          std::string previous_role_rootPort = m_portInfo[rootPortTmp].role;
          m_bridgeInfo.rootPort = rootPortTmp;
          m_portInfo[m_bridgeInfo.rootPort].role = ROOT_ROLE;
          m_bridgeInfo.cost = minCost + 1;
          if(previous_role_rootPort == ALTERNATE_ROLE) 
            {
              NS_LOG_LOGIC("ReconfigureBridge: The new root port(" << rootPortTmp << ") was alternate, so start toLearning timer");             
              MakeDiscarding(rootPortTmp);
              //CHANGED FROM SPBDV
              //m_portInfo[rootPortTmp].toLearningTimer.SetArguments((uint16_t)rootPortTmp);  
              //m_portInfo[rootPortTmp].toLearningTimer.Schedule();
            }
          if(m_portInfo[rootPortTmp].recentRootTimer.IsRunning()) // if the port is elected again as root, reset the recent root timer
            {
              m_portInfo[rootPortTmp].recentRootTimer.Remove();             
            }
        }    
      
    } //else (the root is someone else)

  NS_LOG_LOGIC("ReconfigureBridge: found root port " << m_bridgeInfo.rootPort);

  //from others, select designated port(s)
  ////////////////////////////////////////
  for(uint16_t i=0 ; i < m_portInfo.size() ; i++) //for all ports
    {
      if(m_portInfo[i].state!=DISABLED_STATE)
        {
          if(i != m_bridgeInfo.rootPort)  //except the root port (above selected)
            { 
              if(m_portInfo[i].designatedRootId != m_bridgeInfo.rootId || m_portInfo[i].designatedBridgeId==m_bridgeInfo.bridgeId ) //not completely updated information, or info from the same bridge
                { 
                  previous_role = m_portInfo[i].role;
                  m_portInfo[i].role=DESIGNATED_ROLE;
                }
              else
                {
                  if(m_portInfo[i].designatedCost > m_bridgeInfo.cost)
                    {
                      previous_role = m_portInfo[i].role;
                      m_portInfo[i].role=DESIGNATED_ROLE;
                    }
                  else
                    {
                      if(m_portInfo[i].designatedCost < m_bridgeInfo.cost)
                        {
                          previous_role = m_portInfo[i].role;
                          m_portInfo[i].role=ALTERNATE_ROLE;
                        }
                        
                      if(m_portInfo[i].designatedCost == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId > m_bridgeInfo.bridgeId)
                        {
                          previous_role = m_portInfo[i].role;
                          m_portInfo[i].role=DESIGNATED_ROLE;
                        }
                      else
                        {
                          if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId < m_bridgeInfo.bridgeId)
                            {
                              previous_role = m_portInfo[i].role;
                              m_portInfo[i].role=ALTERNATE_ROLE;
                            }
                          if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId == m_bridgeInfo.bridgeId && m_portInfo[i].designatedPortId > i)
                            {
                              previous_role = m_portInfo[i].role;
                              m_portInfo[i].role=DESIGNATED_ROLE;
                            }
                          else
                            {
                              if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId == m_bridgeInfo.bridgeId && m_portInfo[i].designatedPortId < i)
                                {
                                  previous_role = m_portInfo[i].role;
                                  m_portInfo[i].role=ALTERNATE_ROLE;
                                }
                              if(m_portInfo[i].designatedCost  == m_bridgeInfo.cost && m_portInfo[i].designatedBridgeId == m_bridgeInfo.bridgeId && m_portInfo[i].designatedPortId == i) // tot igual, soc jo el designated
                                { 
                                  previous_role = m_portInfo[i].role;
                                  m_portInfo[i].role=DESIGNATED_ROLE;        
                                }
                            }
                        }    
                    }   
                } //else (not completely updated information)
                      
              if(m_portInfo[i].role==DESIGNATED_ROLE) //if designated, edit port variables and save bridge information
                { 
                  NS_LOG_LOGIC("ReconfigureBridge: found designated port " << i);
                  
                  //for those Designateds 'newly' elected
                  if( m_portInfo[i].designatedRootId != m_bridgeInfo.rootId || m_portInfo[i].designatedCost != m_bridgeInfo.cost || m_portInfo[i].designatedBridgeId != m_bridgeInfo.rootId)// || m_portInfo[i].designatedPortId != i)
                    {
                      NS_LOG_LOGIC("ReconfigureBridge:      reset variables " << i);
                      m_portInfo[i].synced    = false;
                      m_portInfo[i].proposing = false;
                      m_portInfo[i].proposed  = false;
                      m_portInfo[i].agreed    = false;
                      //CHANGED FROM SPBDV
                      if(previous_role!=DESIGNATED_ROLE) m_portInfo[i].newInfo   = true;
                    }
                  
                  // save info from bridge to port
                  NS_LOG_LOGIC("ReconfigureBridge:      save info from bridge to port" << i);
                  m_portInfo[i].designatedPortId = i;
                  m_portInfo[i].designatedBridgeId = m_bridgeInfo.bridgeId;
                  m_portInfo[i].designatedRootId = m_bridgeInfo.rootId;
                  m_portInfo[i].designatedCost = m_bridgeInfo.cost;
                  m_portInfo[i].messAgeTimer.Remove(); // cancel messAge timer
                }
                        
            } //if (... except the root port)
            
            // apply state change, timers... in here for the port (designated or alternate) being looped now
            if(previous_role == ROOT_ROLE && m_portInfo[i].role != ROOT_ROLE)
              {
                NS_LOG_LOGIC("ReconfigureBridge: Old Root port " << i << " is not the the root port any more (start RecentRootTimer)");
                m_portInfo[i].recentRootTimer.SetArguments(i);
                m_portInfo[i].recentRootTimer.Schedule();
              }
            if(previous_role == DESIGNATED_ROLE && m_portInfo[i].role == DESIGNATED_ROLE)
              {
                NS_LOG_LOGIC("ReconfigureBridge: Designated port " << i << " keeps the role of Designated port");
              }
            if(previous_role == ALTERNATE_ROLE && m_portInfo[i].role != ALTERNATE_ROLE) 
              {
                NS_LOG_LOGIC("ReconfigureBridge: Alternate port elected as ROOT/DES " << i);
                MakeDiscarding(i);
                //CHANGED FROM SPBDV
                //m_portInfo[i].toLearningTimer.SetArguments(i);  
                //m_portInfo[i].toLearningTimer.Schedule();
              }
            if(m_portInfo[i].role==ALTERNATE_ROLE)
              {
                NS_LOG_LOGIC("ReconfigureBridge: found alternate port " << i);
                //m_portInfo[i].messAgeTimer.Remove(); // cancel messAge timer => why? it has info from the Designated in that LAN!!!
                m_portInfo[i].recentRootTimer.Remove(); // i guess because the port is synced, so it can not be problematic
                MakeDiscarding(i); //block this port
                m_portInfo[i].proposed=false;               
                m_portInfo[i].synced=true;    //synced because it is blocked
                m_portInfo[i].newInfo=true;   // to send up an agreement
                
                if( m_rstpActivateTopologyChange==1 )
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
  QuickStateTransitions();

  // printout the state of the bridge in a single line
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]       Brdg Info  [R:" << m_bridgeInfo.rootId << "|C:" << m_bridgeInfo.cost << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort << "]");

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      int16_t ageTmp;
      if(m_portInfo[p].role==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge);
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule; 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
      
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]       Port Info  [R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "|A:" << ageTmp << "|Tc:" << m_portInfo[p].topChange <<  "|Tca:" << m_portInfo[p].topChangeAck << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[" << (m_portInfo[p].proposed?"P'ed ":"---- ") << (m_portInfo[p].proposing?"P'ng ":"---- ") << (m_portInfo[p].sync?"Sync ":"---- ") << (m_portInfo[p].synced?"S'ed ":"---- ") << (m_portInfo[p].agreed?"A'ed ":"---- ") << (m_portInfo[p].newInfo?"New":"---") << "]");   
    }
    
  // print all nodes info  
  //m_simGod->PrintBridgesInfo();
}  
    

///////////////////////////////////////////
// RSTP::AllDesignatedsRecentRootExpired //
///////////////////////////////////////////
bool
RSTP::AllDesignatedsRecentRootExpired()
{
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if(m_portInfo[p].role==DESIGNATED_ROLE && m_portInfo[p].recentRootTimer.IsRunning() && m_portInfo[p].state!=DISABLED_STATE)
        {
          return false;
        }
    }
  return true;
}

//////////////////////////////////////////
// RSTP::SetSyncToAllRootAndDesignateds //
//////////////////////////////////////////
void
RSTP::SetSyncToAllRest(uint16_t port)
{
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if((m_portInfo[p].role==DESIGNATED_ROLE || p!=port) && m_portInfo[p].state!=DISABLED_STATE)
        {
          m_portInfo[p].sync=true;
        }
    }
}

///////////////////////////////////////
// RSTP::AllRootAndDesignatedsSynced //
///////////////////////////////////////
bool
RSTP::AllRestSynced(uint16_t port)
{
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if( p!= port && !m_portInfo[p].synced  && m_portInfo[p].state!=DISABLED_STATE )
        {
         return false;
        }
    }
  return true;
}


////////////////////////////////
// RSTP::QuickStateTransitions //
////////////////////////////////
void
RSTP::QuickStateTransitions()
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // printout the state of the bridge in a single line
  NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]       Brdg Info  [R:" << m_bridgeInfo.rootId << "|C:" << m_bridgeInfo.cost << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort << "]");

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      int16_t ageTmp;
      if(m_portInfo[p].role==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge);
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule; 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
        
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]       Port Info  [R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "|A:" << ageTmp << "|Tc:" << m_portInfo[p].topChange <<  "|Tca:" << m_portInfo[p].topChangeAck << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[" << (m_portInfo[p].proposed?"P'ed ":"---- ") << (m_portInfo[p].proposing?"P'ng ":"---- ") << (m_portInfo[p].sync?"Sync ":"---- ") << (m_portInfo[p].synced?"S'ed ":"---- ") << (m_portInfo[p].agreed?"A'ed ":"---- ") << (m_portInfo[p].newInfo?"New":"---") << "]");     
    }
  

  // Quick transition of the Root port
  NS_LOG_LOGIC("QuickStateTransitions: Quick transition of the Root port " << m_bridgeInfo.rootPort << "?");
  if(m_bridgeInfo.rootId != m_bridgeInfo.bridgeId) //if we are not root bridge (so there is a root port)
    {
       //if the root port is not forwarding
      if(m_portInfo[m_bridgeInfo.rootPort].state != FORWARDING_STATE)
        {
          NS_LOG_LOGIC("QuickStateTransitions:     root port not forwarding");
          
          //if all current designated ports have expired their recent root timer
          if( AllDesignatedsRecentRootExpired() )
            {
              NS_LOG_LOGIC("QuickStateTransitions:         all current designated ports have expired their recent root timer");
              MakeForwarding(m_bridgeInfo.rootPort);
              m_portInfo[m_bridgeInfo.rootPort].newInfo=true;
            }
          else
            {
              NS_LOG_LOGIC("QuickStateTransitions:         some recent root timer is still running (make it discarding, and make the new root port forwarding)");
              
              for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
                {
                  if(m_portInfo[p].role==DESIGNATED_ROLE && m_portInfo[p].recentRootTimer.IsRunning() && m_portInfo[p].state!=DISABLED_STATE)
                    {
                      MakeDiscarding(p);
                      //CHANGED FROM SPBDV
                      //m_portInfo[p].toLearningTimer.SetArguments(p);
                      //m_portInfo[p].toLearningTimer.Schedule();
                    }
                }

              MakeForwarding(m_bridgeInfo.rootPort);
              m_portInfo[m_bridgeInfo.rootPort].newInfo=true;              
                            
            }
            
        }
      else
        {
          NS_LOG_LOGIC("QuickStateTransitions:     Root port already FRW");

          //if the root port is proposed, agreement sent anyway because the other is proposing
          if( m_portInfo[m_bridgeInfo.rootPort].proposed )
            {
              NS_LOG_LOGIC("QuickStateTransitions:         the root port is proposed and forwarding. Send Agreement");
              //m_portInfo[m_bridgeInfo.rootPort].synced=true;
              m_portInfo[m_bridgeInfo.rootPort].newInfo=true;
            }
        }
              
      //if the root port is proposed and not synced
      if( m_portInfo[m_bridgeInfo.rootPort].proposed && !m_portInfo[m_bridgeInfo.rootPort].synced )
        {
          NS_LOG_LOGIC("QuickStateTransitions:         the root port is proposed and not synced. Set sync to the rest");
          m_portInfo[m_bridgeInfo.rootPort].proposed=false;
          m_portInfo[m_bridgeInfo.rootPort].sync=false;
          m_portInfo[m_bridgeInfo.rootPort].synced=true;
          m_portInfo[m_bridgeInfo.rootPort].newInfo=true;
          SetSyncToAllRest(m_bridgeInfo.rootPort);
        }
    }

  
  // Quick transition of the Designated ports
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if(m_portInfo[p].role==DESIGNATED_ROLE)
        {
          NS_LOG_LOGIC("QuickStateTransitions: Quick transition of the Designated port " << p << "?");
          
          //check the conditions under the port cannot go to FRW yet
          //cannot go to FRW because it has been told to sync || cannot go to FRW because it is still recent root
          if( (/*m_portInfo[p].state!=DISCARDING_STATE &&*/ m_portInfo[p].sync/* && !m_portInfo[p].synced*/) || ( (m_bridgeInfo.rootId!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort].state!=FORWARDING_STATE) && m_portInfo[p].recentRootTimer.IsRunning() ) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:     cannot go to FRW because it has been told to sync(" << (/*m_portInfo[p].state!=DISCARDING_STATE && */m_portInfo[p].sync/* && !m_portInfo[p].synced*/) << ") or it is still recent root(" <<  ( (m_bridgeInfo.rootId!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort].state!=FORWARDING_STATE) && m_portInfo[p].recentRootTimer.IsRunning() ) << ")");
              MakeDiscarding(p);
              //CHANGED FROM SPBDV
              //m_portInfo[p].toLearningTimer.SetArguments(p);
              //m_portInfo[p].toLearningTimer.Schedule();
            }
          
          //update sync and synced
          // told to sync, but already synced || not synced but discarding || was not synced but got an agreement
          if ( (m_portInfo[p].synced && m_portInfo[p].sync) || (!m_portInfo[p].synced && m_portInfo[p].state==DISCARDING_STATE) || (!m_portInfo[p].synced && m_portInfo[p].agreed) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:     told to sync, but already synced(" << (m_portInfo[p].synced && m_portInfo[p].sync) << ") || not synced but discarding(" << (!m_portInfo[p].synced && m_portInfo[p].state==DISCARDING_STATE) << ") || was not synced but got an agreement(" << (!m_portInfo[p].synced && m_portInfo[p].agreed) << ")");
              m_portInfo[p].synced=true;
              m_portInfo[p].sync=false;
              m_portInfo[p].recentRootTimer.Remove(); // i guess because the port is synced, so it can not be problematic
            }
          
          //start a handshake down the tree
          // not forwarding and not proposing and not agreed
          if(m_portInfo[p].state!=FORWARDING_STATE && !m_portInfo[p].proposing && !m_portInfo[p].agreed)
            {
              NS_LOG_LOGIC("QuickStateTransitions:     not forwarding and not proposing and not agreed (start a handshake down the tree)");
              m_portInfo[p].proposing=true;
              m_portInfo[p].newInfo=true;
            }
          
          //check whether the Designated can be finally set to FRW
          //no one said to sync and it is agreed from downstream root-alternate
          if( m_portInfo[p].state!=FORWARDING_STATE && !m_portInfo[p].sync && m_portInfo[p].agreed)
            {
              NS_LOG_LOGIC("QuickStateTransitions:     finally FRW? no one said to sync and it is agreed from downstream root-alternate");
              
              //not recent root (safe to FRW) || root bridge (no problem with the root port because it does not exist) || root port already forwarding (no problem with the root port because it is already forwarding)
              if( m_portInfo[p].recentRootTimer.IsExpired() || m_bridgeInfo.rootId==m_bridgeInfo.bridgeId || ( m_bridgeInfo.rootId!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort].state==FORWARDING_STATE ) )
                {
                  NS_LOG_LOGIC("QuickStateTransitions:         not recent root(" << (m_portInfo[p].recentRootTimer.IsExpired()) << ") or root bridge(" << (m_bridgeInfo.rootId==m_bridgeInfo.bridgeId) << ") or root port already forwarding(" << ( m_bridgeInfo.rootId!=m_bridgeInfo.bridgeId && m_portInfo[m_bridgeInfo.rootPort].state==FORWARDING_STATE ) << ")");
                  MakeForwarding(p);
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
      if(m_bridgeInfo.rootId != m_bridgeInfo.bridgeId)
        {
          if( (m_portInfo[m_bridgeInfo.rootPort].state != FORWARDING_STATE) && AllDesignatedsRecentRootExpired())
            {
              NS_LOG_LOGIC("QuickStateTransitions:     Root port not forwarding and AllDesignatedsRecentRootExpired");
              MakeForwarding(m_bridgeInfo.rootPort);
              m_portInfo[m_bridgeInfo.rootPort].newInfo=true;
            }

          // recheck the root port for possible changes in designated variables
          if(m_portInfo[m_bridgeInfo.rootPort].proposed && AllRestSynced(m_bridgeInfo.rootPort) )
            {
              NS_LOG_LOGIC("QuickStateTransitions:     root port proposed and all the rest are synced");
              m_portInfo[m_bridgeInfo.rootPort].proposed=false;
              m_portInfo[m_bridgeInfo.rootPort].sync=false;
              m_portInfo[m_bridgeInfo.rootPort].synced=true;
              m_portInfo[m_bridgeInfo.rootPort].newInfo=true;
            }         
        }
    }
    
  // check the alternates as well
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      if(m_portInfo[p].role==ALTERNATE_ROLE)
        {
          if(m_portInfo[p].proposed)
            {  
              m_portInfo[p].sync=false;
              m_portInfo[p].newInfo=true;
            }
          if(m_portInfo[p].sync==true && m_portInfo[p].synced==true)
            {
              m_portInfo[p].sync=false;
            }
        }
    }

  // printout the state of the bridge in a single line
  NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-]       Brdg Info  [R:" << m_bridgeInfo.rootId << "|C:" << m_bridgeInfo.cost << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort << "]");

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
    
      int16_t ageTmp;
      if(m_portInfo[p].role==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge);
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule; 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
        
      NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "]       Port Info  [R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "|A:" << ageTmp << "|Tc:" << m_portInfo[p].topChange <<  "|Tca:" << m_portInfo[p].topChangeAck << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[" << (m_portInfo[p].proposed?"P'ed ":"---- ") << (m_portInfo[p].proposing?"P'ng ":"---- ") << (m_portInfo[p].sync?"Sync ":"---- ") << (m_portInfo[p].synced?"S'ed ":"---- ") << (m_portInfo[p].agreed?"A'ed ":"---- ") << (m_portInfo[p].newInfo?"New":"---") << "]");     
    }

}



///////////////////
// RSTP::SendBPDU //
///////////////////
void
RSTP::SendBPDU (Ptr<NetDevice> outPort, std::string debugComment)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint16_t p = outPort->GetIfIndex();
  Time rooPortMessAge;
  Time rooPortMessAgeTimer;

  NS_LOG_LOGIC("SendBPDU: disseminate to port " << p << "? =>  Port State [R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[" << (m_portInfo[p].proposed?"P'ed ":"---- ") << (m_portInfo[p].proposing?"P'ng ":"---- ") << (m_portInfo[p].sync?"Sync ":"---- ") << (m_portInfo[p].synced?"S'ed ":"---- ") << (m_portInfo[p].agreed?"A'ed ":"---- ") << (m_portInfo[p].newInfo?"New":"---") << "]");  
     
  if(Simulator::Now() < Time(Seconds(m_simulationtime)) && m_portInfo[p].newInfo==true )
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
              bpduFields.SetCost(m_bridgeInfo.cost);//m_portInfo[p].designatedCost);
              bpduFields.SetBridgeId(m_bridgeInfo.bridgeId);//m_portInfo[p].designatedBridgeId);
              bpduFields.SetPortId(m_portInfo[p].designatedPortId);
              
              //set flags
              if(m_portInfo[p].proposing) bpduFields.SetPropFlag(true); else bpduFields.SetPropFlag(false);
              bpduFields.SetRoleFlags(m_portInfo[p].role);
              if(m_portInfo[p].state==DISCARDING_STATE) bpduFields.SetLrnFlag(false); else bpduFields.SetLrnFlag(true);
              if(m_portInfo[p].state==FORWARDING_STATE) bpduFields.SetFrwFlag(true); else bpduFields.SetFrwFlag(false);          
              if(m_portInfo[p].synced && (m_portInfo[p].role==ROOT_ROLE || m_portInfo[p].role==ALTERNATE_ROLE) ) bpduFields.SetAgrFlag(true); else bpduFields.SetAgrFlag(false);
              bpduFields.SetTcFlag(m_portInfo[p].topChange);          
              bpduFields.SetTcaFlag(m_portInfo[p].topChangeAck); 

              if(m_bridgeInfo.rootId == m_bridgeInfo.bridgeId) bpduFields.SetMessAge(0); // it is the root
              else
                {
                  /*rooPortMessAge = Seconds(m_portInfo[m_bridgeInfo.rootPort].messAge);
                  rooPortMessAgeTimer = Simulator::Now() - m_portInfo[m_bridgeInfo.rootPort].lastMessAgeSchedule;      
                  NS_LOG_LOGIC("rooPortMessAge.GetSeconds(): " << rooPortMessAge.GetSeconds());
                  NS_LOG_LOGIC("rooPortMessAgeTimer.GetSeconds(): " << rooPortMessAgeTimer.GetSeconds());
                  NS_LOG_LOGIC("rooPortMessAge + rooPortMessAgeTimer: " << rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds());
                  NS_LOG_LOGIC("Previous + inc: " << rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc);
                  NS_LOG_LOGIC("floor of previous: " << floor ( rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc ) );
                  NS_LOG_LOGIC("uint16_t of previous: " << (uint16_t) floor ( rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc ) );
                  if(m_rstpActivateIncreaseMessAge==1)
                    {
                      bpduFields.SetMessAge( (uint16_t) floor ( rooPortMessAge.GetSeconds()+rooPortMessAgeTimer.GetSeconds() + m_bridgeInfo.messAgeInc ) ); // it is not the root
                    }
                  else
                    {
                      bpduFields.SetMessAge(0);
                    }*/
                  
                  // RSTP sets the MessAge value exactly to the last value received in the root port
                  if(m_rstpActivateIncreaseMessAge==1)
                    {
                      bpduFields.SetMessAge( (uint16_t) ( m_portInfo[m_bridgeInfo.rootPort].messAge + m_bridgeInfo.messAgeInc ) ); // it is not the root
                    }
                  else
                    {
                      bpduFields.SetMessAge(0);
                    }
                  
                }
              
              //if(bpduFields.GetMessAge() < m_bridgeInfo.maxAge)
                //{
                  bpdu->AddHeader(bpduFields);
                  
                  //update txHold counter
                  m_portInfo[p].txHoldCount ++;

                  // debug message
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] RSTP: Sends BPDU [R:" << bpduFields.GetRootId() << "|C:" << bpduFields.GetCost() << "|B:" << bpduFields.GetBridgeId() << "|P:" << bpduFields.GetPortId() << "|A:" << bpduFields.GetMessAge() << "|Tc:" << bpduFields.GetTcFlag() << "|Tca:" << bpduFields.GetTcaFlag() << "|" << bpduFields.GetRoleFlags() <<(bpduFields.GetPropFlag()?"P":"-") << (bpduFields.GetAgrFlag()?"A":"-") << (bpduFields.GetLrnFlag()?"L":"-") << (bpduFields.GetFrwFlag()?"F":"-") << "] (TxHoldCount:"  << m_portInfo[p].txHoldCount << ") - (" << debugComment << ") - (link id: " << outPort->GetChannel()->GetId() << ")");        
                  
                  // send to the out port
                  outPort->SendFrom (bpdu, src, dst, protocol);
                //}
              /*else
                {
                  //mess age expired, discard before sending
                  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] RSTP: Discarded BPDU to Send due to MessAge >= MaxAge [R:" << bpduFields.GetRootId() << "|C:" << bpduFields.GetCost() << "|B:" << bpduFields.GetBridgeId() << "|P:" << bpduFields.GetPortId() << "|A:" << bpduFields.GetMessAge() << "|Tc:" << bpduFields.GetTcFlag() << "|Tca:" << bpduFields.GetTcaFlag() << "]"); 
                }
              */
              
              // reset because it is set just before calling SendBPDU
              m_portInfo[p].topChangeAck=false;
              
            }
          else
            {
              NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << outPort->GetIfIndex() << "] RSTP: Holds BPDU (TxCount: " << m_portInfo[p].txHoldCount << " => " <<  m_portInfo[p].txHoldCount+1 << ")");
              m_portInfo[p].txHoldCount ++; // even if we have not sent, keep counting to differentiate 1 sent from 1 sent and 1, or more, hold
            } 
          m_portInfo[p].newInfo=false;
        }
    }  
}

////////////////////////////
// RSTP::SendHelloTimeBpdu //
////////////////////////////
void
RSTP::SendHelloTimeBpdu()
{
  if( /*m_bridgeInfo.rootId == m_bridgeInfo.bridgeId &&*/ Simulator::Now() < Time(Seconds(m_simulationtime))) // everyone sends periodical BPDUs
    {
    
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[-] RSTP: Periodical BPDUs");
      
      //Mac48Address HwDestination = Mac48Address("00:11:22:33:44:55"); //this must be the 'to all bridges' mac address
     
      for(uint16_t i=0 ; i < m_ports.size() ; i++)
        {
          if(m_portInfo[i].role==DESIGNATED_ROLE){
            m_portInfo[i].newInfo=true;
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
            NS_LOG_ERROR("RSTP: Trying to schedule a helloTimeTimer while there is another scheduled");
            exit(1);
          }
        
      }
    }

  // just check it here as well, for safety
  m_simGod->ReviewTree(0); 
}

///////////////////////////
// RSTP::ResetTxHoldCount //
///////////////////////////
void RSTP::ResetTxHoldCount()
{
  if (Simulator::Now() < Time(Seconds(m_simulationtime)))
    {  
      uint16_t lastTxHoldCount;
           
      for(uint16_t p=0 ; p < m_ports.size() ; p++)
        { 
          if(m_portInfo[p].state!=DISABLED_STATE)
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
                           m_portInfo[p].txHoldCount = m_bridgeInfo.maxBpduTxHoldPeriod - 1 ; // ... otherwise decrement one (case where much more than permitted have benn sent)
                         }
                        else
                         {
                           m_portInfo[p].txHoldCount = m_portInfo[p].txHoldCount - 1 ; // ... otherwise decrement one (case where much less than permitted have benn sent)
                         }
                     }

                }
              NS_LOG_LOGIC (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << p << "] RSTP: Clr TxHold (TxCount: " << lastTxHoldCount << "=>0)");
              
              if( /*m_portInfo[p].role==DESIGNATED_ROLE &&*/ lastTxHoldCount > m_bridgeInfo.maxBpduTxHoldPeriod) //more than the maximum number of BPDUs sent, so at least one is pending
                {
                  m_portInfo[p].newInfo=true;
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
          NS_LOG_ERROR("RSTP: Trying to schedule a txHoldCount while there is another scheduled");
          exit(1);
        }
    }
}

//////////////////////////
// RSTP::MakeDiscarding //
//////////////////////////
void RSTP::MakeDiscarding (uint16_t port)
{
  m_portInfo[port].state=DISCARDING_STATE;
  m_portInfo[port].toLearningTimer.Remove();
  m_portInfo[port].toForwardingTimer.Remove();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] RSTP: DSC State");
  
  //notify simGod
  m_simGod->SetActivePort(0, m_node->GetId(), port, false); // singel tree, so tree 0

  // just check it here as well, for safety
  //m_simGod->ReviewTree(0); 
    
  //notify bridge modele of the blocked port
  m_bridge->SetPortState(-1, port, false); // set to blocking  - tree is -1 because single tree 
}

///////////////////////
// RSTP::MakeLearning //
///////////////////////
void RSTP::MakeLearning (uint16_t port)
{
  m_portInfo[port].state=LEARNING_STATE;    
  m_portInfo[port].toLearningTimer.Remove();
  m_portInfo[port].toForwardingTimer.Remove();
  m_portInfo[port].toForwardingTimer.SetArguments(port);  
  m_portInfo[port].toForwardingTimer.Schedule();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] RSTP: LRN State");
}

/////////////////////////
// RSTP::MakeForwarding //
/////////////////////////
void RSTP::MakeForwarding (uint16_t port)
{
  m_portInfo[port].state=FORWARDING_STATE;
  m_portInfo[port].toLearningTimer.Remove();
  m_portInfo[port].toForwardingTimer.Remove();
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] RSTP: FRW State");

  //notify simGod 
  m_simGod->SetActivePort(0, m_node->GetId(), port, true); // singel tree, so tree 0

  // just check it here as well, for safety
  //m_simGod->ReviewTree(0); 
  
  //notify bridge modele of the blocked port
  m_bridge->SetPortState(-1, port, true); // set to forwarding  - tree is -1 because single tree

//CHANGED FROM SPBDV
/*    
  // since there is a topology change, mark all ports to send BPDUs
  for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
    {
      m_portInfo[i].newInfo=true;
      //SendBPDU(m_ports[i], "disseminate BPDU");
    }
*/
    
  // Topology Change Procedure
  if( m_rstpActivateTopologyChange==1 )
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
}

////////////////////////////
// RSTP::MessageAgeExpired //
////////////////////////////
void RSTP::MessageAgeExpired(uint16_t port)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)) && m_rstpActivateExpirationMaxAge==1 )
    {
      //cancel timer
      m_portInfo[port].messAgeTimer.Remove();
      
      //clear the port info of the expired port with the 'own' info (assuming it will be the root)
        //m_portInfo[port].role  = DESIGNATED_ROLE;
        //m_portInfo[port].state = DISABLED_STATE;
      m_portInfo[port].designatedRootId = m_bridgeInfo.rootId;
      m_portInfo[port].designatedCost = m_bridgeInfo.cost;
      m_portInfo[port].designatedBridgeId = m_bridgeInfo.bridgeId;
      m_portInfo[port].designatedPortId = m_ports[port]->GetIfIndex();
      //m_portInfo[port].messAge = 0;       ??????????

      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] RSTP: MessAgeExp [R:" << m_portInfo[port].designatedRootId << "|C:" << m_portInfo[port].designatedCost << "|B:" << m_portInfo[port].designatedBridgeId << "|P:" << m_portInfo[port].designatedPortId << "|A:" << m_portInfo[port].messAge << "]");      

      //reconfigure the bridge to select new port roles
      ReconfigureBridge();
    
      //send BPDU to designated ports
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          if(m_portInfo[i].role == DESIGNATED_ROLE)
            {
              m_portInfo[i].newInfo=true;
              SendBPDU(m_ports[i], "maxAge expiration");
            }
        }
    } 
}

//////////////////////////////////
// RSTP::RecentRootTimerExpired //
//////////////////////////////////
void RSTP::RecentRootTimerExpired(uint16_t port)
{
  NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] WARNING: RecentRootTimer Expires => Something to do?");
}

void
RSTP::DoDispose (void)
{
  // bridge timers
  m_bridgeInfo.helloTimeTimer.Cancel();
  m_bridgeInfo.txHoldCountTimer.Cancel();
    
  // port timers
  for(uint16_t p=0 ; p < m_ports.size() ; p++)
    {
      m_portInfo[p].toLearningTimer.Cancel();
      m_portInfo[p].toForwardingTimer.Cancel();
      m_portInfo[p].messAgeTimer.Cancel();
      m_portInfo[p].recentRootTimer.Cancel();
    }
}   

//////////////////////////////////
// RSTP::PhyLinkFailureDetected //
//////////////////////////////////
void RSTP::PhyLinkFailureDetected(uint16_t port)
{
  if(Simulator::Now() < Time(Seconds(m_simulationtime)))
    {
      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] SPBDV: Physical Link Failure Detected");  

      //cancel timers
      m_portInfo[port].phyLinkFailDetectTimer.Remove();
      m_portInfo[port].messAgeTimer.Remove();
            
      //clear the port info of the expired port with the 'own' info (assuming it will be the root)
      m_portInfo[port].state=DISABLED_STATE;
      m_portInfo[port].role=DESIGNATED_ROLE;
      m_portInfo[port].agreed=false;
      m_simGod->SetActivePort(0, m_node->GetId(), port, false); // singel tree, so tree 0

      m_portInfo[port].designatedRootId = 999; //m_bridgeInfo.rootId;
      m_portInfo[port].designatedCost = 999; //m_bridgeInfo.cost;
      m_portInfo[port].designatedBridgeId = 999; //m_bridgeInfo.bridgeId;
      m_portInfo[port].designatedPortId = 999; //m_ports[port]->GetIfIndex();
      //m_portInfo[port].messAge = 0;       ??????????

      //trace message
      NS_LOG_INFO (Simulator::Now() << " Brdg[" << m_bridgeInfo.bridgeId << "]:Port[" << port << "] RSTP: Physical Link Failure Detected [R:" << m_portInfo[port].designatedRootId << "|C:" << m_portInfo[port].designatedCost << "|B:" << m_portInfo[port].designatedBridgeId << "|P:" << m_portInfo[port].designatedPortId << "|A:" << m_portInfo[port].messAge << "]");      

      //reconfigure the bridge to select new port roles
      ReconfigureBridge();
      QuickStateTransitions();
        
      //CHANGED FOR DEBUGGING
      //send BPDU to designated ports
      for(uint16_t i=0 ; i < m_portInfo.size() ; i++)
        {
          if(m_portInfo[i].role == DESIGNATED_ROLE)
            {
              m_portInfo[i].newInfo=true;
              SendBPDU(m_ports[i], "phy link failure detected");
            }
        }
    } 
}

////////////////////////////////
// RSTP::TopChangeTimerExpired //
////////////////////////////////
void RSTP::TopChangeTimerExpired(uint16_t port)
{
  if(m_rstpActivateTopologyChange==1)
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
// RSTP::ScheduleLinkFailure //
//////////////////////////////
void
RSTP::ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port)
{  
  NS_LOG_INFO ("Node[" << m_node->GetId() << "] RSTP: Setting Link Failure Physical Detection in port " << port << " at time " << linkFailureTimeUs);

  //configure messAge timer
  m_portInfo[port].phyLinkFailDetectTimer.Remove();
  m_portInfo[port].phyLinkFailDetectTimer.SetArguments(port);  
  m_portInfo[port].phyLinkFailDetectTimer.Schedule(linkFailureTimeUs); 
}


////////////////////////////////////
// RSTP::PrintBridgeInfoSingleLine //
////////////////////////////////////
void
RSTP::PrintBridgeInfoSingleLine()
{

  std::stringstream stringOut;

  // printout the state of the bridge in a single line
  stringOut << "B" << m_bridgeInfo.bridgeId << " [R:" << m_bridgeInfo.rootId << "|C:" << m_bridgeInfo.cost << "|B:" << m_bridgeInfo.bridgeId << "|P:" << m_bridgeInfo.rootPort << "]\n";

  // printout the state of each port in a single line
  for(uint16_t p=0 ; p < m_portInfo.size() ; p++)
    {
      int16_t ageTmp;
      if(m_portInfo[p].role==DESIGNATED_ROLE)
        {
          ageTmp=-1;
        }
      else
        {
          Time portMessAge = Seconds(m_portInfo[p].messAge);
          Time portMessAgeTimer = Simulator::Now() - m_portInfo[p].lastMessAgeSchedule; 
          ageTmp = (int16_t) floor ( portMessAge.GetSeconds()+portMessAgeTimer.GetSeconds() ); // value of mess age
        }
        
      stringOut << "    P" << p << "[R:" << m_portInfo[p].designatedRootId << "|C:" << m_portInfo[p].designatedCost << "|B:" << m_portInfo[p].designatedBridgeId << "|P:" << m_portInfo[p].designatedPortId << "|A:" << ageTmp << "]:[" << m_portInfo[p].role << "-" << m_portInfo[p].state << "]:[" << (m_portInfo[p].proposed?"P'ed ":"---- ") << (m_portInfo[p].proposing?"P'ng ":"---- ") << (m_portInfo[p].sync?"Sync ":"---- ") << (m_portInfo[p].synced?"S'ed ":"---- ") << (m_portInfo[p].agreed?"A'ed ":"---- ") << (m_portInfo[p].newInfo?"New":"---") << "]\n";     
    }
    
  NS_LOG_INFO(stringOut.str());
}


uint64_t
RSTP::GetBridgeId(){
  return m_bridgeInfo.bridgeId;
}

} //namespace ns3
