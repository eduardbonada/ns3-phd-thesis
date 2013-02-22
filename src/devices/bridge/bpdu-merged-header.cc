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

#include <iomanip>
#include <iostream>
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/address-utils.h"
#include "bpdu-merged-header.h"

NS_LOG_COMPONENT_DEFINE ("BpduMergedHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BpduMergedHeader);

BpduMergedHeader::BpduMergedHeader ()
{
}

void
BpduMergedHeader::SetType(uint16_t f)
{
  m_type = f;
}
uint16_t
BpduMergedHeader::GetType (void)
{
  return m_type;
}

//////////////////////////
// MERGED BPDU
//////////////////////////
void
BpduMergedHeader::SetNumBpdusMerged(uint64_t numBpdusMerged)
{
  m_numBpdusMerged=numBpdusMerged;
  
  // init vectors lengths
  m_rootId.resize(m_numBpdusMerged);
  m_cost.resize(m_numBpdusMerged);
  m_bridgeId.resize(m_numBpdusMerged);
  m_portId.resize(m_numBpdusMerged);
  m_flags.resize(m_numBpdusMerged);
  m_messAge.resize(m_numBpdusMerged);
  m_pathVectorNumBridgeIds.resize(m_numBpdusMerged);
  m_pathVectorBridgeIds.resize(m_numBpdusMerged);
  m_dissemFlag.resize(m_numBpdusMerged);  
  m_numNeighbors.resize(m_numBpdusMerged);  
  //m_confRootId.resize(m_numBpdusMerged);  
  //m_confNeighId.resize(m_numBpdusMerged);  
  //m_confType.resize(m_numBpdusMerged);  
    
}

uint64_t
BpduMergedHeader::GetNumBpdusMerged()
{
  return m_numBpdusMerged;
}

void
BpduMergedHeader::SetRootId (uint64_t r, uint64_t bInd)
{
  m_rootId.at(bInd) = r;
}
uint64_t
BpduMergedHeader::GetRootId (uint64_t bInd)
{
  return m_rootId.at(bInd);
}

void
BpduMergedHeader::SetCost(uint32_t c, uint64_t bInd)
{
  m_cost.at(bInd) = c;
}

uint32_t
BpduMergedHeader::GetCost (uint64_t bInd)
{
  return m_cost.at(bInd);
}

void
BpduMergedHeader::SetBridgeId (uint64_t b, uint64_t bInd)
{
  m_bridgeId.at(bInd) = b;
}
uint64_t
BpduMergedHeader::GetBridgeId (uint64_t bInd)
{
  return m_bridgeId.at(bInd);
}

void
BpduMergedHeader::SetPortId (uint16_t p, uint64_t bInd)
{
  m_portId.at(bInd) = p;
}
uint16_t
BpduMergedHeader::GetPortId (uint64_t bInd)
{
  return m_portId.at(bInd);
}

void
BpduMergedHeader::SetMessAge(uint16_t a, uint64_t bInd)
{
  m_messAge.at(bInd) = a;
}
uint16_t
BpduMergedHeader::GetMessAge (uint64_t bInd)
{
  return m_messAge.at(bInd);
}

//////////////////////////
// FLAGS
//////////////////////////
void
BpduMergedHeader::SetTcFlag (bool f, uint64_t bInd)
{
  //TC: 0x80 1000 0000
  if(f==true)
    {
      m_flags.at(bInd) |= 0x80;
    }
  else
    {
      m_flags.at(bInd) &= ~0x80;
    }
}
bool
BpduMergedHeader::GetTcFlag (uint64_t bInd)
{
  //TC: 0x80 1000 0000
  bool booleanTmp = ( ( m_flags.at(bInd) & 0x80 )==0x80 ? true : false );
  return booleanTmp;
}

void
BpduMergedHeader::SetPropFlag (bool f, uint64_t bInd)
{
  //Prop: 0x40 0100 0000
  if(f==true)
    {
      m_flags.at(bInd) |= 0x40;
    }
  else
    {
      m_flags.at(bInd) &= ~0x40;
    }
}
bool
BpduMergedHeader::GetPropFlag (uint64_t bInd)
{
  //Prop: 0x40 0100 0000
  bool booleanTmp = ( ( m_flags.at(bInd) & 0x40 )==0x40 ? true : false );
  return booleanTmp;
}

void
BpduMergedHeader::SetLrnFlag (bool f, uint64_t bInd)
{
  //Lrn: 0x08 0000 1000
  if(f==true)
    {
      m_flags.at(bInd) |= 0x08;
    }
  else
    {
      m_flags.at(bInd) &= ~0x08;
    }
}
bool
BpduMergedHeader::GetLrnFlag (uint64_t bInd)
{
  //Lrn: 0x08 0000 1000
  bool booleanTmp = ( ( m_flags.at(bInd) & 0x08 )==0x08 ? true : false );
  return booleanTmp;
}

void
BpduMergedHeader::SetFrwFlag (bool f, uint64_t bInd)
{
  //Lrn: 0x04 0000 0100
  if(f==true)
    {
      m_flags.at(bInd) |= 0x04;
    }
  else
    {
      m_flags.at(bInd) &= ~0x04;
    }
}
bool
BpduMergedHeader::GetFrwFlag (uint64_t bInd)
{
  //Lrn: 0x04 0000 0100
  bool booleanTmp = ( ( m_flags.at(bInd) & 0x04 )==0x04 ? true : false );
  return booleanTmp;
}

void
BpduMergedHeader::SetAgrFlag (bool f, uint64_t bInd)
{
  //Agr: 0x02 0000 0010
  if(f==true)
    {
      m_flags.at(bInd) |= 0x02;
    }
  else
    {
      m_flags.at(bInd) &= ~0x02;
    }
}
bool
BpduMergedHeader::GetAgrFlag (uint64_t bInd)
{
  //Agr: 0x02 0000 0010
  bool booleanTmp = ( ( m_flags.at(bInd) & 0x02 )==0x02 ? true : false );
  return booleanTmp;
}

void
BpduMergedHeader::SetTcaFlag (bool f, uint64_t bInd)
{
  //Tca: 0x01 0000 0001
  if(f==true)
    {
      m_flags.at(bInd) |= 0x01;
    }
  else
    {
      m_flags.at(bInd) &= ~0x01;
    }
}
bool
BpduMergedHeader::GetTcaFlag (uint64_t bInd)
{
  //Tca: 0x01 0000 0001
  bool booleanTmp = ( ( m_flags.at(bInd) & 0x01 )==0x01 ? true : false );
  return booleanTmp;
}

void 
BpduMergedHeader::SetRoleFlags (std::string r, uint64_t bInd)
{
  if( r==ROOT_ROLE )
    {
      // b00
      m_flags.at(bInd) &= ~0x20; // reset bit
      m_flags.at(bInd) &= ~0x10; // reset bit
    }
  else
    {
      if( r==DESIGNATED_ROLE )
        {
          // b01
          m_flags.at(bInd) &= ~0x20; // reset bit
          m_flags.at(bInd) |= 0x10; // set bit           
        }
        else
          {
            if( r==ALTERNATE_ROLE )
              {
                // b10
                m_flags.at(bInd) |= 0x20;  // set bit
                m_flags.at(bInd) &= ~0x10; // reset bit
              }
            else
              {
                if( r==BACKUP_ROLE )
                  {
                    // b11
                    m_flags.at(bInd) |= 0x20;  // set bit
                    m_flags.at(bInd) |= 0x10;  // set bit
                  }
                else
                  {
                    NS_LOG_ERROR("ERROR: trying to set an unknown codifed role in bpdu role flag");
                    exit(1);
                  }
              }
          }
      }
}

std::string
BpduMergedHeader::GetRoleFlags (uint64_t bInd)
{
  if((m_flags.at(bInd) & 0x20)==0x00 && (m_flags.at(bInd) & 0x10)==0x00) // flags: 0x00
    {
      return ROOT_ROLE;
    }
  else
    {
      if((m_flags.at(bInd) & 0x20)==0x00 && (m_flags.at(bInd) & 0x10)==0x10) // flags: 0x10
        {
          return DESIGNATED_ROLE;     
        }
        else
          {
            if((m_flags.at(bInd) & 0x20)==0x20 && (m_flags.at(bInd) & 0x10)==0x00) // flags: 0x20
              {
                return ALTERNATE_ROLE;
              }
            else
              {
                if((m_flags.at(bInd) & 0x20)==0x20 && (m_flags.at(bInd) & 0x10)==0x10) // flags: 0x30
                  {
                    return BACKUP_ROLE;
                  }
                else
                  {
                    NS_LOG_ERROR("ERROR: trying to get an unknown codifed role in bpdu role flag");
                    exit(1);
                  }
              }
          }
      }  
  
}

void
BpduMergedHeader::SetNumNeighbors(uint16_t n,uint64_t bInd)
{
  m_numNeighbors.at(bInd) = n;
}
uint16_t
BpduMergedHeader::GetNumNeighbors(uint64_t bInd)
{
  return m_numNeighbors.at(bInd);
}

//////////////////////////
// CONFIRMATION BPDU
//////////////////////////
void
BpduMergedHeader::SetConfRootId (uint64_t r)
{
  m_confRootId = r;
}
uint64_t
BpduMergedHeader::GetConfRootId (void)
{
  return m_confRootId;
}
void
BpduMergedHeader::SetConfNeighId (uint64_t n)
{
  m_confNeighId = n;
}
uint64_t
BpduMergedHeader::GetConfNeighId (void)
{
  return m_confNeighId;
}
void
BpduMergedHeader::SetConfType (uint16_t t)
{
  m_confType = t;
}
uint16_t
BpduMergedHeader::GetConfType (void)
{
  return m_confType;
}

//////////////////////////
// BPDU with Sequences
//////////////////////////
void
BpduMergedHeader::SetBpduSeq(uint64_t s)
{
  m_seqNum = s;
}
uint64_t
BpduMergedHeader::GetBpduSeq(void)
{
  return m_seqNum;
}

//////////////////////////
// BPDU with forced dissemination flag
//////////////////////////
void
BpduMergedHeader::SetDissemFlag(uint16_t s, uint64_t bInd)
{
  m_dissemFlag.at(bInd) = s;
}
uint16_t
BpduMergedHeader::GetDissemFlag(uint64_t bInd)
{
  return m_dissemFlag.at(bInd);
}


//////////////////////////
// BPDU with PV
//////////////////////////

void
BpduMergedHeader::SetPathVectorNumBridgeIds(uint16_t n, uint64_t bInd)
{
  m_pathVectorNumBridgeIds.at(bInd) = n;
}
uint16_t
BpduMergedHeader::GetPathVectorNumBridgeIds (uint64_t bInd)
{
  return m_pathVectorNumBridgeIds.at(bInd);
}

void
BpduMergedHeader::AddPathVectorBridgeId(uint64_t b, uint64_t bInd)
{
  m_pathVectorBridgeIds.at(bInd).push_back(b);
  if(m_pathVectorBridgeIds.at(bInd).size() > m_pathVectorNumBridgeIds.at(bInd))
    {
      std::cout << "ERROR: AddPathVectorBridgeId() more path vectors elements than expected";
      exit(1);
    }
}
uint64_t
BpduMergedHeader::GetPathVectorBridgeId (uint16_t i, uint64_t bInd)
{
  return m_pathVectorBridgeIds.at(bInd).at(i);
}

//////////////////////////
// COMMON FIELDS
//////////////////////////

void
BpduMergedHeader::SetHelloTime(uint16_t a)
{
  m_helloTime = a;
}

uint16_t
BpduMergedHeader::GetHelloTime (void)
{
  return m_helloTime;
}

void
BpduMergedHeader::SetForwDelay(uint16_t a)
{
  m_forwDelay = a;
}

uint16_t
BpduMergedHeader::GetForwDelay (void)
{
  return m_forwDelay;
}

void
BpduMergedHeader::SetMaxAge(uint16_t a)
{
  m_maxAge = a;
}

uint16_t
BpduMergedHeader::GetMaxAge (void)
{
  return m_maxAge;
}

//////////////////////////
// QUERY REPLY
//////////////////////////

void
BpduMergedHeader::SetQueryReply(uint16_t qr)
{
  m_queryReply = qr;
}
uint16_t
BpduMergedHeader::GetQueryReply (void)
{
  return m_queryReply;
}
void
BpduMergedHeader::SetRootIdQueryReply(uint64_t r)
{
  m_rootIdQueryReply = r;
}

uint64_t
BpduMergedHeader::GetRootIdQueryReply (void)
{
  return m_rootIdQueryReply;
}


//////////////////////////
// INTERNAL SIMULATOR
//////////////////////////

TypeId 
BpduMergedHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BpduMergedHeader")
    .SetParent<Header> ()
    .AddConstructor<BpduMergedHeader> ()
    ;
  return tid;
}

TypeId 
BpduMergedHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
BpduMergedHeader::Print (std::ostream &os) const
{
  /*os << " length/type=0x" << std::hex << m_lengthType << std::dec
     << ", source=" << m_source
     << ", destination=" << m_destination;*/
}
uint32_t 
BpduMergedHeader::GetSerializedSize (void) const
{
  if(m_type==BPDU_PV_MERGED_TYPE)
    {
      uint32_t byteCounter=18 + 8; //fixed fields + sequence
      for(uint64_t i=0 ; i < m_numBpdusMerged ; i++)
        {
          byteCounter = byteCounter + 27 + 2; //priority vector, messAge, numPV, dissemFlag
          for(uint64_t j=0 ; j < m_pathVectorNumBridgeIds.at(i) ; j++)
            {
              byteCounter = byteCounter + 8;
            }
        }
      NS_LOG_LOGIC("GetSerializedSize() => m_type " << m_type << " - size " << byteCounter);
      return byteCounter;
    }
  else if(m_type==BPDU_QUERY_REPLY)
    {
      NS_LOG_LOGIC("GetSerializedSize() => m_type " << m_type << " - size " << 2 + 8 + 2);
      return 2 + 8 + 2;//type + rootId and 0/1/2 (q/r+/r-)
    }
  else if(m_type==BPDU_PV_MERGED_CONF_TYPE)
    {
      uint32_t byteCounter=18 + 8 + 8; //fixed fields + sequence + numNeigh
      for(uint64_t i=0 ; i < m_numBpdusMerged ; i++)
        {
          byteCounter = byteCounter + 27 + 2; //priority vector, messAge, numPV, dissemFlag
          for(uint64_t j=0 ; j < m_pathVectorNumBridgeIds.at(i) ; j++)
            {
              byteCounter = byteCounter + 8;
            }
        }
      NS_LOG_LOGIC("GetSerializedSize() => m_type " << m_type << " - size " << byteCounter);
      return byteCounter;
    }
  else if(m_type==BPDU_CONF_TYPE)
    {
      return 4+8+8+2;
    }
  else
    {
      std::cout << "ERROR: GetSerializedSize() in BpduMergedHeader not matching type";
      exit(1);
    }
}

void
BpduMergedHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  NS_LOG_LOGIC("Serialize() => m_type " << m_type);
  
  if(m_type==BPDU_PV_MERGED_TYPE)
    {
      NS_LOG_LOGIC("Serialize() => m_type " << m_type << " - m_numBpdusMerged " << m_numBpdusMerged);

      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU8  (m_protVersion);
      i.WriteU8  (m_bpduType);
      i.WriteU64 (m_numBpdusMerged);
      
      for(uint64_t k=0 ; k < m_numBpdusMerged ; k++)
        {
          NS_LOG_LOGIC("Serialize() => bpduInd " << k << " - m_rootId " << m_rootId.at(k) << " - m_cost " << m_cost.at(k) << " - m_bridgeId " << m_bridgeId.at(k) << " - m_portId " << m_portId.at(k) << " - m_messAge " << m_messAge.at(k));

          i.WriteU64 (m_rootId.at(k));
          i.WriteU32 (m_cost.at(k));
          i.WriteU64 (m_bridgeId.at(k));
          i.WriteU16 (m_portId.at(k));
          i.WriteU8  (m_flags.at(k));     
          i.WriteU16 (m_messAge.at(k));

          i.WriteU16 (m_pathVectorNumBridgeIds.at(k));
          NS_LOG_LOGIC("Serialize() => m_pathVectorNumBridgeIds " << m_pathVectorNumBridgeIds.at(k));
          for(uint64_t a=0; a<m_pathVectorNumBridgeIds.at(k); a++)
            {
              i.WriteU64 (m_pathVectorBridgeIds.at(k).at(a));
            }   
               
          i.WriteU16 (m_dissemFlag.at(k)); 
        }
        
      i.WriteU16 (m_maxAge);  
      i.WriteU16 (m_helloTime);  
      i.WriteU16 (m_forwDelay); 
      i.WriteU64 (m_seqNum);

      //remember to update GetSerializeSize()    
    }
  else if(m_type==BPDU_PV_MERGED_CONF_TYPE)
    {
      NS_LOG_LOGIC("Serialize() => m_type " << m_type << " - m_numBpdusMerged " << m_numBpdusMerged);

      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU8  (m_protVersion);
      i.WriteU8  (m_bpduType);
      i.WriteU64 (m_numBpdusMerged);
      
      for(uint64_t k=0 ; k < m_numBpdusMerged ; k++)
        {
          NS_LOG_LOGIC("Serialize() => bpduInd " << k << " - m_rootId " << m_rootId.at(k) << " - m_cost " << m_cost.at(k) << " - m_bridgeId " << m_bridgeId.at(k) << " - m_portId " << m_portId.at(k) << " - m_messAge " << m_messAge.at(k));

          i.WriteU64 (m_rootId.at(k));
          i.WriteU32 (m_cost.at(k));
          i.WriteU64 (m_bridgeId.at(k));
          i.WriteU16 (m_portId.at(k));
          i.WriteU8  (m_flags.at(k));     
          i.WriteU16 (m_messAge.at(k));

          i.WriteU16 (m_pathVectorNumBridgeIds.at(k));
          NS_LOG_LOGIC("Serialize() => m_pathVectorNumBridgeIds " << m_pathVectorNumBridgeIds.at(k));
          for(uint64_t a=0; a<m_pathVectorNumBridgeIds.at(k); a++)
            {
              i.WriteU64 (m_pathVectorBridgeIds.at(k).at(a));
            }   
               
          i.WriteU16 (m_dissemFlag.at(k)); 
          i.WriteU16 (m_numNeighbors.at(k));
        }
        
      i.WriteU16 (m_maxAge);  
      i.WriteU16 (m_helloTime);  
      i.WriteU16 (m_forwDelay); 
      i.WriteU64 (m_seqNum);

      //remember to update GetSerializeSize()    
    }
  else if(m_type==BPDU_QUERY_REPLY)
    {
      NS_LOG_LOGIC("Serialize() => m_type " << m_type << " (query-reply)");
      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU64 (m_rootIdQueryReply);
      i.WriteU16 (m_queryReply);
    }
  else if(m_type==BPDU_CONF_TYPE)
    {
      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU8  (m_protVersion);
      i.WriteU8  (m_bpduType);
      i.WriteU64 (m_confRootId);
      i.WriteU64 (m_confNeighId);
      i.WriteU16 (m_confType);  
      //remember to update GetSerializeSize()    
    }
  else
    {
      std::cout << "ERROR: Serialize() in BpduMergedHeader not matching type";
      exit(1);
    }
}

uint32_t
BpduMergedHeader::Deserialize (Buffer::Iterator start)
{

  Buffer::Iterator i = start;

  m_type           = i.ReadU16 ();

  NS_LOG_LOGIC("Deserialize() => m_type " << m_type);
            
  if(m_type==BPDU_PV_MERGED_TYPE)
    {
      m_protVersion    = i.ReadU8 ();
      m_bpduType       = i.ReadU8 ();
      m_numBpdusMerged = i.ReadU64 ();

      NS_LOG_LOGIC("Deserialize() => m_type " << m_type << " - m_numBpdusMerged " << m_numBpdusMerged);
      
      for(uint64_t k=0 ; k < m_numBpdusMerged ; k++)
        {            
          m_rootId.push_back(  i.ReadU64 ());
          m_cost.push_back(    i.ReadU32 ());
          m_bridgeId.push_back(i.ReadU64 ());
          m_portId.push_back(  i.ReadU16 ());
          m_flags.push_back(   i.ReadU8  ());    
          m_messAge.push_back( i.ReadU16 ());

          NS_LOG_LOGIC("Deserialize() => bpduInd " << k << " - m_rootId " << m_rootId.at(k) << " - m_cost " << m_cost.at(k) << " - m_bridgeId " << m_bridgeId.at(k) << " - m_portId " << m_portId.at(k) << " - m_messAge " << m_messAge.at(k));
          
          m_pathVectorNumBridgeIds.push_back(i.ReadU16 ());
          NS_LOG_LOGIC("Deserialize() => m_pathVectorNumBridgeIds " << m_pathVectorNumBridgeIds.at(k));
          std::vector<uint64_t> tmpArray;
          for(uint64_t a=0; a<m_pathVectorNumBridgeIds.at(k); a++)
            {
              tmpArray.push_back(i.ReadU64());
            }
          m_pathVectorBridgeIds.push_back(tmpArray);
          
          m_dissemFlag.push_back( i.ReadU16 ());
        }

      m_maxAge         = i.ReadU16 ();
      m_helloTime      = i.ReadU16 ();
      m_forwDelay      = i.ReadU16 ();

      m_seqNum         = i.ReadU64 ();
      
      //remember to update GetSerializeSize()      
    }
  else if(m_type==BPDU_PV_MERGED_CONF_TYPE)
    {
      m_protVersion    = i.ReadU8 ();
      m_bpduType       = i.ReadU8 ();
      m_numBpdusMerged = i.ReadU64 ();

      NS_LOG_LOGIC("Deserialize() => m_type " << m_type << " - m_numBpdusMerged " << m_numBpdusMerged);
      
      for(uint64_t k=0 ; k < m_numBpdusMerged ; k++)
        {            
          m_rootId.push_back(  i.ReadU64 ());
          m_cost.push_back(    i.ReadU32 ());
          m_bridgeId.push_back(i.ReadU64 ());
          m_portId.push_back(  i.ReadU16 ());
          m_flags.push_back(   i.ReadU8  ());    
          m_messAge.push_back( i.ReadU16 ());

          NS_LOG_LOGIC("Deserialize() => bpduInd " << k << " - m_rootId " << m_rootId.at(k) << " - m_cost " << m_cost.at(k) << " - m_bridgeId " << m_bridgeId.at(k) << " - m_portId " << m_portId.at(k) << " - m_messAge " << m_messAge.at(k));
          
          m_pathVectorNumBridgeIds.push_back(i.ReadU16 ());
          NS_LOG_LOGIC("Deserialize() => m_pathVectorNumBridgeIds " << m_pathVectorNumBridgeIds.at(k));
          std::vector<uint64_t> tmpArray;
          for(uint64_t a=0; a<m_pathVectorNumBridgeIds.at(k); a++)
            {
              tmpArray.push_back(i.ReadU64());
            }
          m_pathVectorBridgeIds.push_back(tmpArray);
          
          m_dissemFlag.push_back( i.ReadU16 ());

          m_numNeighbors.push_back( i.ReadU16 ());
        }

      m_maxAge         = i.ReadU16 ();
      m_helloTime      = i.ReadU16 ();
      m_forwDelay      = i.ReadU16 ();

      m_seqNum         = i.ReadU64 ();
            
      //remember to update GetSerializeSize()      
    }

  else if(m_type==BPDU_QUERY_REPLY)
    {
      NS_LOG_LOGIC("Deserialize() => m_type " << m_type << " (query-reply)");
      m_rootIdQueryReply = i.ReadU64 ();
      m_queryReply       = i.ReadU16 ();
    }
  else if(m_type==BPDU_CONF_TYPE)
    {
      m_protVersion    = i.ReadU8 ();
      m_bpduType       = i.ReadU8 ();
      m_confRootId     = i.ReadU64 ();
      m_confNeighId    = i.ReadU64 ();
      m_confType       = i.ReadU16 ();
      //remember to update GetSerializeSize()      
    }
  else
    {
      std::cout << "ERROR: Deserialize() not matching type";
      exit(1);
    }
    
  return GetSerializedSize ();
}

} // namespace ns3
