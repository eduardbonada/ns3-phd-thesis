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
#include "tree-tag-header.h"

NS_LOG_COMPONENT_DEFINE ("TreeTagHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TreeTagHeader);

TreeTagHeader::TreeTagHeader ()
{
}


void
TreeTagHeader::SetTreeTag (uint16_t t)
{
  m_treeTag = t;
}
uint16_t
TreeTagHeader::GetTreeTag (void)
{
  return m_treeTag;
}

TypeId 
TreeTagHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TreeTagHeader")
    .SetParent<Header> ()
    .AddConstructor<TreeTagHeader> ()
    ;
  return tid;
}

TypeId 
TreeTagHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
TreeTagHeader::Print (std::ostream &os) const
{
  /*os << " length/type=0x" << std::hex << m_lengthType << std::dec
     << ", source=" << m_source
     << ", destination=" << m_destination;*/
}
uint32_t 
TreeTagHeader::GetSerializedSize (void) const
{
  return 2;
}

void
TreeTagHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU16 (m_treeTag);
}
uint32_t
TreeTagHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_treeTag          = i.ReadU16 ();
         
  return GetSerializedSize ();
}

} // namespace ns3
