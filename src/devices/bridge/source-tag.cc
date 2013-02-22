/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "source-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

TypeId 
SourceTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SourceTag")
    .SetParent<Tag> ()
    .AddConstructor<SourceTag> ()
    .AddAttribute ("id", "The id of the source bridge",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SourceTag::Get),
                   MakeUintegerChecker<uint16_t> ())
    ;
  return tid;
}

TypeId
SourceTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

SourceTag::SourceTag ():
  m_id (0)
{}

uint32_t 
SourceTag::GetSerializedSize (void) const
{
  return 2;
}

void
SourceTag::Serialize (TagBuffer i) const
{
  i.WriteU16 (m_id);
}

void
SourceTag::Deserialize (TagBuffer i)
{
  m_id = i.ReadU16 ();
}

void
SourceTag::Set (uint16_t id)
{
  m_id = id;
}

uint16_t
SourceTag::Get () const
{
  return m_id;
}

void
SourceTag::Print (std::ostream &os) const
{
  os << "id=" << m_id;
}

} //namespace ns3
