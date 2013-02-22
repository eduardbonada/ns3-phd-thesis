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

#ifndef RSTP_H
#define RSTP_H

#define DISCARDING_STATE "DSC"
#define FORWARDING_STATE "FRW" 
#define LEARNING_STATE "LRN"
#define DISABLED_STATE "DBL"
//#define LISTENING_STATE "LIS"
          
// note this is copies to xstp.h, main simulation file and bpdu header.
#define ROOT_ROLE "R"
#define DESIGNATED_ROLE "D"
#define ALTERNATE_ROLE "A"
#define BACKUP_ROLE "B"

#define HELLO_TYPE   100
#define LSP_TYPE     200
#define BPDU_TYPE    300
#define TCN_TYPE     400
#define BPDU_PV_TYPE 500 // BPDU extended with the path vector

#include "ns3/simulation-god.h"
#include "ns3/bridge-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>

#include "ns3/net-device-container.h"

namespace ns3{

class Node;

class RSTP : public Object
{
public:
  // BPDU INFO
  typedef struct{
    uint16_t protIdentifier;
    uint8_t  protVersion;
    uint8_t  bpduType;
    bool proposal;
    std::string role;
    bool learning;
    bool forwarding;
    bool Agreement;
    bool topChangeAck;
    uint64_t rootId;
    uint32_t cost;
    uint64_t bridgeId;
    uint16_t portId;
    uint16_t messAge;
    uint16_t maxAge;
    uint16_t helloTime;
    uint16_t forwDelay;
  }bpduInfo_t;

  // BRIDGE INFO
  typedef struct{
    uint64_t rootId;
    uint32_t cost;
    uint64_t bridgeId;
    int16_t  rootPort;
    uint16_t maxAge;
    uint16_t messAgeInc;
    uint16_t helloTime;
    uint16_t forwDelay;
    double   txHoldPeriod;
    uint32_t maxBpduTxHoldPeriod;
    uint32_t resetTxHoldPeriod;
    Timer    helloTimeTimer;
    Timer    txHoldCountTimer;
  }bridgeInfo_t;
  
  // PORT INFO
  typedef struct{
    std::string state;
    std::string role;
    uint64_t   designatedRootId;
    uint32_t   designatedCost;
    uint64_t   designatedBridgeId;
    uint16_t   designatedPortId;
    uint16_t   messAge;
    uint16_t   txHoldCount;
    bool       proposed;
    bool       proposing;
    bool       sync;
    bool       synced;
    bool       agreed;    
    bool       newInfo;    
    bpduInfo_t receivedBpdu;
    Timer      toLearningTimer;    
    Timer      toForwardingTimer;
    Timer      recentRootTimer;
    Timer      messAgeTimer;
    Timer      phyLinkFailDetectTimer;
    Time       lastMessAgeSchedule;
    bool       topChange;
    bool       topChangeAck;    
    Timer      topChangeTimer;
  }portInfo_t;

  static TypeId GetTypeId (void);
  
  RSTP();
  virtual ~RSTP();

   /** 
   * \brief Installs the RSTP module in the given node, links it to the given bridge net device, and adds the ports to the RSTP::m_ports vector to be accessed to send BPDU to Designated.
   *
   * \param node Node where the module is installed
   * \param bridge Bridge NetDevice linked to the RSTP
   * \param port Container of the NetDevices (ports) to link to RSTP
   * \param bridgeId Container of the NetDevices (ports) to link to RSTP
   * \param rstpSimulationTimeSecs 
   * \param port rstpActivatePeriodicalBpdus
   * \param port rstpMaxAge
   * \param port rstpHelloTime
   * \param port rstpForwDelay
   * \param port rstpTxHoldPeriod
   * \param port rstpMaxBpduTxHoldPeriod
   */
  void Install (Ptr<Node> node, Ptr<BridgeNetDevice> bridge, NetDeviceContainer ports, uint64_t bridgeId, double initTime, uint16_t rstpSimulationTimeSecs, uint16_t rstpActivatePeriodicalBpdus, uint16_t rstpActivateExpirationMaxAge, uint16_t rstpActivateIncreaseMessAge, uint16_t rstpActivateTopologyChange, uint16_t rstpMaxAge, uint16_t rstpHelloTime, uint16_t rstpForwDelay, double rstpTxHoldPeriod, uint32_t rstpMaxBpduTxHoldPeriod, uint32_t rstpResetTxHoldPeriod, Ptr<SimulationGod> god);
    
   /** 
   * \brief Receives a BPDU from the bridge NetDevice (through the Protocol Handler)
   *
   * Checks if it is better, same or worse, and acts accordingly
   * 
   * \param inPort Input port where the BPDU is recieved
   * \param bpdu packet with the bpdu header
   * \param protocol fpr RSTP, this always shouold be the same protocol (which one???)
   * \param src Address of the sender (it will be the address of the neighbir port). Not used?
   * \param dst Address of the receiver (always 01:80:C2:00:00:00). Used?
   * \param packetType Not sure...
   */
  void ReceiveFromBridge (Ptr<NetDevice> inPort, Ptr<const Packet> bpdu, uint16_t protocol, Address const &src, Address const &dst, NetDevice::PacketType packetType);

  void PrintBridgeInfoSingleLine();
  
  uint64_t GetBridgeId();
  
  void ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port);
    
protected:
  virtual void DoDispose (void);

private:
   /** 
   * \brief Compares the BPDU with the internal port info database and returns whether the BPDU is better, worse or the same
   *
   * Note this port database is not the filtering database of the relay function. 
   *
   * \param bpdu BPDU received
   * \param port port where the BPDU has been received
   * \return int16_t 1 for better, 0 for same, -1 for worse
   */
  int16_t UpdatesInfo(bpduInfo_t bpdu, uint16_t port);

   /** 
   * \brief reconfigures the bridge information, actually it re-selects roles (when the incoming bpdu updates the current stored info)
   * 
   * Lookf for a new root port, looks for new designated, the rest to alternate
   */
  void ReconfigureBridge();

   /** 
   * \brief applies the necessary state transitions including checks for quick transitions to forwarding
   */
  void QuickStateTransitions();

   /** 
   * \brief returns true if all Designateds expired their RecentRoot timers, false otherwise
   * \returns true if all Designateds expired their RecentRoot timers, false otherwise
   */
  bool AllDesignatedsRecentRootExpired();
  
   /** 
   * \brief Sets the variable sync in all designated ports except the passed port
   */  
  void SetSyncToAllRest(uint16_t port);

   /** 
   * \brief returns true if all ports, different than passed as argument, are synced
   */  
  bool AllRestSynced(uint16_t port)  ;

   /** 
   * \brief Sends a BPDU tthe outport in the parameter (directly to the net device)
   *
   * \param outPort Pointer to the output port where to send the BPDU
   * \param debugComment string that is printed out in the log to distinguish reason of sending
   */
  void SendBPDU(Ptr<NetDevice> outPort, std::string debugComment);

   /** 
   * \brief Called everytime BPDUs to Designated ports need to be send
   *
   * Reschedules the periodical sendings every m_helloTime
   */
  void SendHelloTimeBpdu();

   /** 
   * \brief Periodically called every txHoldPeriod. Sends pending BPDUs.
   *
   */  
  void ResetTxHoldCount();
  
   /** 
   * \brief Transitions a port to blocking state (sets state and cancels timers).
   * \param port the port to transition
   */  
  void MakeDiscarding (uint16_t port);
  
   /** 
   * \brief Transitions a port to learning state (sets state, cancels timers and sets timer).
   * \param port the port to transition
   */  
  void MakeLearning (uint16_t port);
  
   /** 
   * \brief Transitions a port to forwarding state (sets state, cancels timers and sets timer).
   * \param port the port to transition
   */ 
  void MakeForwarding (uint16_t port);
  
   /** 
   * \brief Executed when a port expires a BPDU after max age of its reception. Results in deleting portInfo.
   * \param port the port where the BPDU is expired
   */ 
  void MessageAgeExpired (uint16_t port);
  
   /** 
   * \brief Executed when a port expires the recent Root timer.
   * \param port the port where the BPDU is expired
   */ 
  void RecentRootTimerExpired(uint16_t port);
  
   /** 
   * \brief Executed when a physical link failure is detected.
   * \param port the port where the failure is detected
   */   
  void PhyLinkFailureDetected(uint16_t port);
  
  void CheckProcessTcFlags(bool tcaFlag, bool tcFlag, uint16_t port);
  
  void TopChangeTimerExpired(uint16_t port);
  
  Ptr<Node> m_node;
  Ptr<BridgeNetDevice> m_bridge;
  std::vector< Ptr<NetDevice> > m_ports; // Pointers to the 'real' port objects (to call the Send() method)
  
  TracedCallback<Ptr<const Packet> >    m_rstpRcvBpdu; // not used so far...
  
  uint16_t m_simulationtime ;               //time to finish sending BPDUs and scheculing timers 
  uint16_t  m_activatePeriodicalBpdus;      // we can also avoid creating periodical BPDUs, only initial ones
  uint16_t  m_rstpActivateExpirationMaxAge; // we can also avoid expiring max age timers and avoid steady state recalculations
  uint16_t  m_rstpActivateIncreaseMessAge;  // activate the increase of the MessAge BPDU parameter (the message age is increased by 1 at each hop).
  uint16_t  m_rstpActivateTopologyChange;    // activate the topology change mechanisms

  double    m_initTime;                    // to store the time when the bridge is switched on
    
  bridgeInfo_t            m_bridgeInfo;  // Bridge Information used by the RSTP
  std::vector<portInfo_t> m_portInfo;    // Port Information database used by the RSTP

  ////////////////////////////////
  

  
  Ptr<SimulationGod>      m_simGod;
  
};

} //namespace ns3

#endif // RSTP_H
