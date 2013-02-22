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

#ifndef ETH_POINT_TO_POINT_CHANNEL_H
#define ETH_POINT_TO_POINT_CHANNEL_H

#include <list>
#include "ns3/channel.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/timer.h"
#include "ns3/simulation-god.h"

namespace ns3 {

class EthPointToPointNetDevice;
class Packet;

/**
 * \brief Simple Point To Ethernet Point Channel.
 *
 * There are two "wires" in the channel.  The first device connected gets the
 * [0] wire to transmit on.  The second device gets the [1] wire.  There is a
 * state (IDLE, TRANSMITTING) associated with each wire.
 */
class EthPointToPointChannel : public Channel 
{
public:
  static TypeId GetTypeId (void);

  /**
   * \brief Create a EthPointToPointChannel
   *
   * By default, you get a channel that
   * has zero transmission delay.
   */
  EthPointToPointChannel ();

  /**
   * \brief Attach a given netdevice to this channel
   * \param device pointer to the netdevice to attach to the channel
   */
  void Attach (Ptr<EthPointToPointNetDevice> device);

  /**
   * \brief Transmit a packet over this channel
   * \param p Packet to transmit
   * \param src Source EthPointToPointNetDevice
   * \param txTime Transmit time to apply
   * \returns true if successful (currently always true)
   */
  bool TransmitStart (Ptr<Packet> p, Ptr<EthPointToPointNetDevice> src, Time txTime);
  
  /**
   * \brief Executes the link failure by setting a flag (called after the timer)
   */
  void ExecuteLinkFailure();
 
  /**
   * \brief Sets and schedules the failure timer
   * \param linkFailureTimeUs Time of the link failure in us
   */
  void SetLinkFailure(Time linkFailureTimeUs);

  /**
   * \brief Get number of devices on this channel
   * \returns number of devices on this channel
   */
  virtual uint32_t GetNDevices (void) const;

  /*
   * \brief Get EthPointToPointNetDevice corresponding to index i on this channel
   * \param i Index number of the device requested
   * \returns Ptr to EthPointToPointNetDevice requested
   */
  Ptr<EthPointToPointNetDevice> GetPointToPointDevice (uint32_t i) const;

  /*
   * \brief Get NetDevice corresponding to index i on this channel
   * \param i Index number of the device requested
   * \returns Ptr to NetDevice requested
   */
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;
  
  void ScheduleLinkFailure(Time linkFailureTimeUs);

  void SetSimulationGod(Ptr<SimulationGod> simGod);

  void AlternativeDoDispose (void);

protected:
  //virtual void DoDispose (void);

  
private:
  // Each point to point link has exactly two net devices
  static const int N_DEVICES = 2;

  Time          m_delay;
  int32_t       m_nDevices;
  
  uint8_t m_activateFailure;
  Time    m_failureTimeUs;
  Timer   m_executeLinkFailureTimer;
  bool    m_failedLink;
  
  Ptr<SimulationGod>      m_simGod;
  
  /**
   * The trace source for the packet transmission animation events that the 
   * device can fire.
   * Arguments to the callback are the packet, transmitting
   * net device, receiving net device, transmittion time and 
   * packet receipt time.
   *
   * \see class CallBackTraceSource
   */
  TracedCallback<Ptr<const Packet>, // Packet being transmitted
                 Ptr<NetDevice>,    // Transmitting NetDevice
                 Ptr<NetDevice>,    // Receiving NetDevice
                 Time,              // Amount of time to transmit the pkt
                 Time               // Last bit receive time (relative to now)
                 > m_txrxPointToPoint;

  enum WireState
    {
      INITIALIZING,
      IDLE,
      TRANSMITTING,
      PROPAGATING
    };

  class Link
  {
  public:
    Link() : m_state (INITIALIZING), m_src (0), m_dst (0) {}
    WireState                  m_state;
    Ptr<EthPointToPointNetDevice> m_src;
    Ptr<EthPointToPointNetDevice> m_dst;
  };
    
  Link    m_link[N_DEVICES];
};

} // namespace ns3

#endif /* ETH_POINT_TO_POINT_CHANNEL_H */
