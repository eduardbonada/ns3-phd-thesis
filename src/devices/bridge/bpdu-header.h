/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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

#ifndef _BPDU_HEADER_H
#define _BPDU_HEADER_H

#include <stdlib.h>
#include <string>
#include "ns3/header.h"
#include "ns3/mac48-address.h"

#define HELLO_TYPE   100
#define LSP_TYPE     200
#define BPDU_TYPE    300
#define TCN_TYPE     400
#define BPDU_PV_TYPE 500 // BPDU extended with the path vector
#define BPDU_TAP_TYPE 600 // BPDU extended with the SPB promises

// note this is copies to xstp.h, main simulation file and bpdu header.
#define ROOT_ROLE "R"
#define DESIGNATED_ROLE "D"
#define ALTERNATE_ROLE "A"
#define BACKUP_ROLE "B"

namespace ns3 {

class BpduHeader : public Header 
{
public:

  BpduHeader ();
  
  //BPDU
  uint64_t GetRootId (void);
  uint32_t GetCost (void);
  uint64_t GetBridgeId (void);
  uint16_t GetPortId (void);
  uint16_t GetMessAge (void);
  uint16_t GetMaxAge (void);
  uint16_t GetHelloTime (void);
  uint16_t GetForwDelay (void);
  
  void SetRootId (uint64_t r);
  void SetCost (uint32_t c);
  void SetBridgeId (uint64_t b);
  void SetPortId (uint16_t p);
  void SetMessAge (uint16_t p);
  void SetMaxAge (uint16_t p);  
  void SetHelloTime (uint16_t p);
  void SetForwDelay (uint16_t p);
  
  void SetTcFlag (bool);
  bool GetTcFlag ();
  void SetPropFlag (bool);
  bool GetPropFlag ();
  void SetRoleFlags (std::string);
  std::string GetRoleFlags ();
  void SetLrnFlag (bool);
  bool GetLrnFlag ();
  void SetFrwFlag (bool);
  bool GetFrwFlag ();
  void SetAgrFlag (bool);
  bool GetAgrFlag ();
  void SetTcaFlag (bool);
  bool GetTcaFlag ();
  
  uint64_t GetBpduSeq (void);
  void     SetBpduSeq (uint64_t s);

   
  //LSP&HELLO
  uint16_t GetType (void);
  void     SetType (uint16_t t);    
  //uint64_t GetBridgeId (void);
  //void     SetBridgeId (uint64_t n);
  uint32_t GetHelloCost (void);
  void     SetHelloCost (uint32_t c);
  uint64_t GetLspSequence (void);
  void     SetLspSequence (uint64_t s);
  uint64_t GetLspNumAdjacencies (void);
  void     SetLspNumAdjacencies (uint64_t n);
  void     AddAdjacency(uint64_t n, uint32_t c);
  uint64_t GetAdjBridgeId (uint64_t n);
  uint32_t GetAdjCost (uint64_t n);
  
  //TCN
  //no fields
  
  //BPDU extended with PV
  void     SetPathVectorNumBridgeIds (uint16_t n);
  uint16_t GetPathVectorNumBridgeIds ();
  void     AddPathVectorBridgeId(uint64_t b);
  uint64_t GetPathVectorBridgeId (uint16_t i);
  
  //BPDU extended with the TAP promises
  void     SetPN (uint16_t pn);
  uint16_t GetPN ();
  void     SetDPN(uint16_t dpn);
  uint16_t GetDPN ();
  
  //COMMON    
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  
private:
  //COMMON
  uint16_t      m_type;               // hello(100) or lsp(200) or bpdu(300) or bpduv2* (400) (* merged BPDUs)
  
  //BPDU
  //uint16_t m_protIdentifier;
  uint8_t  m_protVersion;
  uint8_t  m_bpduType;
  uint8_t  m_flags;
  uint64_t m_rootId;
  uint32_t m_cost;
  uint64_t m_bridgeId;
  uint16_t m_portId;
  uint16_t m_messAge;
  uint16_t m_maxAge;
  uint16_t m_helloTime;
  uint16_t m_forwDelay;
  uint64_t m_seqNum;

  //BPDU extended with PV
  std::vector<uint64_t> m_pathVectorBridgeIds;     // BPDU with PV: array of bridgeId that the BPDU has traversed
  uint16_t              m_pathVectorNumBridgeIds;  // LSP type: number of adjacencies
    
  //BPDU for SPB
  uint16_t m_pn;                              // promise number
  uint16_t m_dpn;                             // discarded promise number
  
  //LSP&HELLO
  //uint64_t      m_bridgeId;                   // bridgeId of sender  
  uint64_t              m_lspSequence;        // LSP type: sequence number
  uint64_t              m_lspNumAdjacencies;  // LSP type: number of adjacencies
  std::vector<uint64_t> m_lspAdjBridgeId;     // LSP type: array of nodeId adjacencies (same index as adjCost)
  std::vector<uint32_t> m_lspAdjCost;         // LSP type: array of nodeId costs (same index as adjNodeId)  
  uint32_t              m_helloCost;          // HELLO type: cost

};

}; // namespace ns3


#endif /* STP_BPDU_HEADER_H */
