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
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
 
#include <cstdlib>
 
#include "bridge-net-device.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "tree-tag-header.h"

NS_LOG_COMPONENT_DEFINE ("BridgeNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BridgeNetDevice);


TypeId
BridgeNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BridgeNetDevice")
    .SetParent<NetDevice> ()
    .AddConstructor<BridgeNetDevice> ()
    .AddAttribute ("EnableLearning",
                   "Enable the learning mode of the Learning Bridge",
                   BooleanValue (true),
                   MakeBooleanAccessor (&BridgeNetDevice::m_enableLearning),
                   MakeBooleanChecker ())
    .AddAttribute ("ExpirationTime",
                   "Time it takes for learned MAC state entry to expire.",
                   TimeValue (Seconds (300)),
                   MakeTimeAccessor (&BridgeNetDevice::m_expirationTime),
                   MakeTimeChecker ())
    ;
  return tid;
}


BridgeNetDevice::BridgeNetDevice ()
  : m_node (0),
    m_ifIndex (0),
    m_mtu (0xffff)
{
  m_channel = CreateObject<BridgeChannel> ();
}

BridgeNetDevice::~BridgeNetDevice()
{
}


void 
BridgeNetDevice::DoDispose ()
{
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
    {
      *iter = 0;
    }
  m_ports.clear ();
  m_channel = 0;
  m_node = 0;
  NetDevice::DoDispose ();
}

void 
BridgeNetDevice::SetNumNodes (uint64_t n)
{
  std::vector<bool> arrayTmp;
  
  m_numNodes = n;
  
  // start with the port blocked for all trees
  for (uint16_t p=0 ; p < m_ports.size () ; p++ )
    {
      for (uint64_t n=0 ; n < m_numNodes ; n++)
        {
          arrayTmp.push_back(false); 
        }
      m_portForwarding.push_back(arrayTmp); 
    }
}

void 
BridgeNetDevice::SetBridgeId (uint64_t b)
{
  m_bridgeId = b;
  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::SetBridgeId => Setting BridgeId  to " << m_bridgeId);
}

void 
BridgeNetDevice::AddBridgePort (Ptr<NetDevice> bridgePort)
{
  //NS_LOG_FUNCTION_NOARGS ();
  
  NS_ASSERT (bridgePort != this);
  
  if (!Mac48Address::IsMatchingType (bridgePort->GetAddress ()))
    {
      NS_FATAL_ERROR ("BridgeNetDevice[" << m_node->GetId() << "]::AddBridgePort => Device does not support eui 48 addresses: cannot be added to bridge.");
    }
  if (!bridgePort->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("BridgeNetDevice[" << m_node->GetId() << "]::AddBridgePort => Device does not support SendFrom: cannot be added to bridge.");
    }
  if (m_address == Mac48Address ())
    {
      m_address = Mac48Address::ConvertFrom (bridgePort->GetAddress ());
    }

  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::AddBridgePort => RegisterProtocolHandler");
  
  m_node->RegisterProtocolHandler (MakeCallback (&BridgeNetDevice::ReceiveFromDevice, this), 8888, bridgePort, true); // protocol 8888 should be data frames, BPDUs are 9999
  m_ports.push_back (bridgePort);
  m_channel->AddChannel (bridgePort->GetChannel ());

}

void
BridgeNetDevice::SetTraffSink (Ptr<BridgeTraffSink> sink)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_traffSink = sink;
}

void
BridgeNetDevice::SetPortState (int64_t tree, uint16_t port, bool forward)
{
  //NS_LOG_FUNCTION_NOARGS ();
  
  if(tree == -1) // set port state in all trees (single tree protocol)
    {    
      if(forward) NS_LOG_INFO (Simulator::Now() << " Node[" << m_node->GetId() << "]:Port[" << port << "] BRDG: Port goes forwarding [All trees]");
      else        NS_LOG_INFO (Simulator::Now() << " Node[" << m_node->GetId() << "]:Port[" << port << "] BRDG: Port goes blocking [All trees]");      
      for ( uint64_t n=0 ; n < m_numNodes ; n++ )
        {
          m_portForwarding.at(port).at(n) = forward;
        }
    }
  else
    {
      if(forward) NS_LOG_INFO (Simulator::Now() << " Node[" << m_node->GetId() << "]:Port[" << port << "] BRDG: Port goes forwarding [tree " << tree << "]");
      else        NS_LOG_INFO (Simulator::Now() << " Node[" << m_node->GetId() << "]:Port[" << port << "] BRDG: Port goes blocking [tree " << tree << "]");      
      m_portForwarding.at(port).at(tree) = forward;
    }
  
}

bool
BridgeNetDevice::IsPortForwarding (uint16_t port, uint64_t tree)
{
  return m_portForwarding.at(port).at(tree);
}

void
BridgeNetDevice::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  //NS_LOG_FUNCTION_NOARGS ();

  Mac48Address src48 = Mac48Address::ConvertFrom (src);
  Mac48Address dst48 = Mac48Address::ConvertFrom (dst);
  
  // extract the TreeTag header (from the copy)
  TreeTagHeader treeTagHeader = TreeTagHeader ();
  Ptr<Packet> p = packet->Copy ();
  p->RemoveHeader(treeTagHeader);
  uint64_t treeTag = treeTagHeader.GetTreeTag();
  
  // remove the header from the packet that will be forwarded to the sink
  TreeTagHeader treeTagHeaderTmp = TreeTagHeader ();
  Ptr<Packet> pTmp = packet->Copy ();
  pTmp->RemoveHeader(treeTagHeaderTmp);

  
  if( IsPortForwarding(incomingPort->GetIfIndex(), treeTag)==true )
    {   
      NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ReceiveFromDevice => Received frame [DA:" << dst48 << "|SA:" << src48 << "|type:" << protocol << "|tree:" << treeTag << "]");

      Learn (src48, incomingPort);       
 
      if (!m_promiscRxCallback.IsNull ())
        {
          m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
        }

      switch (packetType)
        {
        case PACKET_HOST:
          if (dst48 == m_address)
            {
              NS_LOG_ERROR("ERROR BridgeNetDevice[" << m_node->GetId() << "]: a PACKET_HOST type received when the netDevice is supposed to not marke frames with this type");
              exit(-1);
            }
          break;

        case PACKET_BROADCAST:
          NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ReceiveFromDevice => PACKET_BROADCAST forwarded to m_traffSink and to ForwardBroadcast()");
          
          m_traffSink->Receive (pTmp->Copy(), protocol, src48, dst48);
          
          ForwardBroadcast (incomingPort, packet->Copy (), protocol, src48, dst48, treeTag);
          
          break;
          
        case PACKET_MULTICAST:
          // if is is directed to 01:80:C2:00:00:00 (BPDU Group addresses, not to be forwarded)
          NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ReceiveFromDevice => PACKET_MULTICAST receuved, what to do?");
          exit(-1);
          break;

        case PACKET_OTHERHOST:
          if (dst48 == m_address)
            {
              NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ReceiveFromDevice => PACKET_OTHERHOST and local address forwarded to m_traffSink");
              m_traffSink->Receive (pTmp->Copy(), protocol, src48, dst48);
            }
          else
            {
              NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ReceiveFromDevice => PACKET_OTHERHOST and not local address forwarded to ForwardUnicast");
              ForwardUnicast (incomingPort, packet->Copy (), protocol, src48, dst48, treeTag);
            }
          break;
        }
    }
  else //port is not forwarding
    {
      NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ReceiveFromDevice => Discarded frame received in a blocked port [DA:" << dst48 << "|SA:" << src48 << "|type:" << protocol << "]");
    }
}

void
BridgeNetDevice::ForwardUnicast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst, uint64_t treeTag)
{
  //NS_LOG_FUNCTION_NOARGS ();
  
  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ForwardBroadcast => Forward frame [DA:" << dst << "|SA:" << src << "|type:" << protocol << "]");
 
  Ptr<NetDevice> outPort = GetLearnedState (dst);
    
  if (outPort != NULL)
    {
      if(outPort != incomingPort && IsPortForwarding(outPort->GetIfIndex(), treeTag)==true)
        { 
          NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ForwardUnicast => Forward frame to destination " << dst << " already learned in port " << outPort->GetIfIndex());
          outPort->SendFrom (packet->Copy (), src, dst, protocol);
        }
    }
  else
    {
      NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ForwardUnicast => Destination " << dst << " not yet learned, forward to all except incoming port " << incomingPort->GetIfIndex());
      for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
        {
          Ptr<NetDevice> port = *iter;
          if (port != incomingPort && IsPortForwarding(port->GetIfIndex(), treeTag)==true)
            {
              NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ForwardUnicast => Forward frame through port " << port->GetIfIndex());
              port->SendFrom (packet->Copy (), src, dst, protocol);
            }
        }
    }
}

void
BridgeNetDevice::ForwardBroadcast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                        uint16_t protocol, Mac48Address src, Mac48Address dst, uint64_t treeTag)
{
  //NS_LOG_FUNCTION_NOARGS ();

  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ForwardBroadcast => Forward frame to all ports except incoming [DA:" << dst << "|SA:" << src << "|type:" << protocol << "]");
  
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
    {
      bool activePort=false;
      Ptr<NetDevice> port = *iter;
      if(IsPortForwarding(port->GetIfIndex(), treeTag)==true) activePort=true;
      if (port != incomingPort && activePort)
        {
          NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::ForwardBroadcast => Forward frame through port " << port->GetIfIndex());
          port->SendFrom (packet->Copy (), src, dst, protocol);
        }
    }
}

void BridgeNetDevice::Learn (Mac48Address source, Ptr<NetDevice> port)
{
  //NS_LOG_FUNCTION_NOARGS ();
  
  if (m_enableLearning)
    {
      NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::Learn => Updates m_learnState with source " << source << " - port: " << port->GetIfIndex());
      LearnedState &state = m_learnState[source];
      state.associatedPort = port;
      state.expirationTime = Simulator::Now () + m_expirationTime;
    }
  PrintMacTable();
}

Ptr<NetDevice> BridgeNetDevice::GetLearnedState (Mac48Address source)
{
  //NS_LOG_FUNCTION_NOARGS ();
  
  if (m_enableLearning)
    {
      Time now = Simulator::Now ();
      std::map<Mac48Address, LearnedState>::iterator iter = m_learnState.find (source);
      if (iter != m_learnState.end ())
        {
          LearnedState &state = iter->second;
          if (state.expirationTime > now)
            {
              NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::GetLearnedState => Entry indicates address " << source << " in port " << (state.associatedPort)->GetIfIndex());              
              return state.associatedPort;
            }
          else
            {
              NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::GetLearnedState => Entry already expired (but removed now)");
              m_learnState.erase (iter);
            }
        }
    }
  return NULL;
}

bool 
BridgeNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  //decide the treeTag header value
  uint64_t treeTag = m_bridgeId;

  // add the TreeTag header  
  TreeTagHeader treeTagHeader = TreeTagHeader ();
  treeTagHeader.SetTreeTag(treeTag); 
  packet->AddHeader(treeTagHeader);

  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::Send => Sending frame tree " << treeTag << "(bridgeId:" << m_bridgeId << ")");
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool 
BridgeNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  //NS_LOG_FUNCTION_NOARGS ();
   
  Mac48Address dst = Mac48Address::ConvertFrom (dest);
  
  // extract the TreeTag header (from the copy)
  TreeTagHeader treeTagHeader = TreeTagHeader ();
  Ptr<Packet> p = packet->Copy ();
  p->RemoveHeader(treeTagHeader);
  uint64_t treeTag = treeTagHeader.GetTreeTag();

  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::SendFrom => Sending frame [DA:" << dst << "|SA:" << Mac48Address::ConvertFrom (src) << "|type:" << protocolNumber << "|tree:" << treeTag << "]");

  // try to use the learned state if data is unicast
  if (!dst.IsGroup ())
    {
      Ptr<NetDevice> outPort = GetLearnedState (dst);
      if (outPort != NULL) 
        {
          if (IsPortForwarding(outPort->GetIfIndex(), treeTag)==true)
            {
              NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::SendFrom => Destination learned so send to port " << outPort->GetIfIndex());
              outPort->SendFrom (packet->Copy(), src, dest, protocolNumber);
              return true;
            }
        }
    }

  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::SendFrom => Destination not yet learned, or group/multicast/broadcast, so send all ports");
  // data was not unicast or no state has been learned for that mac
  // address => flood through all ports.
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
    {
      Ptr<NetDevice> port = *iter;
      if (IsPortForwarding(port->GetIfIndex(), treeTag)==true)
        {
          NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::SendFrom => Send to port " << port->GetIfIndex() << "(tree:" << treeTag << ")");
          port->SendFrom (packet->Copy(), src, dest, protocolNumber);
        }
    }

  return true;
}

void
BridgeNetDevice::ChangeExpirationTime (uint16_t value)
{
  // NS_LOG_FUNCTION_NOARGS ();
  
  NS_LOG_INFO (Simulator::Now() << " Node[" << m_node->GetId() << "]:Port[-] BRDG: Expiration Time Changed to " << value);
  
  // change expiration of all entries according to the new expiration value
  for (std::map<Mac48Address, LearnedState>::iterator iter = m_learnState.begin(); iter != m_learnState.end (); iter++)
    {
      // m_learnState strores the 'global' time of the expiration, not the timer value , hence we need to substract the difference between the current expiration time and the previous
      Time previous = (*iter).second.expirationTime;
      (*iter).second.expirationTime = previous - (m_expirationTime - Seconds(value));
      
      // delete entry if already expired
      //if(Simulator::Now() > (*iter).second.expirationTime )
        //{
          //m_learnState.erase (iter);
        //}
    }
  
  // set value to m_expirationTime variable (for coming learned frames)
  m_expirationTime = Seconds(value);
  
  PrintMacTable();
}

void
BridgeNetDevice::FlushEntriesOfPort (uint16_t port)
{
  // NS_LOG_FUNCTION_NOARGS ();
  
  NS_LOG_INFO (Simulator::Now() << " Node[" << m_node->GetId() << "]:Port[-] BRDG: Flush Entries of port " << port);
  
  // Loop all entries and delete those that have the given port as output port
  // actually, don't delete them, but set the expiration time to Simulation::Now().
  for (std::map<Mac48Address, LearnedState>::iterator iter = m_learnState.begin(); iter != m_learnState.end (); iter++)
    {
      if( (*iter).second.associatedPort->GetIfIndex() == port)
        {
          (*iter).second.expirationTime = Simulator::Now();
        }
    }  
}

void
BridgeNetDevice::PrintMacTable ()
{
  NS_LOG_LOGIC ("BridgeNetDevice[" << m_node->GetId() << "]::PrintMacTable at " << Simulator::Now());
  
  NS_LOG_LOGIC("\tMAC\tPort\tExp");
  for (std::map<Mac48Address, LearnedState>::iterator iter = m_learnState.begin(); iter != m_learnState.end (); iter++)
    {
      NS_LOG_LOGIC("\t" << (*iter).first << "\t" << (*iter).second.associatedPort->GetIfIndex() << "\t" << (*iter).second.expirationTime);
    }  
}

uint32_t
BridgeNetDevice::GetNBridgePorts (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}
Ptr<NetDevice>
BridgeNetDevice::GetBridgePort (uint32_t n) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_ports[n];
}
void 
BridgeNetDevice::SetIfIndex(const uint32_t index)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_ifIndex = index;
}
uint32_t 
BridgeNetDevice::GetIfIndex(void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}
Ptr<Channel> 
BridgeNetDevice::GetChannel (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}
void
BridgeNetDevice::SetAddress (Address address)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_address = Mac48Address::ConvertFrom (address);
}
Address 
BridgeNetDevice::GetAddress (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}
bool 
BridgeNetDevice::SetMtu (const uint16_t mtu)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_mtu = mtu;
  return true;
}
uint16_t 
BridgeNetDevice::GetMtu (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}
bool 
BridgeNetDevice::IsLinkUp (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return true;
}
void 
BridgeNetDevice::AddLinkChangeCallback (Callback<void> callback)
{}
bool 
BridgeNetDevice::IsBroadcast (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return true;
}
Address
BridgeNetDevice::GetBroadcast (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}
bool
BridgeNetDevice::IsMulticast (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return true;
}
Address
BridgeNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
 NS_LOG_FUNCTION (this << multicastGroup);
 Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
 return multicast;
}
bool 
BridgeNetDevice::IsPointToPoint (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return false;
}
bool 
BridgeNetDevice::IsBridge (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return true;
}
Ptr<Node> 
BridgeNetDevice::GetNode (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}
void 
BridgeNetDevice::SetNode (Ptr<Node> node)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
}
bool 
BridgeNetDevice::NeedsArp (void) const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void 
BridgeNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}
void 
BridgeNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}
bool
BridgeNetDevice::SupportsSendFrom () const
{
  //NS_LOG_FUNCTION_NOARGS ();
  return true;
}
Address BridgeNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

} // namespace ns3
