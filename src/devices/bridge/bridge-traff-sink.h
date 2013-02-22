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
 */

#ifndef __BRIDGE_TRAFF_SINK_H__
#define __BRIDGE_TRAFF_SINK_H__

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "bridge-net-device.h"

#include <iostream>
#include <fstream>

using namespace std;

namespace ns3 {

class Address;
class Packet;

class BridgeTraffSink : public Application 
{
public:
  static TypeId GetTypeId (void);
  
  BridgeTraffSink (Ptr<Node> n);
  virtual ~BridgeTraffSink ();

  virtual void Receive (Ptr<const Packet> packet, uint16_t protocol, Mac48Address src, Mac48Address dst);

protected:
  virtual void DoDispose (void);
private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void Construct (Ptr<Node> n);
  
  Ptr<Node>	   m_node;                  //Pointer to the node where the traffic sink is located    

  std::vector< uint16_t > m_lastSeqRecvd;
};

} // namespace ns3

#endif

