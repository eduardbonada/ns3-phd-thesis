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
 * Author: Eduard Bonada <eduard.bonada@upf.edu>
 */

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/ethernet-header.h"
#include "ns3/ethernet-trailer.h"
#include "eth-point-to-point-net-device.h"
#include "eth-point-to-point-channel.h"


NS_LOG_COMPONENT_DEFINE ("EthPointToPointNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (EthPointToPointNetDevice);

TypeId 
EthPointToPointNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EthPointToPointNetDevice")
    .SetParent<NetDevice> ()
    .AddConstructor<EthPointToPointNetDevice> ()
    .AddAttribute ("Address", 
                   "The MAC address of this device.",
                   Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                   MakeMac48AddressAccessor (&EthPointToPointNetDevice::m_address),
                   MakeMac48AddressChecker ())
    .AddAttribute ("FrameSize", 
                   "The maximum size of a packet sent over this device.",
                   UintegerValue (DEFAULT_FRAME_SIZE),
                   MakeUintegerAccessor (&EthPointToPointNetDevice::SetFrameSize,
                                         &EthPointToPointNetDevice::GetFrameSize),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("DataRate", 
                   "The default data rate for point to point links",
                   DataRateValue (DataRate ("32768b/s")),
                   MakeDataRateAccessor (&EthPointToPointNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddAttribute ("ReceiveErrorModel", 
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&EthPointToPointNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("InterframeGap", 
                   "The time to wait between packet (frame) transmissions",
                   TimeValue (Seconds (0.0)),
                   MakeTimeAccessor (&EthPointToPointNetDevice::m_tInterframeGap),
                   MakeTimeChecker ())
    .AddAttribute ("TxQueue", 
                   "A queue to use as the transmit queue in the device.",
                   PointerValue (),
                   MakePointerAccessor (&EthPointToPointNetDevice::m_queue),
                   MakePointerChecker<Queue> ())

    // Trace sources at the "top" of the net device, where packets transition to/from higher layers.
    .AddTraceSource ("MacTx", 
                     "Trace source indicating a packet has arrived for transmission by this device",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_macTxTrace))
    .AddTraceSource ("MacTxDrop", 
                     "Trace source indicating a packet has been dropped by the device before transmission",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_macTxDropTrace))
    .AddTraceSource ("MacPromiscRx", 
                     "A packet has been received by this device, has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_macPromiscRxTrace))
    .AddTraceSource ("MacRx", 
                     "A packet has been received by this device, has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_macRxTrace))
    .AddTraceSource ("PhyTxBegin", 
                     "Trace source indicating a packet has begun transmitting over the channel",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_phyTxBeginTrace))
    .AddTraceSource ("PhyTxEnd", 
                     "Trace source indicating a packet has been completely transmitted over the channel",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_phyTxEndTrace))
    .AddTraceSource ("PhyTxDrop", 
                     "Trace source indicating a packet has been dropped by the device during transmission",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_phyTxDropTrace))
    .AddTraceSource ("PhyRxEnd", 
                     "Trace source indicating a packet has been completely received by the device",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_phyRxEndTrace))
    .AddTraceSource ("PhyRxDrop", 
                     "Trace source indicating a packet has been dropped by the device during reception (due to error rx model)",
                     MakeTraceSourceAccessor (&EthPointToPointNetDevice::m_phyRxDropTrace))
    ;
  return tid;
}

EthPointToPointNetDevice::EthPointToPointNetDevice () 
: 
  m_txMachineState (READY),
  m_channel (0), 
  m_linkUp (false),
  m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);

  m_encapMode = DIX;
  m_frameSize = DEFAULT_FRAME_SIZE;
  m_mtu = MtuFromFrameSize (m_frameSize);
}

EthPointToPointNetDevice::~EthPointToPointNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

  void 
EthPointToPointNetDevice::DoDispose()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  NetDevice::DoDispose ();
}

  void 
EthPointToPointNetDevice::AddHeader(Ptr<Packet> p,   Mac48Address source,  Mac48Address dest,  uint16_t protocolNumber)
{  
  NS_LOG_FUNCTION (p << source << dest << protocolNumber);

  EthernetHeader header (false);
  header.SetSource (source);
  header.SetDestination (dest);

  EthernetTrailer trailer;

  /*
  NS_LOG_LOGIC ("p->GetSize () = " << p->GetSize ());
  NS_LOG_LOGIC ("m_encapMode = " << m_encapMode);
  NS_LOG_LOGIC ("m_mtu = " << m_mtu);
  NS_LOG_LOGIC ("m_frameSize = " << m_frameSize);
  */

  uint16_t lengthType = 0;
  switch (m_encapMode) 
    {
    case DIX:
      //NS_LOG_LOGIC ("Encapsulating packet as DIX (type interpretation)");
      lengthType = protocolNumber;
      break;
    case LLC: 
      {
        //NS_LOG_LOGIC ("Encapsulating packet as LLC (length interpretation)");

        LlcSnapHeader llc;
        llc.SetType (protocolNumber);
        p->AddHeader (llc);
    
        lengthType = p->GetSize ();
        NS_ASSERT_MSG (lengthType <= m_frameSize - 18,
          "CsmaNetDevice::AddHeader(): 802.3 Length/Type field with LLC/SNAP: "
          "length interpretation must not exceed device frame size minus overhead");
      }
      break;
    case ILLEGAL:
    default:
      NS_FATAL_ERROR ("EthPointToPointNetDevice::AddHeader(): Unknown packet encapsulation mode");
      break;
    }

  //NS_LOG_LOGIC ("header.SetLengthType (" << lengthType << ")");
  header.SetLengthType (lengthType);
  p->AddHeader (header);

  trailer.CalcFcs (p);
  p->AddTrailer (trailer);
}

EthernetHeader 
EthPointToPointNetDevice::ProcessHeader(Ptr<Packet> p, uint16_t& param)
{  
  NS_LOG_FUNCTION (p << param);

  EthernetTrailer trailer;
      
  p->RemoveTrailer (trailer);
  trailer.CheckFcs (p);

  EthernetHeader header (false);
  p->RemoveHeader (header);

  if ((header.GetDestination () != GetBroadcast ()) &&
      (header.GetDestination () != GetAddress ()))
    {
      //return false;
      return header;
    }

  switch (m_encapMode)
    {
    case DIX:
      param = header.GetLengthType ();
      break;
    case LLC: 
      {
        LlcSnapHeader llc;
        p->RemoveHeader (llc);
        param = llc.GetType ();
      } 
      break;
    case ILLEGAL:
    default:
      NS_FATAL_ERROR ("EthPointToPointNetDevice::ProcessHeader(): Unknown packet encapsulation mode");
      break;
    }
  //return true;
  return header;
}

  void 
EthPointToPointNetDevice::SetDataRate(DataRate bps)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_bps = bps;
}

  void 
EthPointToPointNetDevice::SetInterframeGap(Time t)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_tInterframeGap = t;
}

  bool
EthPointToPointNetDevice::TransmitStart (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_INFO ("EthPointToPointNetDevice::TransmitStart Node[" << m_node->GetId() << "]:Port[" << m_ifIndex <<"] => UID is " << p->GetUid ());

  //
  // This function is called to start the process of transmitting a packet.
  // We need to tell the channel that we've started wiggling the wire and
  // schedule an event that will be executed when the transmission is complete.
  //
  NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
  m_txMachineState = BUSY;
  m_currentPkt = p;
  m_phyTxBeginTrace (m_currentPkt);

  Time txTime = Seconds (m_bps.CalculateTxTime(p->GetSize()));
  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_INFO ("EthPointToPointNetDevice::TransmitStart => end of transmission of UID " << p->GetUid () << " in " << txCompleteTime << " sec");
  Simulator::Schedule (txCompleteTime, &EthPointToPointNetDevice::TransmitComplete, this);

  bool result = m_channel->TransmitStart(p, this, txTime); 
  if (result == false)
    {
      m_phyTxDropTrace (p);
    }
  return result;
}

  void 
EthPointToPointNetDevice::TransmitComplete (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  
  // This function is called to when we're all done transmitting a packet.
  // We try and pull another packet off of the transmit queue.  If the queue
  // is empty, we are done, otherwise we need to start transmitting the
  // next packet.
  NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
  m_txMachineState = READY;

  NS_ASSERT_MSG (m_currentPkt != 0, "EthPointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

  m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  Ptr<Packet> p = m_queue->Dequeue ();
  if (p == 0)
    {
      // No packet was on the queue, so we just exit.
      NS_LOG_INFO ("EthPointToPointNetDevice::TransmitComplete Node[" << m_node->GetId() << "]:Port[" << m_ifIndex <<"] => no more packets in the queue");
      return;
    }

  // Got another packet off of the queue, so start the transmit process agin.
  NS_LOG_INFO ("EthPointToPointNetDevice::TransmitComplete Node[" << m_node->GetId() << "]:Port[" << m_ifIndex <<"] => next packet to transmit with UID " << p->GetUid ());
  TransmitStart(p);
}

  bool 
EthPointToPointNetDevice::Attach (Ptr<EthPointToPointChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_channel->Attach(this);

  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  NotifyLinkUp ();
  return true;
}

  void
EthPointToPointNetDevice::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

  void
EthPointToPointNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

 void
EthPointToPointNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  NS_LOG_INFO ("EthPointToPointNetDevice::Receive => UID is " << packet->GetUid ());
  
  uint16_t protocol = 0;

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) ) 
    {
      // 
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
      m_phyRxDropTrace (packet);
    }
  else 
    {
      // 
      // Hit the trace hooks.  All of these hooks are in the same place in this 
      // device becuase it is so simple, but this is not usually the case in 
      // more complicated devices.
      //
      m_phyRxEndTrace (packet);

      //
      // Strip off the ethernet header and forward this packet
      // up the protocol stack.  Since this is a simple point-to-point link,
      // there is no difference in what the promisc callback sees and what the
      // normal receive callback sees.
      //
      EthernetHeader ethHdr = ProcessHeader(packet, protocol);

      if (!m_promiscCallback.IsNull ())
        {
          m_macPromiscRxTrace (packet);
          if(ethHdr.GetDestination() == Mac48Address("01:80:c2:00:00:00"))
            {
              NS_LOG_LOGIC ("m_promiscCallback is not null and destination is BPDUs group address, forward up to m_promiscCallback with type Multicast");
              m_promiscCallback (this, packet, ethHdr.GetLengthType (), ethHdr.GetSource (), ethHdr.GetDestination (), NetDevice::PACKET_MULTICAST);              
            }
          else if (ethHdr.GetDestination() == Mac48Address("ff:ff:ff:ff:ff:ff"))
            {
              NS_LOG_LOGIC ("m_promiscCallback is not null and destination is broadcast address, forward up to m_promiscCallback with type Broadcast");
              m_promiscCallback (this, packet, ethHdr.GetLengthType (), ethHdr.GetSource (), ethHdr.GetDestination (), NetDevice::PACKET_BROADCAST);
            }
          else
            {            
              NS_LOG_LOGIC ("m_promiscCallback is not null and destination is NOT BPDUs group address, forward up to m_promiscCallback with type otherhost");
              m_promiscCallback (this, packet, ethHdr.GetLengthType (), ethHdr.GetSource (), ethHdr.GetDestination (), NetDevice::PACKET_OTHERHOST);
            }

        }
      //NS_LOG_LOGIC ("forward up to m_rxCallback");
      //m_macRxTrace (packet);
      //m_rxCallback (this, packet, protocol, GetRemote ());
    }
}

  Ptr<Queue> 
EthPointToPointNetDevice::GetQueue(void) const 
{ 
  NS_LOG_FUNCTION_NOARGS ();
  return m_queue;
}

  void
EthPointToPointNetDevice::NotifyLinkUp (void)
{
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

  void 
EthPointToPointNetDevice::SetIfIndex(const uint32_t index)
{
  m_ifIndex = index;
}

  uint32_t 
EthPointToPointNetDevice::GetIfIndex(void) const
{
  return m_ifIndex;
}

  Ptr<Channel> 
EthPointToPointNetDevice::GetChannel (void) const
{
  return m_channel;
}

  void 
EthPointToPointNetDevice::SetAddress (Address address)
{
  m_address = Mac48Address::ConvertFrom (address);
}

  Address 
EthPointToPointNetDevice::GetAddress (void) const
{
  return m_address;
}

  bool 
EthPointToPointNetDevice::IsLinkUp (void) const
{
  return m_linkUp;
}

  void 
EthPointToPointNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a point-to-point device, so every transmission is a broadcast to
// all of the devices on the network.
//
  bool 
EthPointToPointNetDevice::IsBroadcast (void) const
{
  return true;
}

//
// We don't really need any addressing information since this is a 
// point-to-point device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
  Address
EthPointToPointNetDevice::GetBroadcast (void) const
{
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

//
// We don't deal with multicast here.  It doesn't make sense to include some
// of the one destinations on the network but exclude some of the others.
//
  bool 
EthPointToPointNetDevice::IsMulticast (void) const
{
  return false;
}

  Address 
EthPointToPointNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
EthPointToPointNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION(this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

  bool 
EthPointToPointNetDevice::IsPointToPoint (void) const
{
  return true;
}

  bool 
EthPointToPointNetDevice::IsBridge (void) const
{
  return false;
}

  bool 
EthPointToPointNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
EthPointToPointNetDevice::SendFrom (Ptr<Packet> packet, const Address &src, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_INFO ("EthPointToPointNetDevice::SendFrom => UID is " << packet->GetUid ());

  //
  // If IsLinkUp() is false it means there is no channel to send any packet 
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  Mac48Address destination = Mac48Address::ConvertFrom (dest);
  Mac48Address source = Mac48Address::ConvertFrom (src);
  AddHeader (packet, source, destination, protocolNumber);

  m_macTxTrace (packet);

  // If there's a transmission in progress, we enque the packet for later
  // transmission; otherwise we send it now.
  if (m_txMachineState == READY) 
    {
      // Even if the transmitter is immediately available, we still enqueue and
      // dequeue the packet to hit the tracing hooks.
      NS_LOG_INFO ("EthPointToPointNetDevice::SendFrom => enqueue+dequeue UID " << packet->GetUid ());
      m_queue->Enqueue (packet);
      packet = m_queue->Dequeue ();
      return TransmitStart (packet);
    }
  else
    {
      NS_LOG_INFO ("EthPointToPointNetDevice::SendFrom => enqueue UID " << packet->GetUid () << " (numPacketsQueue: " << m_queue->GetNPackets() << ") (time: " << Simulator::Now() << ")");
      return m_queue->Enqueue(packet);
    }
  return false;
}

  Ptr<Node> 
EthPointToPointNetDevice::GetNode (void) const
{
  return m_node;
}

  void 
EthPointToPointNetDevice::SetNode (Ptr<Node> node)
{
  m_node = node;
}

  bool 
EthPointToPointNetDevice::NeedsArp (void) const
{
  return false;
}

  void 
EthPointToPointNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_rxCallback = cb;
}

void
EthPointToPointNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_promiscCallback = cb;
}

  bool
EthPointToPointNetDevice::SupportsSendFrom (void) const
{
  return true;
}


Address 
EthPointToPointNetDevice::GetRemote (void) const
{
  NS_ASSERT (m_channel->GetNDevices () == 2);
  for (uint32_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
      if (tmp != this)
        {
          return tmp->GetAddress ();
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return Address ();
}

  uint32_t
EthPointToPointNetDevice::MtuFromFrameSize (uint32_t frameSize)
{
  NS_LOG_FUNCTION (frameSize);

  NS_ASSERT_MSG (frameSize <= std::numeric_limits<uint16_t>::max (), 
                 "EthPointToPointNetDevice::MtuFromFrameSize(): Frame size should be derived from 16-bit quantity: " << 
                 frameSize);

  uint32_t newSize;

  switch (m_encapMode) 
    {
    case DIX:
      newSize = frameSize - ETHERNET_OVERHEAD;
      break;
    case LLC: 
      {
        LlcSnapHeader llc;

        NS_ASSERT_MSG ((uint32_t)(frameSize - ETHERNET_OVERHEAD) >= llc.GetSerializedSize (), 
                       "EthPointToPointNetDevice::MtuFromFrameSize(): Given frame size too small to support LLC mode");
        newSize = frameSize - ETHERNET_OVERHEAD - llc.GetSerializedSize ();
      }
      break;
    case ILLEGAL:
    default:
      NS_FATAL_ERROR ("EthPointToPointNetDevice::MtuFromFrameSize(): Unknown packet encapsulation mode");
      return 0;
    }

  return newSize;
}
  
  uint32_t
EthPointToPointNetDevice::FrameSizeFromMtu (uint32_t mtu)
{
  NS_LOG_FUNCTION (mtu);

  uint32_t newSize;

  switch (m_encapMode) 
    {
    case DIX:
      newSize = mtu + ETHERNET_OVERHEAD;
      break;
    case LLC: 
      {
        LlcSnapHeader llc;
        newSize = mtu + ETHERNET_OVERHEAD + llc.GetSerializedSize ();
      }
      break;
    case ILLEGAL:
    default:
      NS_FATAL_ERROR ("EthPointToPointNetDevice::FrameSizeFromMtu(): Unknown packet encapsulation mode");
      return 0;
    }

  return newSize;
}

  void 
EthPointToPointNetDevice::SetEncapsulationMode (enum EncapsulationMode mode)
{
  NS_LOG_FUNCTION (mode);

  m_encapMode = mode;
  m_mtu = MtuFromFrameSize (m_frameSize);
}

  void 
EthPointToPointNetDevice::SetFrameSize (uint16_t frameSize)
{
  NS_LOG_FUNCTION (frameSize);

  m_frameSize = frameSize;
  m_mtu = MtuFromFrameSize (frameSize);
}

  uint16_t
EthPointToPointNetDevice::GetFrameSize (void) const
{
  return m_frameSize;
}

  bool
EthPointToPointNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);

  uint32_t newFrameSize = FrameSizeFromMtu (mtu);

  if (newFrameSize > std::numeric_limits<uint16_t>::max ())
    {
      NS_LOG_WARN ("EthPointToPointNetDevice::SetMtu(): Frame size overflow, MTU not set.");
      return false;
    }

  m_frameSize = newFrameSize;
  m_mtu = mtu;

  return true;
}

  uint16_t
EthPointToPointNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}


} // namespace ns3
