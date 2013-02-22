/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Universitat Pompeu Fabra
 *
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
 * 
 * Implementation of the TrafficGenerator class
 * Folder: /src/netcom-platform/traffic-generator
 * Files: traffic-generator.c & traffic-generator.h
 * Functionality: creates packets depending on a distribution (now is CBR with fixed rate/packet Length)
 */



#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/bridge-traff-gen.h"
#include "ns3/data-rate.h"
#include "ns3/source-tag.h"
#include "ns3/seq-tag.h"
//#include "ns3/timestamp-tag.h"

NS_LOG_COMPONENT_DEFINE ("BridgeTraffGen");

using namespace std;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BridgeTraffGen);

TypeId
BridgeTraffGen::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BridgeTraffGen")
    .SetParent<Application> ()
    ;
  return tid;
}

/*
  - Constructor of the class
  - Parameters:
    - n: pointer to the node where the traffic generator is attached
    - bridge: pointer to the bridgeNetDevice where the traffic will be directed to.
*/
BridgeTraffGen::BridgeTraffGen(Ptr<Node> n, Ptr<BridgeNetDevice> bridge)
{
  Construct (n, bridge);
}

BridgeTraffGen::~BridgeTraffGen()
{}

/*
  - Really set ups the object instance
  - the idea is to have different Constructors that call the same Construct()
*/
void
BridgeTraffGen::Construct (Ptr<Node> n, Ptr<BridgeNetDevice> bridge)
{ 
  NS_LOG_LOGIC("Debbuging Traffic Generator: Construct");
  
  m_node=n;
  m_bridge=bridge;
  m_destMacAddress = Mac48Address("ff:ff:ff:ff:ff:ff");
  m_typeLength = 8888;  
}

void
BridgeTraffGen::DoDispose (void)
{
  Application::DoDispose ();
}

void BridgeTraffGen::SetPktLength(int distr, double mean, double variance, double lower, double upper)
{
  NS_LOG_LOGIC("Debbuging Traffic Generator: SetPktLength");
  
  m_pktLengthDistr    = distr;
  m_meanPktLength     = mean;
  m_variancePktLength = variance;
  m_lowerPktLength    = lower;
  m_upperPktLength    = upper;
}

void BridgeTraffGen::SetInterPktTime(int distr, double mean, double variance, double lower, double upper)
{
  NS_LOG_LOGIC("Debbuging Traffic Generator: SetInterPktTime");
  
  m_interPktTimeDistr    = distr;
  m_meanInterPktTime     = mean;
  m_varianceInterPktTime = variance;
  m_lowerInterPktTime    = lower;
  m_upperInterPktTime    = upper;
}

void BridgeTraffGen::SetDestMacAddress(Mac48Address dest){
  m_destMacAddress=dest;
}
void BridgeTraffGen::SetTypeLength(uint16_t type){
  m_typeLength=type;
}

void BridgeTraffGen::SetNumPktToGenerate(uint64_t n){
  m_numPktsToGenerate = n;
}

/*
  - automatically called at the time stated by "TG->start(time_to_start)" from the main script (when you setup the instance in the main menu)
  - calls method StartSending()
*/
void BridgeTraffGen::StartApplication()
{
  NS_LOG_LOGIC("Debbuging Traffic Generator: StartApplication (calls StartSending)");

  StartSending();
}

/*
  - automatically called at the time stated by "TG->stop(time_to_start)" from the main script (when you setup the instance in the main menu)
  - cancels the m_sendEvent (cancelling this event cancels the creation of packets - note the simulation will be stopped when no more packets are sent/processed)
*/
void BridgeTraffGen::StopApplication()
{
  NS_LOG_LOGIC("Debbuging Traffic Generator: StopApplication (cancels m_sendEvent)");
 
  Simulator::Cancel(m_sendEvent);
}

/*
  - inherited from application / schedules a first interarrival time before sending the first packet
*/
void BridgeTraffGen::StartSending()
{
  NS_LOG_LOGIC("Debbuging Traffic Generator: StartSending (calls GeneratePacket)");
  
  //Simulator::Schedule(Simulator::Now(),&BridgeTraffGen::GeneratePacket, this);
  GeneratePacket();
}

/*
  - computes the interarrivel time (when new packet must be created) and schedules a call to function SendPacket()
*/
void BridgeTraffGen::ScheduleNext()
{  
  // schedule the event pointing to GeneratePacket

  Time interPacketTime = ComputeInterPacketTime(m_interPktTimeDistr);
  
  NS_LOG_LOGIC("Debbuging Traffic Generator: ScheduleNext (schedules for " << interPacketTime << " a call to GeneratePacket)");
  
  m_sendEvent=Simulator::Schedule(interPacketTime,&BridgeTraffGen::GeneratePacket, this);
}

/*
  - really creates a packet and sends it to the port (m_port -> Send(...))
  - also calls ScheduleNext() to schedule next packet to be created
*/
void BridgeTraffGen::GeneratePacket()
{
  //compute packet length (bytes)
  
  // create packet fields
  m_numGeneratedPkts++;

  uint32_t pktLengthTemp = ComputePacketLength(m_pktLengthDistr);
  Ptr<Packet> packet = Create<Packet> (pktLengthTemp);

  SourceTag SidTag;
  SidTag.Set (m_node->GetId());
  packet->AddPacketTag (SidTag);  

  SeqTag seq;
  seq.Set (m_numGeneratedPkts);
  packet->AddPacketTag (seq);  

/*  TimestampTag timestamp;
  timestamp.Set ((Simulator::Now()).GetMicroSeconds());
  packet->AddPacketTag (timestamp);  
*/    
  NS_LOG_INFO(Simulator::Now() << " Traffic Gen [" << m_node->GetId() << "]      Generates packet (Length:" << pktLengthTemp << "|dst:" << m_destMacAddress << "|Type:" << m_typeLength << "|source:" << SidTag.Get() << "|seq:" << seq.Get() << ")");//"|ts:" << timestamp.Get() << ")");

  m_bridge->Send(packet, m_destMacAddress, m_typeLength);
	
  //if(m_numGeneratedPkts < m_numPktsToGenerate){
    //NS_LOG_LOGIC("Debbuging Traffic Generator: GeneratePacket (has already generated, and now calls ScheduleNext)");
    ScheduleNext();
  //}
}


uint32_t BridgeTraffGen::ComputePacketLength(int distribution){
  // returns the Length of the packet to be generated
  // the parameter 'distrib' indicates the distribution to use (constant, exponential, uniform...)
   
  switch (distribution){
    
    case DIST_CONSTANT:{
      ConstantVariable value = ConstantVariable(m_meanPktLength); //param is the constant value
      return (uint32_t) value.GetValue();
      }
      break;

    case DIST_UNIFORM:{
      UniformVariable value = UniformVariable(m_lowerPktLength,m_upperPktLength); //params are lower and upper bound
      return (uint32_t) value.GetValue();
      }
      break;

    case DIST_EXPONENTIAL:{
      ExponentialVariable value = ExponentialVariable(m_meanPktLength); //param is the mean
      return (uint32_t) value.GetValue();
      }
      break;

    /*case DIST_PARETO:{
      ParetoVariable value = ParetoVariable(param1,param2,param3); //params are ???
      return value.GetInteger();
      }
      break;*/

    case DIST_NORMAL:{
      NormalVariable value = NormalVariable(m_meanPktLength,m_variancePktLength); //params are the mean and the variance
      return (uint32_t) value.GetValue();
      }
      break;

    /*case DIST_LOGNORMAL:{
      LogNormalVariable value = LogNormalVariable(param1,param2); //params are mu and sigma
      return value.GetInteger();
      }
      break;*/
    
    default:
      NS_LOG_INFO("ERROR: BridgeTraffGen has detected a not existing distribution (" << distribution << ").");
      exit(1);
      break; 
  }
  
  NS_LOG_INFO("ERROR: BridgeTraffGen has detected a not existing distribution (" << distribution << ").");
  exit(1);  
  return 0;
}

Time BridgeTraffGen::ComputeInterPacketTime(int distribution){
  // returns the time between packet generations (the interarrival, not the absolute time)
  // the parameter 'distrib' indicates the distribution to use (constant, exponential, uniform...)

  switch (distribution){
    
    case DIST_CONSTANT:{
      ConstantVariable value = ConstantVariable(m_meanInterPktTime); //param is the constant value
      return Seconds( value.GetValue() );
      }
      break;

    case DIST_UNIFORM:{
      UniformVariable value = UniformVariable(m_lowerInterPktTime,m_upperInterPktTime); //params are lower and upper bound
      return Seconds( value.GetValue() );
      }
      break;

    case DIST_EXPONENTIAL:{
      ExponentialVariable value = ExponentialVariable(m_meanInterPktTime); //param is the mean
      return Seconds( value.GetValue() );
      }
      break;

    /*case DIST_PARETO:{
      ParetoVariable value = ParetoVariable(param1,param2,param3); //params are ???
      return Seconds( value.GetValue() );
      }
      break;*/

    case DIST_NORMAL:{
      NormalVariable value = NormalVariable(m_meanInterPktTime,m_varianceInterPktTime); //params are the mean and the variance
      return Seconds( value.GetValue() );
      }
      break;

    /*case DIST_LOGNORMAL:{
      LogNormalVariable value = LogNormalVariable(param1,param2); //params are mu and sigma
      return Seconds( value.GetValue() );
      }
      break;*/
    
    default:
      NS_LOG_INFO("ERROR: BridgeTraffGen has detected a not existing distribution (" << distribution << ").");
      exit(1);
      break; 
  }
  
  NS_LOG_INFO("ERROR: BridgeTraffGen has detected a not existing distribution (" << distribution << ").");
  exit(1);  
  return Seconds(0.0);
}

} // Namespace ns3
