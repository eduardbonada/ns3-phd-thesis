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

#ifndef SPBDV_H
#define SPBDV_H

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
#define BPDU_PV_MERGED_TYPE 600 // BPDU extended with the path vector

#include "ns3/simulation-god.h"
#include "ns3/bridge-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "bpdu-merged-header.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>

#include "ns3/net-device-container.h"

namespace ns3{

class Node;

class SPBDV : public Object
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
    std::vector<uint64_t> rootId;
    std::vector<uint32_t> cost;
    std::vector<int16_t>  rootPort;
    uint64_t bridgeId;
    std::vector<bool> activeTree;
    uint16_t maxAge;
    uint16_t messAgeInc;
    uint16_t helloTime;
    uint16_t forwDelay;
    double   txHoldPeriod;
    uint32_t maxBpduTxHoldPeriod;
    uint32_t resetTxHoldPeriod;
    Timer    helloTimeTimer;
    Timer    txHoldCountTimer;
    std::vector<std::vector < uint64_t > >   pathVector;
  }bridgeInfo_t;
  
  // PORT INFO
  typedef struct{
    std::vector<std::string> state;
    std::vector<std::string> role;
    std::vector<uint64_t>    designatedRootId;
    std::vector<uint32_t>    designatedCost;
    std::vector<uint64_t>    designatedBridgeId;
    std::vector<uint16_t>    designatedPortId;
    std::vector<uint16_t>    messAge;
    uint16_t   txHoldCount;
    std::vector<bool>       proposed;
    std::vector<bool>       proposing;
    std::vector<bool>       sync;
    std::vector<bool>       synced;
    std::vector<bool>       agreed;    
    std::vector<bool>       newInfo;    
    std::vector<bpduInfo_t> receivedBpdu;
    std::vector<Timer>      toLearningTimer;    
    std::vector<Timer>      toForwardingTimer;
    std::vector<Timer>      recentRootTimer;
    std::vector<Timer>      messAgeTimer;
    std::vector<Time>       lastMessAgeSchedule;
    std::vector<bool>       atLeastOneHold;
    Timer      phyLinkFailDetectTimer;
    bool       topChange;
    bool       topChangeAck;    
    Timer      topChangeTimer;
    std::vector<std::vector < uint64_t > >   pathVector;
  }portInfo_t;

  static TypeId GetTypeId (void);
  
  SPBDV();
  virtual ~SPBDV();

   /** 
   * \brief Installs the SPBDV module in the given node, links it to the given bridge net device, and adds the ports to the SPBDV::m_ports vector to be accessed to send BPDU to Designated.
   *
   * \param node Node where the module is installed
   * \param bridge Bridge NetDevice linked to the SPBDV
   * \param port Container of the NetDevices (ports) to link to SPBDV
   * \param bridgeId Container of the NetDevices (ports) to link to SPBDV
   * \param spbdvSimulationTimeSecs 
   * \param port spbdvActivatePeriodicalBpdus
   * \param port spbdvMaxAge
   * \param port spbdvHelloTime
   * \param port spbdvForwDelay
   * \param port spbdvTxHoldPeriod
   * \param port spbdvMaxBpduTxHoldPeriod
   */
  void Install (Ptr<Node> node, Ptr<BridgeNetDevice> bridge, NetDeviceContainer ports, uint64_t bridgeId, double initTime, uint16_t spbdvSimulationTimeSecs, uint16_t spbdvActivatePeriodicalBpdus, uint16_t spbdvActivateExpirationMaxAge, uint16_t spbdvActivateIncreaseMessAge, uint16_t spbdvActivateTopologyChange, uint16_t spbdvActivatePathVectorDuplDetect, uint16_t spbdvactivatePeriodicalStpStyle, uint16_t spbdvActivateBpduMerging, uint16_t spbdvMaxAge, uint16_t spbdvHelloTime, uint16_t spbdvForwDelay, double spbdvTxHoldPeriod, uint32_t spbdvMaxBpduTxHoldPeriod, uint32_t spbdvResetTxHoldPeriod, uint64_t numNodes, Ptr<SimulationGod> god);
    
   /** 
   * \brief Receives a BPDU from the bridge NetDevice (through the Protocol Handler)
   *
   * Checks if it is better, same or worse, and acts accordingly
   * 
   * \param inPort Input port where the BPDU is recieved
   * \param bpdu packet with the bpdu header
   * \param protocol fpr SPBDV, this always shouold be the same protocol (which one???)
   * \param src Address of the sender (it will be the address of the neighbir port). Not used?
   * \param dst Address of the receiver (always 01:80:C2:00:00:00). Used?
   * \param packetType Not sure...
   */
  void ReceiveFromBridge (Ptr<NetDevice> inPort, Ptr<const Packet> bpdu, uint16_t protocol, Address const &src, Address const &dst, NetDevice::PacketType packetType);

  void PrintBridgeInfoSingleLine(uint64_t tree);
  
  uint64_t GetBridgeId();
  
  void ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port);
    
protected:
  virtual void DoDispose (void);

private:

  int16_t UpdatesInfo(BpduMergedHeader bpduHdr, uint16_t port, uint64_t tree, uint64_t bInd);

  int16_t BetterPathVector(bool bpduRcvd, BpduMergedHeader bpduHdr, uint64_t bInd, uint16_t portA, uint16_t portB, uint64_t tree);
  
  void ReconfigureBridge(uint64_t tree);
  void QuickStateTransitions(uint64_t tree);

  bool AllDesignatedsRecentRootExpired(uint64_t tree);
  void SetSyncToAllRest(uint16_t port, uint64_t tree);
  bool AllRestSynced(uint16_t port, uint64_t tree)  ;

  void SendBPDU(Ptr<NetDevice> outPort, std::string debugComment, uint64_t tree);
  void SendHelloTimeBpdu();

  void MakeDiscarding (uint16_t port, uint64_t tree);
  void MakeLearning (uint16_t port, uint64_t tree);
  void MakeForwarding (uint16_t port, uint64_t tree);

  void MessageAgeExpired (uint16_t port, uint64_t tree);
  void RecentRootTimerExpired(uint16_t port, uint64_t tree);
  void PhyLinkFailureDetected(uint16_t port);

  void ResetTxHoldCount();
  
  void CheckProcessTcFlags(bool tcaFlag, bool tcFlag, uint16_t port, string role);
  
  void TopChangeTimerExpired(uint16_t port);

  void PrintBpdu(BpduMergedHeader bpdu);
  
  Ptr<Node> m_node;
  Ptr<BridgeNetDevice> m_bridge;
  std::vector< Ptr<NetDevice> > m_ports; // Pointers to the 'real' port objects (to call the Send() method)
  
  TracedCallback<Ptr<const Packet> >    m_spbdvRcvBpdu; // not used so far...
  
  uint16_t m_simulationtime ;                     //time to finish sending BPDUs and scheculing timers 
  
  uint16_t  m_activatePeriodicalBpdus;            // we can also avoid creating periodical BPDUs, only initial ones
  uint16_t  m_spbdvActivateExpirationMaxAge;      // we can also avoid expiring max age timers and avoid steady state recalculations
  uint16_t  m_spbdvActivateIncreaseMessAge;       // activate the increase of the MessAge BPDU parameter (the message age is increased by 1 at each hop).
  uint16_t  m_spbdvActivateTopologyChange;        // activate the topology change mechanisms
  uint16_t  m_spbdvActivatePathVectorDuplDetect;  // activates the detection of duplicated elements in the path vector (in order to avoid looping BPDUs)
  uint16_t  m_spbdvActivatePeriodicalStpStyle;    // only the root sends periodical messages if true (all nodes if false)   
  uint16_t  m_spbdvActivateBpduMerging;
  double    m_initTime;                           // to store the time when the bridge is switched on
    
  bridgeInfo_t            m_bridgeInfo;  // Bridge Information used by the SPBDV
  std::vector<portInfo_t> m_portInfo;    // Port Information database used by the SPBDV
  
  Ptr<SimulationGod>      m_simGod;

  uint64_t  m_numNodes;  
};

} //namespace ns3

#endif // SPBDV_H
