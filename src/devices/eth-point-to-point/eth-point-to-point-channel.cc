/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Eduard Bonada <eduard.bonada@upf.edu>
 */

#include "eth-point-to-point-channel.h"
#include "eth-point-to-point-net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE ("EthPointToPointChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (EthPointToPointChannel);

TypeId 
EthPointToPointChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EthPointToPointChannel")
    .SetParent<Channel> ()
    .AddConstructor<EthPointToPointChannel> ()
    .AddAttribute ("Delay", "Transmission delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&EthPointToPointChannel::m_delay),
                   MakeTimeChecker ())
    //.AddAttribute ("activateFailure", "Activate the property to fail (the failure time is identified in failureTime)",
    //               UintegerValue (0),
    //               MakeUintegerAccessor (&EthPointToPointChannel::m_activateFailure),
    //               MakeUintegerChecker<uint8_t> ())
    //.AddAttribute ("failureTimeUs", "Time in us when the link failure happens",
    //               TimeValue (MicroSeconds (0)),
    //               MakeTimeAccessor (&EthPointToPointChannel::m_failureTimeUs),
    //               MakeTimeChecker ())
    .AddTraceSource ("TxRxPointToPoint",
                     "Trace source indicating transmission of packet from the EthPointToPointChannel, used by the Animation interface.",
                     MakeTraceSourceAccessor (&EthPointToPointChannel::m_txrxPointToPoint))
    ;
  return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
EthPointToPointChannel::EthPointToPointChannel()
: 
  Channel (), 
  m_delay (Seconds (0.)),
  m_nDevices (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_failedLink=false;
}

void
EthPointToPointChannel::SetSimulationGod(Ptr<SimulationGod> simGod)
{
  m_simGod = simGod;
}

void
EthPointToPointChannel::Attach(Ptr<EthPointToPointNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG(m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT(device != 0);

  m_link[m_nDevices++].m_src = device;
//
// If we have both devices connected to the channel, then finish introducing
// the two halves and set the links to IDLE.
//
  if (m_nDevices == N_DEVICES)
    {
      m_link[0].m_dst = m_link[1].m_src;
      m_link[1].m_dst = m_link[0].m_src;
      m_link[0].m_state = IDLE;
      m_link[1].m_state = IDLE;
    }
}

bool
EthPointToPointChannel::TransmitStart(
  Ptr<Packet> p,
  Ptr<EthPointToPointNetDevice> src,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  
  if(m_failedLink==true)
    {
      // do nothing because it is failed
      
      NS_LOG_INFO(Simulator::Now() << " EthPointToPointChannel(): Frame Lost Due to Failed Link");
      
      return false;
    }
  else
    {  

      NS_ASSERT(m_link[0].m_state != INITIALIZING);
      NS_ASSERT(m_link[1].m_state != INITIALIZING);

      uint32_t wire = src == m_link[0].m_src ? 0 : 1;

      Simulator::Schedule (txTime + m_delay, &EthPointToPointNetDevice::Receive,
        m_link[wire].m_dst, p);

      // Call the tx anim callback on the net device
      m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
      return true;
    }
}

void
EthPointToPointChannel::ExecuteLinkFailure()
{
  //change flag of failed link
  NS_LOG_INFO(Simulator::Now() << " EthPointToPointChannel(): Failed Link between nodes " << m_link[0].m_src->GetNode()->GetId() << "-" << m_link[0].m_dst->GetNode()->GetId());
  m_failedLink=true;
  m_simGod->SetFailedLink(m_link[0].m_src->GetNode()->GetId(),m_link[0].m_dst->GetNode()->GetId());
}

void
EthPointToPointChannel::ScheduleLinkFailure(Time linkFailureTimeUs)
{
  NS_LOG_INFO(Simulator::Now() << " EthPointToPointChannel(): Setting Link Failure event at " << linkFailureTimeUs);
  //configure and schedule  m_executeLinkFailureTimer
  m_executeLinkFailureTimer.SetFunction(&EthPointToPointChannel::ExecuteLinkFailure,this);
  m_executeLinkFailureTimer.SetDelay(linkFailureTimeUs);
  m_executeLinkFailureTimer.Schedule();
}

uint32_t 
EthPointToPointChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<EthPointToPointNetDevice>
EthPointToPointChannel::GetPointToPointDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT(i < 2);
  NS_LOG_LOGIC("i: " << i);
  NS_LOG_LOGIC("m_link[i].m_src->GetIfIndex(): " << m_link[i].m_src->GetIfIndex());
  return m_link[i].m_src;
}

Ptr<NetDevice>
EthPointToPointChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointDevice (i);
}

void
EthPointToPointChannel::AlternativeDoDispose (void)
{
  m_executeLinkFailureTimer.Cancel();
}  


} // namespace ns3
