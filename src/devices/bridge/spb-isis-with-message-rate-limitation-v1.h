/*
This version contains the LSP rate limitation considering a single queue of LSPs per port.
The LSPs are queued as they are generated or disseminated.
The queue may contain two instances of the same LSP...
*/






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

#ifndef SPB_ISIS_H
#define SPB_ISIS_H

#define DISCARDING_STATE "DSC"
#define FORWARDING_STATE "FRW" 
#define DISABLED_STATE "DBL"

#define ROOT_ROLE "R"
#define DESIGNATED_ROLE "D"
#define ALTERNATE_ROLE "A"

#define HELLO_TYPE   100
#define LSP_TYPE     200
#define BPDU_TYPE    300
#define TCN_TYPE     400
#define BPDU_PV_TYPE 500 // BPDU extended with the path vector
#define BPDU_TAP_TYPE 600 // BPDU extended with the SPB promises

#include "ns3/simulation-god.h"
#include "ns3/bridge-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/queue.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"

namespace ns3{

class Node;
class Queue;

class SPBISIS : public Object
{
public:

  // LSP INFO
  typedef struct{
     uint64_t         bridgeId;     // origin node
     uint32_t         seq;        // sequence number
     std::vector<uint64_t> adjBridgeId;  // node id of the adjacent node - same index as adjCost
     std::vector<uint32_t> adjCost;    // cost of the link to the adjacent node - same index as adjNodeId
  }lspInfo_t;
    
  // BPDU INFO
  typedef struct{
     //uint16_t   protIdentifier;
     //uint8_t    protVersion;
     //uint8_t    bpduType;
     int64_t     rootId;
     int32_t     cost;
     int64_t     bridgeId;
     //uint8_t      flags;
     //uint16_t   portId;
     //uint16_t   messAge;
     //uint16_t   maxAge;
     //uint16_t   helloTime;
     //uint16_t   forwDelay;
  }bpduInfo_t;

  // BRIDGE INFO
  typedef struct{
    std::vector<uint64_t>  rootId;      // each index refers to a different tree
    std::vector<uint32_t>  cost;        // " "
    std::vector<int16_t>   rootPort;    // " "
    uint64_t               bridgeId;
    
    std::vector< std::vector<uint32_t> >                topVertAdjMx;   // matrix containing the topology vertex adjacency matrix (lenght:m_numNodes x m_numNodes)
    std::vector< std::vector<uint32_t> >                topCostMx;      // matrix containing the topology cost matrix (lenght:m_numNodes x m_numNodes)
    std::vector<uint32_t>                               topPorts;       // array containing the local port connected to neighbors (lenght:m_numNodes)
    std::vector< std::vector< std::vector<uint32_t> > > treeVertAdjMx;  // matrix containing the vertex adjacency matrix of the trees (lenght:m_numNodes x m_numNodes x m_numNodes)    

    std::vector<lspInfo_t>                              lspDB;          // array to store the received LSPs for each node (lenght:m_numNodes)

    //uint16_t maxAge;
    //uint16_t messAgeInc;
    //uint16_t helloTime;
    //uint16_t forwDelay;
    //double   txHoldPeriod;
    //uint32_t maxBpduTxHoldPeriod;
    Timer    helloTimer;
    //Timer    txHoldCountTimer;

  }bridgeInfo_t;

  // PROMISE TYPE
  typedef struct{
    bool active;        // wether the promise is active (really held or really outstanding)   
    bool applicable;    // wether the promise is applicable
    uint64_t rootId;    // promise priority vector
    uint32_t cost;      //       "        "
    uint64_t bridgeId;  //       "        "
    uint16_t portId;    //       "        "
    uint16_t rcvdPN;    // received promise number
  }promise_t;
    
  // PORT INFO
  typedef struct{
    std::vector<std::string>  state;                   // each index refers to a different tree
    std::vector<std::string>  role;                    // " "
    std::vector<uint64_t>     designatedRootId;        // " "
    std::vector<uint32_t>     designatedCost;          // " "
    std::vector<uint64_t>     designatedBridgeId;      // " "
    std::vector<uint64_t>     neighborRootId;          // " "
    std::vector<uint32_t>     neighborCost;            // " "
    std::vector<uint64_t>     neighborBridgeId;        // " "
    Timer                     phyLinkFailDetectTimer;
    
    std::vector<uint16_t>     pn;                      // contract/agreement stuff
    std::vector<uint16_t>     dpn;
    std::vector<promise_t>    contractHeld;
    std::vector<promise_t>    contractOutstanding;
    std::vector<promise_t>    agreementHeld;    
    std::vector<promise_t>    agreementOutstanding;

    Timer                     lspTransmissionInterval;
  }portInfo_t;

  static TypeId GetTypeId (void);
  
  SPBISIS();
  virtual ~SPBISIS();

   /** 
   * \brief Installs the SPBISIS module in the given node, links it to the given bridge net device, and adds the ports to the SPBISIS::m_ports vector to be accessed to send BPDU to Designated.
   *
   * \param node Node where the module is installed
   * \param bridge Bridge NetDevice linked to the SPBISIS
   * \param port Container of the NetDevices (ports) to link to SPBISIS
   * \param bridgeId Container of the NetDevices (ports) to link to SPBISIS
   * \param spbSimulationTimeSecs 
   */
  void Install (Ptr<Node> node, Ptr<BridgeNetDevice> bridge, NetDeviceContainer ports, uint64_t bridgeId, double initTime, uint16_t spbSimulationTimeSecs, uint64_t numNodes, Ptr<SimulationGod> god, uint16_t spbisisActivateBpduForwarding, double lspTransmissionInterval );
    
   /** 
   * \brief Receives a BPDU from the bridge NetDevice (through the Protocol Handler)
   *
   * Checks if it is better, same or worse, and acts accordingly
   * 
   * \param inPort Input port where the BPDU is recieved
   * \param bpdu packet with the bpdu header
   * \param protocol for SPBISIS, this always shouold be the same protocol (which one???)
   * \param src Address of the sender (it will be the address of the neighbir port). Not used?
   * \param dst Address of the receiver (always 01:80:C2:00:00:00). Used?
   * \param packetType Not sure...
   */
  void ReceiveFromDevice (Ptr<NetDevice> inPort, Ptr<const Packet> bpdu, uint16_t protocol, Address const &src, Address const &dst, NetDevice::PacketType packetType);

  void ScheduleLinkFailure(Time linkFailureTimeUs, uint16_t port);
  
  void SetPortsToForwarding(bool frw); // set all Root and Designated ports to forwarding (sim God says so)

  void PrintTopCostMx();
  void PrintTopVertAdjMx();
  void PrintTreeVertAdjMx(uint64_t t);
  void PrintLspDB();
  void PrintBridgeInfo();
  void PrintBridgeInfoSingleLine();
  void PrintBridgePromises();
    
protected:
  virtual void DoDispose (void);

private:

   /** 
   * \brief Sends a LSP the outport in the parameter (directly to the net device)
   *
   * \param outPort Pointer to the output port where to send the LSP
   * \param debugComment string that is printed out in the log to distinguish reason of sending
   */
  void SendLSP(uint16_t outPort);

   /** 
   * \brief Sends a hello to all ports
   */
  void SendHello();
  
   /** 
   * \brief Sends a BPDU to the given port
   */  
  void SendBPDU(uint16_t outPort, uint64_t tree);

   /** 
   * \brief Computes Dijkstra and sends BPDUs if necessary
   */    
  void ReconfigureBridge ();

   /** 
   * \brief Computes the tree rooted at the given node
   */    
  void ComputeTree(uint64_t);  

   /** 
   * \brief Computes the tree rooted at the given node
   */
  void UpdatePromisesAfterLS(uint64_t);

  int ComparePromises (promise_t, promise_t);

  void UpdateApplicability (uint16_t, uint64_t);  
  bool CheckToMakePortForwarding (uint16_t, uint64_t);
  bool CheckToSendTapMessage (uint16_t, uint64_t);

    
  /**
    - Transitions the given port to forwarding state
  */
  void MakeForwarding(uint16_t p, uint64_t t);  

  /**
    - Transitions the given port to discarding state
  */
  void MakeDiscarding(uint16_t p, uint64_t t);  

   /** 
   * \brief Executed when a physical link failure is detected.
   * \param port the port where the failure is detected
   */   
  void PhyLinkFailureDetected(uint16_t port);
   
  void TransmitLspAfterInterval(uint16_t port);
         
  Ptr<Node>                     m_node;
  Ptr<BridgeNetDevice>          m_bridgePort;
  std::vector< Ptr<NetDevice> > m_ports;    // Pointers to the 'real' port objects (to call the Send() method)
   
  bridgeInfo_t                  m_bridgeInfo;     // Bridge Information used by the SPBISIS
  std::vector<portInfo_t>       m_portInfo;       // Port Information database used by the SPBISIS

  uint64_t  m_numNodes;
  
  uint16_t  m_simulationtime;               // time to finish sending BPDUs and scheculing timer
  Ptr<SimulationGod>      m_simGod;
  
  uint16_t m_activateBpduForwarding;

  std::vector< Ptr<Queue> >     m_portQueues;    // queues used to buffer LSPs (message rate limitation)
  double   m_lspTransmissionInterval;
  bool     m_activateLspSubstitution; //look into the LSP Queue looking for an LSP from same node and substitute the old by the new freshly received
  
};

} //namespace ns3

#endif // SPBISIS_H
