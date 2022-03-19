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

#include "sfc-tag.h"
#include "vnf-info.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SfcTag);

SfcTag::SfcTag ()
  : m_timestamp (Simulator::Now ().GetTimeStep ()),
    m_finalIp (0),
    m_finalPort (0),
    m_trafficId (0),
    m_nVnfs (0),
    m_nextVnfIdx (0)
{
  memset (m_listVnfs, 0, 8);
}

SfcTag::SfcTag (uint16_t trafficId, std::vector<uint8_t> vnfList, InetSocketAddress finalAddr)
  : m_timestamp (Simulator::Now ().GetTimeStep ()),
    m_trafficId (trafficId),
    m_nVnfs (vnfList.size ()),
    m_nextVnfIdx (0)
{
  NS_ASSERT_MSG (m_nVnfs <= 8, "Maximum of 8 VNFs allowed in SFC.");

  m_finalIp = finalAddr.GetIpv4 ().Get ();
  m_finalPort = finalAddr.GetPort ();
  memset (m_listVnfs, 0, 8);
  for (size_t i = 0; i < m_nVnfs; i++)
    {
      m_listVnfs[i] = vnfList.at (i);
    }
}

TypeId
SfcTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfcTag")
    .SetParent<Tag> ()
    .AddConstructor<SfcTag> ()
  ;
  return tid;
}

TypeId
SfcTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
SfcTag::GetSerializedSize (void) const
{
  return 26;
}

void
SfcTag::Serialize (TagBuffer i) const
{
  i.WriteU64 (m_timestamp);
  i.WriteU32 (m_finalIp);
  i.WriteU16 (m_finalPort);
  i.WriteU16 (m_trafficId);
  i.WriteU8  (m_nVnfs);
  i.WriteU8  (m_nextVnfIdx);
  i.Write    (m_listVnfs, 8);
}

void
SfcTag::Deserialize (TagBuffer i)
{
  m_timestamp   = i.ReadU64 ();
  m_finalIp     = i.ReadU32 ();
  m_finalPort   = i.ReadU16 ();
  m_trafficId   = i.ReadU16 ();
  m_nVnfs       = i.ReadU8 ();
  m_nextVnfIdx  = i.ReadU8 ();
  i.Read (m_listVnfs, 8);
}

void
SfcTag::Print (std::ostream &os) const
{
  os << "SfcTag=(timestamp:" << Time (m_timestamp)
     << " finalIp:" << Ipv4Address (m_finalIp)
     << " finalPort:" << (uint16_t) m_finalPort
     << " traffId:" << (uint16_t) m_trafficId
     << " numOfVnfs:" << (uint16_t) m_nVnfs
     << " nextVnfIdx:" << (uint16_t) m_nextVnfIdx
     << " vnfList:";
  for (int i = 0; i < 7; i++)
    {
      os << (uint16_t) m_listVnfs[i] << ",";
    }
  os << (uint16_t) m_listVnfs[7] << ")" << std::endl;
}

Time
SfcTag::GetTimestamp (void) const
{
  return Time (m_timestamp);
}

uint16_t
SfcTag::GetTrafficId (void) const
{
  return m_trafficId;
}

InetSocketAddress
SfcTag::GetNextAddress (bool advance)
{
  if (m_nextVnfIdx < m_nVnfs)
    {
      Ptr<VnfInfo> vnfInfo = VnfInfo::GetPointer (m_listVnfs[m_nextVnfIdx]);
      if (advance)
        {
          m_nextVnfIdx++;
        }
      return InetSocketAddress (vnfInfo->GetIpAddr (), vnfInfo->GetUdpPort ());
    }
  else
    {
      return InetSocketAddress (Ipv4Address (m_finalIp), m_finalPort);
    }
}

} // namespace ns3
