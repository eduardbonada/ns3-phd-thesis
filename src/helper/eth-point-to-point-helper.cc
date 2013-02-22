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
#include "eth-point-to-point-helper.h"
#include "ns3/simulator.h"
#include "ns3/queue.h"
#include "ns3/config.h"
#include "ns3/packet.h"
#include "ns3/names.h"
#include "ns3/pcap-writer.h"
#include "ns3/ascii-writer.h"

namespace ns3 {


EthPointToPointHelper::EthPointToPointHelper ()
{
  m_queueFactory.SetTypeId ("ns3::DropTailQueue");
  m_deviceFactory.SetTypeId ("ns3::EthPointToPointNetDevice");
  m_channelFactory.SetTypeId ("ns3::EthPointToPointChannel");
}

void 
EthPointToPointHelper::SetQueue (std::string type,
                              std::string n1, const AttributeValue &v1,
                              std::string n2, const AttributeValue &v2,
                              std::string n3, const AttributeValue &v3,
                              std::string n4, const AttributeValue &v4)
{
  m_queueFactory.SetTypeId (type);
  m_queueFactory.Set (n1, v1);
  m_queueFactory.Set (n2, v2);
  m_queueFactory.Set (n3, v3);
  m_queueFactory.Set (n4, v4);
}

void 
EthPointToPointHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  m_deviceFactory.Set (n1, v1);
}

void 
EthPointToPointHelper::SetChannelAttribute (std::string n1, const AttributeValue &v1)
{
  m_channelFactory.Set (n1, v1);
}

void 
EthPointToPointHelper::EnablePcap (std::string filename, uint32_t nodeid, uint32_t deviceid)
{
  std::ostringstream oss;
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::EthPointToPointNetDevice/";
  Config::MatchContainer matches = Config::LookupMatches (oss.str ());
  if (matches.GetN () == 0)
    {
      return;
    }
  oss.str ("");
  oss << filename << "-" << nodeid << "-" << deviceid << ".pcap";
  Ptr<PcapWriter> pcap = CreateObject<PcapWriter> ();
  pcap->Open (oss.str ());
  pcap->WritePppHeader ();
  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid;
  oss << "/$ns3::EthPointToPointNetDevice/PromiscSniffer";
  Config::ConnectWithoutContext (oss.str (), MakeBoundCallback (&EthPointToPointHelper::SniffEvent, pcap));
}

void 
EthPointToPointHelper::EnablePcap (std::string filename, Ptr<NetDevice> nd)
{
  EnablePcap (filename, nd->GetNode ()->GetId (), nd->GetIfIndex ());
}

void 
EthPointToPointHelper::EnablePcap (std::string filename, std::string ndName)
{
  Ptr<NetDevice> nd = Names::Find<NetDevice> (ndName);
  EnablePcap (filename, nd->GetNode ()->GetId (), nd->GetIfIndex ());
}

void 
EthPointToPointHelper::EnablePcap (std::string filename, NetDeviceContainer d)
{
  for (NetDeviceContainer::Iterator i = d.Begin (); i != d.End (); ++i)
    {
      Ptr<NetDevice> dev = *i;
      EnablePcap (filename, dev->GetNode ()->GetId (), dev->GetIfIndex ());
    }
}

void
EthPointToPointHelper::EnablePcap (std::string filename, NodeContainer n)
{
  NetDeviceContainer devs;
  for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
    {
      Ptr<Node> node = *i;
      for (uint32_t j = 0; j < node->GetNDevices (); ++j)
        {
          devs.Add (node->GetDevice (j));
        }
    }
  EnablePcap (filename, devs);
}

void
EthPointToPointHelper::EnablePcapAll (std::string filename)
{
  EnablePcap (filename, NodeContainer::GetGlobal ());
}

void 
EthPointToPointHelper::EnableAscii (std::ostream &os, uint32_t nodeid, uint32_t deviceid)
{
  Ptr<AsciiWriter> writer = AsciiWriter::Get (os);
  Packet::EnablePrinting ();
  std::ostringstream oss;
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::EthPointToPointNetDevice/MacRx";
  Config::Connect (oss.str (), MakeBoundCallback (&EthPointToPointHelper::AsciiRxEvent, writer));
  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::EthPointToPointNetDevice/TxQueue/Enqueue";
  Config::Connect (oss.str (), MakeBoundCallback (&EthPointToPointHelper::AsciiEnqueueEvent, writer));
  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::EthPointToPointNetDevice/TxQueue/Dequeue";
  Config::Connect (oss.str (), MakeBoundCallback (&EthPointToPointHelper::AsciiDequeueEvent, writer));
  oss.str ("");
  oss << "/NodeList/" << nodeid << "/DeviceList/" << deviceid << "/$ns3::EthPointToPointNetDevice/TxQueue/Drop";
  Config::Connect (oss.str (), MakeBoundCallback (&EthPointToPointHelper::AsciiDropEvent, writer));
}

void 
EthPointToPointHelper::EnableAscii (std::ostream &os, NetDeviceContainer d)
{
  for (NetDeviceContainer::Iterator i = d.Begin (); i != d.End (); ++i)
    {
      Ptr<NetDevice> dev = *i;
      EnableAscii (os, dev->GetNode ()->GetId (), dev->GetIfIndex ());
    }
}

void
EthPointToPointHelper::EnableAscii (std::ostream &os, NodeContainer n)
{
  NetDeviceContainer devs;
  for (NodeContainer::Iterator i = n.Begin (); i != n.End (); ++i)
    {
      Ptr<Node> node = *i;
      for (uint32_t j = 0; j < node->GetNDevices (); ++j)
        {
          devs.Add (node->GetDevice (j));
        }
    }
  EnableAscii (os, devs);
}

void
EthPointToPointHelper::EnableAsciiAll (std::ostream &os)
{
  EnableAscii (os, NodeContainer::GetGlobal ());
}

NetDeviceContainer 
EthPointToPointHelper::Install (Ptr<Node> a, Ptr<Node> b, Ptr<EthPointToPointChannel> *eptpch)
{
  NetDeviceContainer container;

  Ptr<EthPointToPointNetDevice> devA = m_deviceFactory.Create<EthPointToPointNetDevice> ();
  //devA->SetAddress (Mac48Address::Allocate ());
  a->AddDevice (devA);
  Ptr<Queue> queueA = m_queueFactory.Create<Queue> ();
  devA->SetQueue (queueA);
  Ptr<EthPointToPointNetDevice> devB = m_deviceFactory.Create<EthPointToPointNetDevice> ();
  //devB->SetAddress (Mac48Address::Allocate ());
  b->AddDevice (devB);
  Ptr<Queue> queueB = m_queueFactory.Create<Queue> ();
  devB->SetQueue (queueB);
  Ptr<EthPointToPointChannel> channel = m_channelFactory.Create<EthPointToPointChannel> ();
  *eptpch = channel;
  devA->Attach (channel);
  devB->Attach (channel);
  container.Add (devA);
  container.Add (devB);

  return container;
}

void 
EthPointToPointHelper::SniffEvent (Ptr<PcapWriter> writer, Ptr<const Packet> packet)
{
  writer->WritePacket (packet);
}

void
EthPointToPointHelper::AsciiEnqueueEvent (Ptr<AsciiWriter> writer, std::string path,
                                       Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::ENQUEUE, path, packet);
}

void
EthPointToPointHelper::AsciiDequeueEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::DEQUEUE, path, packet);
}

void
EthPointToPointHelper::AsciiDropEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::DROP, path, packet);
}

void
EthPointToPointHelper::AsciiRxEvent (Ptr<AsciiWriter> writer, std::string path, Ptr<const Packet> packet)
{
  writer->WritePacket (AsciiWriter::RX, path, packet);
}

} // namespace ns3
