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
  m_serverScaling (1),
  m_switchScaling (1),
  m_activeCopyIdx (0)
{
  NS_LOG_FUNCTION (this);

  // Allocate virtual IP addresses for this VNF
  std::string ipServerStr = "10.10.1." + std::to_string (vnfId);
  std::string ipSwitchStr = "10.10.2." + std::to_string (vnfId);
  m_serverIpAddress = Ipv4Address (ipServerStr.c_str ());
  m_switchIpAddress = Ipv4Address (ipSwitchStr.c_str ());

  // Allocate virtual MAC addresses for this VNF
  m_serverMacAddress = Mac48Address::Allocate ();
  m_switchMacAddress = Mac48Address::Allocate ();

  // Configure the factories
  m_serverFactory.SetTypeId (VnfApp::GetTypeId ());
  m_serverFactory.Set ("VnfId", UintegerValue (vnfId));
  m_serverFactory.Set ("Ipv4Address", Ipv4AddressValue (m_serverIpAddress));

  m_switchFactory.SetTypeId (VnfApp::GetTypeId ());
  m_switchFactory.Set ("VnfId", UintegerValue (vnfId));
  m_switchFactory.Set ("Ipv4Address", Ipv4AddressValue (m_switchIpAddress));

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
VnfInfo::GetServerIpAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverIpAddress;
}

Ipv4Address
VnfInfo::GetSwitchIpAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchIpAddress;
}

Mac48Address
VnfInfo::GetServerMacAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverMacAddress;
}

Mac48Address
VnfInfo::GetSwitchMacAddr (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchMacAddress;
}

double
VnfInfo::GetServerScaling (void) const
{
  NS_LOG_FUNCTION (this);

  return m_serverScaling;
}

double
VnfInfo::GetSwitchScaling (void) const
{
  NS_LOG_FUNCTION (this);

  return m_switchScaling;
}

int
VnfInfo::GetActiveCopyIdx (void) const
{
  NS_LOG_FUNCTION (this);

  return m_activeCopyIdx;
}

void
VnfInfo::SetServerScaling (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_serverScaling = value;
  m_serverFactory.Set ("PktSizeScalingFactor", DoubleValue (value));
  for (auto &app : m_serverAppList)
    {
      app->SetAttribute ("PktSizeScalingFactor", DoubleValue (value));
    }
}

void
VnfInfo::SetSwitchScaling (double value)
{
  NS_LOG_FUNCTION (this << value);

  m_switchScaling = value;
  m_switchFactory.Set ("PktSizeScalingFactor", DoubleValue (value));
  for (auto &app : m_switchAppList)
    {
      app->SetAttribute ("PktSizeScalingFactor", DoubleValue (value));
    }
}

void
VnfInfo::SetActiveCopyIdx (int value)
{
  NS_LOG_FUNCTION (this);

  m_activeCopyIdx = value;
}

Ptr<VnfApp>
VnfInfo::CreateServerApp (void)
{
  NS_LOG_FUNCTION (this);

  return m_serverFactory.Create ()->GetObject<VnfApp> ();
}

Ptr<VnfApp>
VnfInfo::CreateSwitchApp (void)
{
  NS_LOG_FUNCTION (this);

  return m_switchFactory.Create ()->GetObject<VnfApp> ();
}

void
VnfInfo::NewVnfCopy (
  Ptr<VnfApp> serverApp, Ptr<OFSwitch13Device> serverDevice, uint32_t serverPort,
  Ptr<VnfApp> switchApp, Ptr<OFSwitch13Device> switchDevice, uint32_t switchPort)
{
  NS_LOG_FUNCTION (this << serverApp << serverDevice << serverPort <<
                   switchApp << switchDevice << switchPort);

  m_serverAppList.push_back (serverApp);
  m_switchAppList.push_back (switchApp);
  m_serverDevList.Add (serverDevice);
  m_switchDevList.Add (switchDevice);
  m_serverPortList.push_back (serverPort);
  m_switchPortList.push_back (switchPort);
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

  m_switchAppList.clear ();
  m_serverAppList.clear ();
  Object::DoDispose ();
}

void
VnfInfo::RegisterVnfInfo (Ptr<VnfInfo> vnfInfo)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t vnfId = vnfInfo->GetVnfId ();
  std::pair<uint32_t, Ptr<VnfInfo> > entry (vnfId, vnfInfo);
  auto ret = VnfInfo::m_vnfInfoById.insert (entry);
  NS_ABORT_MSG_IF (ret.second == false, "Existing VNF info with this ID.");
}

} // namespace ns3
