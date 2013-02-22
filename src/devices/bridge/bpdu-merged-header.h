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

#ifndef _BPDU_MERGED_HEADER_H
#define _BPDU_MERGED_HEADER_H

#include <stdlib.h>
#include <string>
#include "ns3/header.h"
#include "ns3/mac48-address.h"

#define HELLO_TYPE   100
#define LSP_TYPE     200
#define BPDU_TYPE    300
#define TCN_TYPE     400
#define BPDU_PV_TYPE 500 // BPDU extended with the path vector
#define BPDU_PV_MERGED_TYPE 600 // BPDU extended with the path vector
#define BPDU_QUERY_REPLY 900 // BPDU used to query the root about its existence (before using information in an alternate)
#define BPDU_PV_MERGED_CONF_TYPE 800 // BPDU extended with the path vector
#define BPDU_CONF_TYPE 700 // BPDU to confirm root alive

// note this is copies to xstp.h, main simulation file and bpdu header.
#define ROOT_ROLE "R"
#define DESIGNATED_ROLE "D"
#define ALTERNATE_ROLE "A"
#define BACKUP_ROLE "B"

namespace ns3 {

class BpduMergedHeader : public Header 
{
public:

  uint16_t GetType (void);
  void     SetType (uint16_t t);    
  
  BpduMergedHeader ();
  
  //BPDU
  uint64_t GetNumBpdusMerged();
  void     SetNumBpdusMerged(uint64_t numBpdusMerged);
  
  uint64_t GetRootId (uint64_t bpduId);
  uint32_t GetCost (uint64_t bpduId);
  uint64_t GetBridgeId (uint64_t bpduId);
  uint16_t GetPortId (uint64_t bpduId);
  uint16_t GetMessAge (uint64_t bpduId);

  void SetRootId (uint64_t r, uint64_t bpduId);
  void SetCost (uint32_t c, uint64_t bpduId);
  void SetBridgeId (uint64_t b, uint64_t bpduId);
  void SetPortId (uint16_t p, uint64_t bpduId);
  void SetMessAge (uint16_t p, uint64_t bpduId);
 
  void SetTcFlag (bool, uint64_t bpduId);
  bool GetTcFlag (uint64_t bpduId);
  void SetPropFlag (bool, uint64_t bpduId);
  bool GetPropFlag (uint64_t bpduId);
  void SetRoleFlags (std::string, uint64_t bpduId);
  std::string GetRoleFlags (uint64_t bpduId);
  void SetLrnFlag (bool, uint64_t bpduId);
  bool GetLrnFlag (uint64_t bpduId);
  void SetFrwFlag (bool, uint64_t bpduId);
  bool GetFrwFlag (uint64_t bpduId);
  void SetAgrFlag (bool, uint64_t bpduId);
  bool GetAgrFlag (uint64_t bpduId);
  void SetTcaFlag (bool, uint64_t bpduId);
  bool GetTcaFlag (uint64_t bpduId);

  uint16_t GetMaxAge ();
  uint16_t GetHelloTime ();
  uint16_t GetForwDelay (); 
  
  void SetMaxAge (uint16_t p);  
  void SetHelloTime (uint16_t p);
  void SetForwDelay (uint16_t p);

  uint16_t GetNumNeighbors (uint64_t bpduId);
  void     SetNumNeighbors (uint16_t n, uint64_t bpduId);

  // BPDU with confirmation
  uint64_t GetConfRootId (void);
  void SetConfRootId (uint64_t id);  
  uint64_t GetConfNeighId (void);
  void SetConfNeighId (uint64_t id);  
  uint16_t GetConfType (void);
  void     SetConfType (uint16_t t);




  //BPDU extended with forced dissemination flag
  uint16_t GetDissemFlag (uint64_t bInd);
  void     SetDissemFlag (uint16_t f, uint64_t bInd);
  
  //BPDU extended with sequences
  uint64_t GetBpduSeq (void);
  void     SetBpduSeq (uint64_t s);
      
  //BPDU extended with PV
  void     SetPathVectorNumBridgeIds (uint16_t n, uint64_t bpduId);
  uint16_t GetPathVectorNumBridgeIds (uint64_t bpduId);
  void     AddPathVectorBridgeId(uint64_t b, uint64_t bpduId);
  uint64_t GetPathVectorBridgeId (uint16_t i, uint64_t bpduId);

  //Query-Reply
  void SetQueryReply(uint16_t qr);
  uint16_t GetQueryReply (); 
  void SetRootIdQueryReply(uint64_t id);
  uint64_t GetRootIdQueryReply (); 

  //TCN
  //no fields
   
  //COMMON    
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  
private:
  //COMMON
  uint16_t      m_type;               
  
  //BPDU
  uint64_t m_numBpdusMerged;
  uint8_t  m_protVersion;
  uint8_t  m_bpduType;
  
  std::vector<uint64_t> m_rootId;
  std::vector<uint32_t> m_cost;
  std::vector<uint64_t> m_bridgeId;
  std::vector<uint16_t> m_portId;
  std::vector<uint8_t>  m_flags;
  std::vector<uint16_t> m_messAge;
  std::vector<uint16_t>                m_pathVectorNumBridgeIds;
  std::vector< std::vector<uint64_t> > m_pathVectorBridgeIds;
  
  uint16_t m_maxAge;
  uint16_t m_helloTime;
  uint16_t m_forwDelay;

  std::vector<uint16_t> m_dissemFlag;
  uint64_t m_seqNum;

  std::vector<uint16_t> m_numNeighbors;

  //BPDU confirmation
  uint64_t m_confRootId;  
  uint64_t m_confNeighId;
  uint16_t m_confType;


  uint16_t m_queryReply;
  uint64_t m_rootIdQueryReply;
};

}; // namespace ns3


#endif /* STP_BPDU_MERGED_HEADER_H */
