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
#include "seq-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

TypeId 
SeqTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqTag")
    .SetParent<Tag> ()
    .AddConstructor<SeqTag> ()
    .AddAttribute ("id", "The seq of the sent packet",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SeqTag::Get),
                   MakeUintegerChecker<uint16_t> ())
    ;
  return tid;
}

TypeId
SeqTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

SeqTag::SeqTag ():
  m_seq (0)
{}

uint32_t 
SeqTag::GetSerializedSize (void) const
{
  return 2;
}

void
SeqTag::Serialize (TagBuffer i) const
{
  i.WriteU16 (m_seq);
}

void
SeqTag::Deserialize (TagBuffer i)
{
  m_seq = i.ReadU16 ();
}

void
SeqTag::Set (uint16_t id)
{
  m_seq = id;
}

uint16_t
SeqTag::Get () const
{
  return m_seq;
}

void
SeqTag::Print (std::ostream &os) const
{
  os << "seq=" << m_seq;
}

} //namespace ns3
