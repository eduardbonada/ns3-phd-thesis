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
#include "bpdu-header.h"

NS_LOG_COMPONENT_DEFINE ("BpduHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BpduHeader);

BpduHeader::BpduHeader ()
{
  m_lspNumAdjacencies=0;
  m_pathVectorNumBridgeIds=0;
}

//////////////////////////
// BPDU
//////////////////////////
void
BpduHeader::SetRootId (uint64_t r)
{
  m_rootId = r;
}
uint64_t
BpduHeader::GetRootId (void)
{
  return m_rootId;
}

void
BpduHeader::SetCost(uint32_t c)
{
  m_cost = c;
}

uint32_t
BpduHeader::GetCost (void)
{
  return m_cost;
}

void
BpduHeader::SetBridgeId (uint64_t b)
{
  m_bridgeId = b;
}
uint64_t
BpduHeader::GetBridgeId (void)
{
  return m_bridgeId;
}

void
BpduHeader::SetPortId (uint16_t p)
{
  m_portId = p;
}
uint16_t
BpduHeader::GetPortId (void)
{
  return m_portId;
}

void
BpduHeader::SetMessAge(uint16_t a)
{
  m_messAge = a;
}
uint16_t
BpduHeader::GetMessAge (void)
{
  return m_messAge;
}

void
BpduHeader::SetMaxAge(uint16_t a)
{
  m_maxAge = a;
}
uint16_t
BpduHeader::GetMaxAge (void)
{
  return m_maxAge;
}

void
BpduHeader::SetHelloTime(uint16_t a)
{
  m_helloTime = a;
}
uint16_t
BpduHeader::GetHelloTime (void)
{
  return m_helloTime;
}


void
BpduHeader::SetForwDelay(uint16_t a)
{
  m_forwDelay = a;
}
uint16_t
BpduHeader::GetForwDelay (void)
{
  return m_forwDelay;
}

void
BpduHeader::SetTcFlag (bool f)
{
  //TC: 0x80 1000 0000
  if(f==true)
    {
      m_flags |= 0x80;
    }
  else
    {
      m_flags &= ~0x80;
    }
}
bool
BpduHeader::GetTcFlag ()
{
  //TC: 0x80 1000 0000
  bool booleanTmp = ( ( m_flags & 0x80 )==0x80 ? true : false );
  return booleanTmp;
}

void
BpduHeader::SetPropFlag (bool f)
{
  //Prop: 0x40 0100 0000
  if(f==true)
    {
      m_flags |= 0x40;
    }
  else
    {
      m_flags &= ~0x40;
    }
}
bool
BpduHeader::GetPropFlag ()
{
  //Prop: 0x40 0100 0000
  bool booleanTmp = ( ( m_flags & 0x40 )==0x40 ? true : false );
  return booleanTmp;
}

void
BpduHeader::SetLrnFlag (bool f)
{
  //Lrn: 0x08 0000 1000
  if(f==true)
    {
      m_flags |= 0x08;
    }
  else
    {
      m_flags &= ~0x08;
    }
}
bool
BpduHeader::GetLrnFlag ()
{
  //Lrn: 0x08 0000 1000
  bool booleanTmp = ( ( m_flags & 0x08 )==0x08 ? true : false );
  return booleanTmp;
}

void
BpduHeader::SetFrwFlag (bool f)
{
  //Lrn: 0x04 0000 0100
  if(f==true)
    {
      m_flags |= 0x04;
    }
  else
    {
      m_flags &= ~0x04;
    }
}
bool
BpduHeader::GetFrwFlag ()
{
  //Lrn: 0x04 0000 0100
  bool booleanTmp = ( ( m_flags & 0x04 )==0x04 ? true : false );
  return booleanTmp;
}

void
BpduHeader::SetAgrFlag (bool f)
{
  //Agr: 0x02 0000 0010
  if(f==true)
    {
      m_flags |= 0x02;
    }
  else
    {
      m_flags &= ~0x02;
    }
}
bool
BpduHeader::GetAgrFlag ()
{
  //Agr: 0x02 0000 0010
  bool booleanTmp = ( ( m_flags & 0x02 )==0x02 ? true : false );
  return booleanTmp;
}

void
BpduHeader::SetTcaFlag (bool f)
{
  //Tca: 0x01 0000 0001
  if(f==true)
    {
      m_flags |= 0x01;
    }
  else
    {
      m_flags &= ~0x01;
    }
}
bool
BpduHeader::GetTcaFlag ()
{
  //Tca: 0x01 0000 0001
  bool booleanTmp = ( ( m_flags & 0x01 )==0x01 ? true : false );
  return booleanTmp;
}

void 
BpduHeader::SetRoleFlags (std::string r)
{
  if( r==ROOT_ROLE )
    {
      // b00
      m_flags &= ~0x20; // reset bit
      m_flags &= ~0x10; // reset bit
    }
  else
    {
      if( r==DESIGNATED_ROLE )
        {
          // b01
          m_flags &= ~0x20; // reset bit
          m_flags |= 0x10; // set bit           
        }
        else
          {
            if( r==ALTERNATE_ROLE )
              {
                // b10
                m_flags |= 0x20;  // set bit
                m_flags &= ~0x10; // reset bit
              }
            else
              {
                if( r==BACKUP_ROLE )
                  {
                    // b11
                    m_flags |= 0x20;  // set bit
                    m_flags |= 0x10;  // set bit
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
BpduHeader::GetRoleFlags ()
{
  if((m_flags & 0x20)==0x00 && (m_flags & 0x10)==0x00) // flags: 0x00
    {
      return ROOT_ROLE;
    }
  else
    {
      if((m_flags & 0x20)==0x00 && (m_flags & 0x10)==0x10) // flags: 0x10
        {
          return DESIGNATED_ROLE;     
        }
        else
          {
            if((m_flags & 0x20)==0x20 && (m_flags & 0x10)==0x00) // flags: 0x20
              {
                return ALTERNATE_ROLE;
              }
            else
              {
                if((m_flags & 0x20)==0x20 && (m_flags & 0x10)==0x10) // flags: 0x30
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
BpduHeader::SetBpduSeq(uint64_t s)
{
  m_seqNum = s;
}
uint64_t
BpduHeader::GetBpduSeq(void)
{
  return m_seqNum;
}

//////////////////////////
// LSP & HELLO
//////////////////////////
void
BpduHeader::SetType(uint16_t f)
{
  m_type = f;
}
uint16_t
BpduHeader::GetType (void)
{
  return m_type;
}

void
BpduHeader::SetHelloCost(uint32_t c)
{
  m_helloCost = c;
}
uint32_t
BpduHeader::GetHelloCost (void)
{
  return m_helloCost;
}

void
BpduHeader::SetLspSequence(uint64_t s)
{
  m_lspSequence = s;
}
uint64_t
BpduHeader::GetLspSequence (void)
{
  return m_lspSequence;
}

void
BpduHeader::SetLspNumAdjacencies(uint64_t n)
{
  m_lspNumAdjacencies = n;
}
uint64_t
BpduHeader::GetLspNumAdjacencies (void)
{
  return m_lspNumAdjacencies;
}

void
BpduHeader::AddAdjacency(uint64_t n, uint32_t c)
{
  m_lspAdjBridgeId.push_back(n);
  m_lspAdjCost.push_back(c);
}
uint64_t
BpduHeader::GetAdjBridgeId (uint64_t n)
{
  return m_lspAdjBridgeId.at(n);
}
uint32_t
BpduHeader::GetAdjCost (uint64_t n)
{
  return m_lspAdjCost.at(n);
}



//////////////////////////
// BPDU with PV
//////////////////////////

void
BpduHeader::SetPathVectorNumBridgeIds(uint16_t n)
{
  m_pathVectorNumBridgeIds = n;
}
uint16_t
BpduHeader::GetPathVectorNumBridgeIds (void)
{
  return m_pathVectorNumBridgeIds;
}

void
BpduHeader::AddPathVectorBridgeId(uint64_t b)
{
  m_pathVectorBridgeIds.push_back(b);
  if(m_pathVectorBridgeIds.size() > m_pathVectorNumBridgeIds)
    {
      std::cout << "ERROR: AddPathVectorBridgeId() more path vectors elements than expected";
      exit(1);
    }
}
uint64_t
BpduHeader::GetPathVectorBridgeId (uint16_t i)
{
  return m_pathVectorBridgeIds.at(i);
}


//////////////////////////
// BPDU with Promises
//////////////////////////
void
BpduHeader::SetPN(uint16_t pn)
{
  m_pn = pn;
}
uint16_t
BpduHeader::GetPN (void)
{
  return m_pn;
}
void
BpduHeader::SetDPN(uint16_t dpn)
{
  m_dpn = dpn;
}
uint16_t
BpduHeader::GetDPN (void)
{
  return m_dpn;
}





//////////////////////////
// COMMON
//////////////////////////

TypeId 
BpduHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BpduHeader")
    .SetParent<Header> ()
    .AddConstructor<BpduHeader> ()
    ;
  return tid;
}

TypeId 
BpduHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
BpduHeader::Print (std::ostream &os) const
{
  /*os << " length/type=0x" << std::hex << m_lengthType << std::dec
     << ", source=" << m_source
     << ", destination=" << m_destination;*/
}
uint32_t 
BpduHeader::GetSerializedSize (void) const
{
  if(m_type==HELLO_TYPE)
    {
      return 14;
    }
  else if(m_type==LSP_TYPE)
    {
      return (26 + m_lspNumAdjacencies*12);
    }
  else if(m_type==BPDU_TYPE)
    {
      return 35;
    }
  else if(m_type==TCN_TYPE)
    {
      return 2;
    }
  else if(m_type==BPDU_TAP_TYPE)
    {
      return 39;
    }
  else if(m_type==BPDU_PV_TYPE)
    {
      return 35 + 2 + m_pathVectorNumBridgeIds*8 + 8;
    }
  else
    {
      std::cout << "ERROR: GetSerializedSize() not matching LSP type";
      exit(1);
    }
}

void
BpduHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  if(m_type==HELLO_TYPE)
    {
      i.WriteU16 (m_type);
      i.WriteU64 (m_bridgeId);      
      i.WriteU32 (m_helloCost);
      //remember to update GetSerializeSize()
    }
  else if(m_type==LSP_TYPE)
    {
      i.WriteU16 (m_type);
      i.WriteU64 (m_bridgeId);      
      i.WriteU64 (m_lspSequence);
      i.WriteU64 (m_lspNumAdjacencies);
      for(uint64_t a=0; a<m_lspNumAdjacencies; a++)
        {
          i.WriteU64 (m_lspAdjBridgeId.at(a));
          i.WriteU32 (m_lspAdjCost.at(a));
        }
      //remember to update GetSerializeSize()
    }
  else if(m_type==BPDU_TYPE)
    {
      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU8  (m_protVersion);
      i.WriteU8  (m_bpduType);
      i.WriteU8  (m_flags);     
      i.WriteU64 (m_rootId);
      i.WriteU32 (m_cost);
      i.WriteU64 (m_bridgeId);
      i.WriteU16 (m_portId);
      i.WriteU16 (m_messAge);
      i.WriteU16 (m_maxAge);  
      i.WriteU16 (m_helloTime);  
      i.WriteU16 (m_forwDelay);  
      //i.WriteU64 (m_pn);  
      //i.WriteU64 (m_dpn); 
      //remember to update GetSerializeSize()    
    }
  else if(m_type==TCN_TYPE)
    {
      i.WriteU16 (m_type); //should be protocolIdentifier...
    }
  else if(m_type==BPDU_TAP_TYPE)
    {
      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU8  (m_protVersion);
      i.WriteU8  (m_bpduType);
      i.WriteU8  (m_flags);     
      i.WriteU64 (m_rootId);
      i.WriteU32 (m_cost);
      i.WriteU64 (m_bridgeId);
      i.WriteU16 (m_portId);
      i.WriteU16 (m_messAge);
      i.WriteU16 (m_maxAge);  
      i.WriteU16 (m_helloTime);  
      i.WriteU16 (m_forwDelay);  
      i.WriteU16 (m_pn);  
      i.WriteU16 (m_dpn); 
      //remember to update GetSerializeSize()
    }
  else if(m_type==BPDU_PV_TYPE)
    {
      i.WriteU16 (m_type); //should be protocolIdentifier...
      i.WriteU8  (m_protVersion);
      i.WriteU8  (m_bpduType);
      i.WriteU8  (m_flags);     
      i.WriteU64 (m_rootId);
      i.WriteU32 (m_cost);
      i.WriteU64 (m_bridgeId);
      i.WriteU16 (m_portId);
      i.WriteU16 (m_messAge);
      i.WriteU16 (m_maxAge);  
      i.WriteU16 (m_helloTime);  
      i.WriteU16 (m_forwDelay);  
      i.WriteU16 (m_pathVectorNumBridgeIds);
      for(uint64_t a=0; a<m_pathVectorNumBridgeIds; a++)
        {
          i.WriteU64 (m_pathVectorBridgeIds.at(a));
        }      
      i.WriteU64 (m_seqNum);
      //remember to update GetSerializeSize()    
    }
  else
    {
      std::cout << "ERROR: Serialize() not matching LSP type";
      exit(1);
    }


  

}
uint32_t
BpduHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

    m_type           = i.ReadU16 (); //for BPDU should be protocolIdentifier...
      
    if(m_type==HELLO_TYPE)
    {
      m_bridgeId    = i.ReadU64 ();
      m_helloCost = i.ReadU32 ();
      //remember to update GetSerializeSize()
    }
  else if(m_type==LSP_TYPE)
    {
      m_bridgeId          = i.ReadU64 ();
      m_lspSequence       = i.ReadU64 ();
      m_lspNumAdjacencies = i.ReadU64 ();
      
      for(uint64_t a=0; a<m_lspNumAdjacencies; a++)
        {
          m_lspAdjBridgeId.push_back(i.ReadU64());
          m_lspAdjCost.push_back(i.ReadU32());
        }
      //remember to update GetSerializeSize()
    }
  else if(m_type==BPDU_TYPE)
    {
      m_protVersion    = i.ReadU8 ();
      m_bpduType       = i.ReadU8 ();
      m_flags          = i.ReadU8 ();    
      m_rootId         = i.ReadU64 ();
      m_cost           = i.ReadU32 ();
      m_bridgeId       = i.ReadU64 ();
      m_portId         = i.ReadU16 ();
      m_messAge        = i.ReadU16 ();
      m_maxAge         = i.ReadU16 ();
      m_helloTime      = i.ReadU16 ();
      m_forwDelay      = i.ReadU16 ();  
      //remember to update GetSerializeSize()      
    }
  else if(m_type==TCN_TYPE)
    {
    }
  else if(m_type==BPDU_TAP_TYPE)
    {
      m_protVersion    = i.ReadU8 ();
      m_bpduType       = i.ReadU8 ();
      m_flags          = i.ReadU8 ();    
      m_rootId         = i.ReadU64 ();
      m_cost           = i.ReadU32 ();
      m_bridgeId       = i.ReadU64 ();
      m_portId         = i.ReadU16 ();
      m_messAge        = i.ReadU16 ();
      m_maxAge         = i.ReadU16 ();
      m_helloTime      = i.ReadU16 ();
      m_forwDelay      = i.ReadU16 ();
      m_pn             = i.ReadU16 ();
      m_dpn            = i.ReadU16 ();            
      //remember to update GetSerializeSize()      
    }
  else if(m_type==BPDU_PV_TYPE)
    {
      m_protVersion    = i.ReadU8 ();
      m_bpduType       = i.ReadU8 ();
      m_flags          = i.ReadU8 ();    
      m_rootId         = i.ReadU64 ();
      m_cost           = i.ReadU32 ();
      m_bridgeId       = i.ReadU64 ();
      m_portId         = i.ReadU16 ();
      m_messAge        = i.ReadU16 ();
      m_maxAge         = i.ReadU16 ();
      m_helloTime      = i.ReadU16 ();
      m_forwDelay      = i.ReadU16 ();   
      m_pathVectorNumBridgeIds = i.ReadU16 ();   
      for(uint64_t a=0; a<m_pathVectorNumBridgeIds; a++)
        {
          m_pathVectorBridgeIds.push_back(i.ReadU64());
        }
      m_seqNum         = i.ReadU64 ();
      //remember to update GetSerializeSize()      
    }
  else
    {
      std::cout << "ERROR: Deserialize() not matching LSP type";
      exit(1);
    }
    
  return GetSerializedSize ();
}

} // namespace ns3
