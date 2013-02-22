/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 Universitat Pompeu Fabra
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
 * Author:  Eduard Bonada (eduard.bonada@upf.edu)
 * 
 * Header file of the TrafficSink Class
 * Folder: /src/netcom-platform/traffic-sink
 * Files: traffic-sink.h & traffic-sink.cc
 * Functionality: Receives packets and shows a message 
 */
 
#include "ns3/address.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "bridge-traff-sink.h"
#include "ns3/source-tag.h"
#include "ns3/seq-tag.h"
//#include "ns3/timestamp-tag.h"

//using namespace std;

NS_LOG_COMPONENT_DEFINE ("BridgeTraffSink");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BridgeTraffSink);

TypeId
BridgeTraffSink::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BridgeTraffSink")
    .SetParent<Application> ()
    ;
  return tid;
}

BridgeTraffSink::BridgeTraffSink (Ptr<Node> n)
{
/*
  - Constructor of the TrafficSink class
  - Parameters
    - n: pointer to the node where the TrafficSink is attached
*/
  Construct (n);
}

void
BridgeTraffSink::Construct (Ptr<Node> n)
{
/*
  - really sets up the traffic sink
  - stores the pointer to the node to the corresponding internal variable (m_node)
*/
  m_node = n;

  for(int i=0;i<200;i++) m_lastSeqRecvd.push_back(0);
}

BridgeTraffSink::~BridgeTraffSink()
{
/*
  - Destructor of the class
*/
}

void
BridgeTraffSink::DoDispose (void)
{
/*
  - method automatically called when the instance is removed (must exist)
*/
  Application::DoDispose ();
}

void BridgeTraffSink::StartApplication()
{
/*
  - called from the main script (TS->start(time_to_start)) when the application becomes active
*/
}

void BridgeTraffSink::StopApplication()
{
/*
  - called from the main script (TS->stop(time_to_start)) when the application becomes inactive
*/
}

void BridgeTraffSink::Receive(Ptr<const Packet> packet, uint16_t protocol, Mac48Address src, Mac48Address dst)
{
/*
  - interface with external objects
  - called from the port (if the port is connected to the sink)
  - prints out that a packet has been recieved
*/

  uint16_t sourceId;
  SourceTag sidTag;
  bool isThereTag = ConstCast<Packet> (packet)->RemovePacketTag (sidTag); //packet->RemovePacketTag (sidTag);
  if(isThereTag) sourceId = sidTag.Get();

  uint16_t seq;
  SeqTag sequenceTag;
  isThereTag = ConstCast<Packet> (packet)->RemovePacketTag (sequenceTag); //packet->RemovePacketTag (sidTag);
  if(isThereTag) seq = sequenceTag.Get();

/*  uint32_t timestamp;
  TimestampTag timestampTag;
  isThereTag = ConstCast<Packet> (packet)->RemovePacketTag (timestampTag); //packet->RemovePacketTag (sidTag);
  if(isThereTag) timestamp = timestampTag.Get();
*/  
  if(sourceId!=m_node->GetId() && m_lastSeqRecvd.at(sourceId) < seq)
    {
      m_lastSeqRecvd.at(sourceId) = seq;  
      NS_LOG_INFO(Simulator::Now() << " Traffic Sink[" << m_node->GetId() << "]      Receives packet (Length:" << packet->GetSize() << "|DA:" << dst << "|SA:" << src << "|Type:" << protocol << "|source:" << sourceId << "|seq:" << seq << ")"); //|ts:" << timestamp << ") (delay: " << (Simulator::Now()).GetMicroSeconds() - timestamp << " )");
    }
}

} // Namespace ns3
