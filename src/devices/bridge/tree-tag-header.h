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

#ifndef _TREE_TAG_HEADER_H
#define _TREE_TAG_HEADER_H

#include <stdlib.h>
#include <string>
#include "ns3/header.h"
#include "ns3/mac48-address.h"

namespace ns3 {

class TreeTagHeader : public Header 
{
public:

  TreeTagHeader ();
  
  uint16_t GetTreeTag (void);
  void SetTreeTag (uint16_t p);


  //COMMON    
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  
private:
  uint16_t m_treeTag;
};

}; // namespace ns3


#endif /* TREE_TAG_HEADER_H */
