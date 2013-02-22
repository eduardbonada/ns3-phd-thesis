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

#ifndef SIMULATION_GOD_H
#define SIMULATION_GOD_H

#define DISCARDING_STATE "DSC"
#define FORWARDING_STATE "FRW" 
#define LEARNING_STATE "LRN"
#define DISABLED_STATE "DBL"

#include "ns3/bridge-net-device.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"
#include "ns3/traced-callback.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/net-device-container.h"

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>

namespace ns3{

class Node;

class STP;
class RSTP;
class SPBISIS;

class SimulationGod : public Object
{
/*
  This object contains the functions that include checks and others thyat relate several objects.
  For example, a function that checks the active ports of each xstp object and computes if loops exist
*/
public:

  static TypeId GetTypeId (void);

  SimulationGod();
  virtual ~SimulationGod();

  void SetStpMode(uint16_t m);
  void SetNumNodes(uint64_t n);
  void SetPortConnection(uint64_t nodeA, uint64_t nodeB, uint16_t port);
  void SetVertexAdjPhyMx(std::vector < std::vector < int > > *mx);
  
  void AddSTPPointer(Ptr<STP> p);
  void AddRSTPPointer(Ptr<RSTP> p);
  void AddSPBISISPointer(Ptr<SPBISIS> p);

  void SetActivePort(uint64_t tree, uint64_t node, uint16_t port, bool f);
  void SetFailedLink(uint64_t nodeA,uint64_t nodeB);
  
  void SetNodeIdBridgeIdMap(uint64_t, uint64_t);
  
  int16_t CheckLoopsTreeMx(uint64_t tree);
  bool    CheckSymmetry();
  
  void SetSpbIsisTopVertAdjMx(uint64_t node , std::vector < std::vector < uint32_t > > *mx);  
  bool CheckSpbIsIsAllSameVertAdjMx();
  bool CheckSpbIsIsSameVertAdjMx(uint64_t bridgeA, uint64_t bridgeB);
  
  void PrintSimInfo();
  void PrintPhyMx();
  void PrintPortMx();
  void PrintTreeMx(uint64_t tree);
  void PrintMx(std::vector < std::vector < int > > mx);
  
  void PrintBridgesInfo();
  
  
protected:
  virtual void DoDispose (void);

private:  

  uint64_t m_numNodes;

  uint16_t m_stpMode;  
  std::vector < Ptr < STP > >  m_stps;
  std::vector < Ptr < RSTP > > m_rstps;
  std::vector < Ptr < SPBISIS > >  m_spbisiss;
  
  // TOPOLOGY RELATED VARIABLES
  std::vector < std::vector < int > > m_vertexAdjPhyMx;  //vertex adjacency matrix of the physical topology  
  std::vector < std::vector < uint16_t > > m_portMx;     //matrix where (i,j) is the port connecting i=>j 
  std::vector < std::vector < int > > m_failedMx;        //matrix with 1s in those failed links. [i,j]=1 (and [j,i]=1) if link between nodes i,j failed
  uint64_t m_cumulatedLinkFailures;                      //just a counter to notify the number of failures (actually whether there is failure or not)
  std::vector < std::vector < std::vector < int > > > m_vertexAdjTree;   // array of vertex adjacency matrix of the trees (transient and final tree) - one tree per node, or tree o for single trees
  std::vector <bool> m_failedNodes;
  std::vector <bool> m_treeIsSpanning;
  
  std::vector <uint64_t> m_nodeId2bridgeId;
  std::vector <uint64_t> m_bridgeId2nodeId;
  
  std::vector <bool>     m_lastTopologyWithLoop;                       // whether the last tree checking detected a loop (per tree)
  
  // SPB RELATED VARIABLES
  std::vector< std::vector< std::vector<uint32_t> > > m_spbIsisTopVertAdjMx; // array of topology vertex adjacency matrices (one per node), to externally check if every node has the same topology view, and hence it is time to go forwarding

};

} //namespace ns3

#endif // RSTP_H
