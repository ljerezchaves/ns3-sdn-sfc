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

#include "vnf-info.h"
#include "vnf-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VnfInfo");
NS_OBJECT_ENSURE_REGISTERED (VnfInfo);

// Initializing VnfInfo static members.
VnfInfo::VnfInfoMap_t VnfInfo::m_vnfInfoById;

VnfInfo::VnfInfo (uint32_t vnfId)
  : m_vnfId (vnfId),
  m_1stScaling (1),
  m_2ndScaling (1)
{
  NS_LOG_FUNCTION (this);

  // Allocate virtual IP and MAC addresses for this VNF
  std::string vnfIpStr = "10.10.1." + std::to_string (vnfId);
  m_vnfIpAddress = Ipv4Address (vnfIpStr.c_str ());
  m_vnfMacAddress = Mac48Address::Allocate ();

  // Configure the factories
  m_1stFactory.SetTypeId (VnfApp::GetTypeId ());
  m_1stFactory.Set ("VnfId", UintegerValue (vnfId));
  m_1stFactory.Set ("Ipv4Address", Ipv4AddressValue (m_vnfIpAddress));

  m_2ndFactory.SetTypeId (VnfApp::GetTypeId ());
  m_2ndFactory.Set ("VnfId", UintegerValue (vnfId));
  m_2ndFactory.Set ("Ipv4Address", Ipv4AddressValue (m_vnfIpAddress));

  RegisterVnfInfo (Ptr<VnfInfo> (this));
}

VnfInfo::~VnfInfo ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
VnfInfo::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VnfInfo")
    .SetParent<Object> ()
  ;
  return tid;
}

uint32_t
VnfInfo::GetVnfId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_vnfId;
}

Ipv4Address
VnfInfo::GetIpAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_vnfIpAddress;
}

Mac48Address
VnfInfo::GetMacAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_vnfMacAddress;
}

double
VnfInfo::Get1stScaling (void) const
{
  NS_LOG_FUNCTION (this);

  return m_1stScaling;
}

double
VnfInfo::Get2ndScaling (void) const
{
  NS_LOG_FUNCTION (this);

  return m_2ndScaling;
}

void
VnfInfo::Set1stScaling (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_1stScaling = value;
  m_1stFactory.Set ("PktSizeScalingFactor", DoubleValue (value));
  for (auto &app : m_1stAppList)
    {
      app->SetAttribute ("PktSizeScalingFactor", DoubleValue (value));
    }
}

void
VnfInfo::Set2ndScaling (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_2ndScaling = value;
  m_2ndFactory.Set ("PktSizeScalingFactor", DoubleValue (value));
  for (auto &app : m_2ndAppList)
    {
      app->SetAttribute ("PktSizeScalingFactor", DoubleValue (value));
    }
}

std::pair<Ptr<VnfApp>, Ptr<VnfApp>>
VnfInfo::CreateVnfApps (void)
{
  NS_LOG_FUNCTION (this);

  std::pair<Ptr<VnfApp>, Ptr<VnfApp>> apps;

  apps.first = m_1stFactory.Create ()->GetObject<VnfApp> ();
  apps.second = m_2ndFactory.Create ()->GetObject<VnfApp> ();

  m_1stAppList.push_back (apps.first);
  m_2ndAppList.push_back (apps.second);

  return apps;
}

Ptr<VnfInfo>
VnfInfo::GetPointer (uint32_t vnfId)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<VnfInfo> vnfInfo = 0;
  auto ret = VnfInfo::m_vnfInfoById.find (vnfId);
  if (ret != VnfInfo::m_vnfInfoById.end ())
    {
      vnfInfo = ret->second;
    }
  return vnfInfo;
}

void
VnfInfo::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_1stAppList.clear ();
  m_2ndAppList.clear ();
  Object::DoDispose ();
}

void
VnfInfo::RegisterVnfInfo (Ptr<VnfInfo> vnfInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t vnfId = vnfInfo->GetVnfId ();
  std::pair<uint32_t, Ptr<VnfInfo>> entry (vnfId, vnfInfo);
  auto ret = VnfInfo::m_vnfInfoById.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing VNF info with this ID.");
}

} // namespace ns3
