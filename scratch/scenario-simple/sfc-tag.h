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
 */

#ifndef SFC_TAG_H
#define SFC_TAG_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

class Tag;

/**
 * Tag used for saving the list of VNFs in the service function chaining.
 * Current implementation can carry a maximum of 8 VNF IDs in the tag.
 */
class SfcTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  /** Constructors */
  SfcTag ();
  SfcTag (uint16_t trafficId, std::vector<uint8_t> vnfList, InetSocketAddress finallAddr);

  // Inherited from Tag
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Print (std::ostream &os) const;

  /**
   * Get the timestamp of when this packet was created.
   * \return The timestamp.
   */
  Time GetTimestamp (void) const;

  /**
   * Get the traffic identification value.
   * \return The traffic ID.
   */
  uint16_t GetTrafficId (void) const;

  /**
   * Get the socket address of the next VNF application in the SFC list. In case
   * there are no more VNFs in the SFC list, this method will return the socket
   * address of the final sink application.
   * \param advance If true, advance internal pointer in the SFC list.
   * \return The socket address.
   */
  InetSocketAddress GetNextAddress  (bool advance = true);

private:
  uint64_t  m_timestamp;        //!< Packet creation timestamp.
  uint32_t  m_finalIp;          //!< Final host IP
  uint16_t  m_finalPort;        //!< Final host port.
  uint16_t  m_trafficId;        //!< Traffic ID.
  uint8_t   m_nVnfs;            //!< Number of VNFs in the chain.
  uint8_t   m_nextVnfIdx;       //!< Next VNF ID index in the chain.
  uint8_t   m_listVnfs[8];      //!< VNF ID chain.
};

} // namespace ns3
#endif // SFC_TAG_H
