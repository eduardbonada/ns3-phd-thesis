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
 * Author: Eduard Bonada <eduard.bonada@upf.edu>
 * 
 * Implementation of the TrafficGenerator class
 * Functionality: creates packets based on several configurable distributions
 */

#ifndef __BRIDGE_TRAFF_GEN_H
#define __BRIDGE_TRAFF_GEN_H

#include <fstream>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/net-device.h"
#include "ns3/random-variable.h"
#include "ns3/data-rate.h"
#include "bridge-net-device.h"


///////////////////////////////////////////////////
// copy this to main file and traffgen if changed
#define DIST_CONSTANT     1
#define DIST_UNIFORM      2
#define DIST_EXPONENTIAL  3
#define DIST_PARETO       4
#define DIST_NORMAL       5
#define DIST_LOGNORMAL    6
// copy this to main file and traffgen if changed
///////////////////////////////////////////////////

namespace ns3 {

class Address;
class RandomVariable;

class BridgeTraffGen : public Application 
{
public:
  static TypeId GetTypeId (void);

  BridgeTraffGen(Ptr<Node> n, Ptr<BridgeNetDevice> bridge);
  virtual ~BridgeTraffGen();
  
  void SetDestMacAddress(Mac48Address);
  void SetTypeLength(uint16_t);
   
  void SetPktLength(int distr, double mean, double variance, double lowerBound, double upperBound);
  void SetInterPktTime(int distr, double mean, double variance, double lowerBound, double upperBound);
  void SetNumPktToGenerate(uint64_t);
  
protected:
  virtual void DoDispose (void);
  
private:
  void Construct (Ptr<Node> n, Ptr<BridgeNetDevice> bridge);

  virtual void StartApplication (void);   // start the application
  virtual void StopApplication (void);    // stop the application

  void StartSending();                    // starts sending (to model a flow, for example)
  void StopSending();                     // stops sending (to model a flow, for example)

  void GeneratePacket();                  // the function computes the packet length, creates the packet and sends it to the output (port)
  void ScheduleNext();                    // the function computes the time of the next packet to be generated and schedules an event pointing to GeneratePacket()
  
  uint32_t ComputePacketLength(int);      // returns the size of the packet to be generated
  Time   ComputeInterPacketTime(int);     // returns the time between packet generations (the interarrival, not the absolute time)
	
	
  // variables
  Ptr<Node>               m_node;         // pointer to the node where the traffic generator is located
  Ptr<BridgeNetDevice>    m_bridge;       // pointer to the bridge where the traffic generator directs its sent packets

  int              m_pktLengthDistr;
  double           m_meanPktLength;        // size (bytes) of the packet - constant distribution, exponential mean, normal mean
  double           m_variancePktLength;    // variance size (bytes) of the packet - normal variance
  double           m_lowerPktLength;       // size (bytes) of the packet - lower bound for uniform distribution
  double           m_upperPktLength;       // size (bytes) of the packet - upper bound for uniform distribution
  
  int              m_interPktTimeDistr;
  double           m_meanInterPktTime;     // time in seconds between packet generations - constant distribution, exponential mean, normal mean 
  double           m_varianceInterPktTime; // variance time in seconds between packet generations - normal variance 
  double           m_lowerInterPktTime;    // time in seconds between packet generations - lower bound for uniform distribution 
  double           m_upperInterPktTime;    // time in seconds between packet generations - upper bound for uniform distribution
  
  Mac48Address     m_destMacAddress;       // Destination MAC address to be conveyed in the generated frames
  uint16_t         m_typeLength;           // Type/Length to be conveyed in the generated frame

  EventId          m_sendEvent;           // Eventid of pending "send packet" event - used to cancel application
  // how to manage the (on-off) flows, another event is needed ???
  
  uint64_t         m_numGeneratedPkts;     // Total number of generated packets
  uint64_t         m_numPktsToGenerate;    // Total number of packets to generate (input param) 
};

} // namespace ns3

#endif

