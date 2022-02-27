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

#include "sdn-network.h"

NS_LOG_COMPONENT_DEFINE ("SdnNetwork");
NS_OBJECT_ENSURE_REGISTERED (SdnNetwork);

SdnNetwork::SdnNetwork ()
  : m_controllerApp (0),
  m_controllerNode (0),
  m_switchHelper (0)
{
  NS_LOG_FUNCTION (this);
}

SdnNetwork::~SdnNetwork ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdnNetwork::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdnNetwork")
    .SetParent<Object> ()
    .AddConstructor<SdnNetwork> ()
  ;
  return tid;
}

void
SdnNetwork::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_controllerApp = 0;
  m_controllerNode = 0;
  m_switchHelper = 0;
  Object::DoDispose ();
}

void
SdnNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure IP address helper for hosts.
  m_hostAddHelper.SetBase ("10.1.0.0", "255.255.0.0");

  // Create the OFSwitch13 helper using P2P connections for OpenFlow channel.
  m_switchHelper = CreateObjectWithAttributes<OFSwitch13InternalHelper> (
      "ChannelType", EnumValue (OFSwitch13Helper::DEDICATEDP2P));

  // Create the SDN controller.
  m_controllerApp = CreateObject<SdnController> ();
  m_switchHelper->InstallController (m_controllerNode, m_controllerApp);

  // Create the edge and core switches.
  m_networkNodes.Create (3);
  m_networkDevices = m_switchHelper->InstallSwitch (m_networkNodes);

  // Set pointer to nodes and devices.
  Ptr<Node> coreSwitchNode1 = m_networkNodes.Get (0);
  Ptr<Node> edgeSwitchNode1 = m_networkNodes.Get (1);
  Ptr<Node> edgeSwitchNode2 = m_networkNodes.Get (2);
  Ptr<OFSwitch13Device> coreSwitchDev1 = m_networkDevices.Get (0);
  Ptr<OFSwitch13Device> edgeSwitchDev1 = m_networkDevices.Get (1);
  Ptr<OFSwitch13Device> edgeSwitchDev2 = m_networkDevices.Get (2);

  // TODO
  // Create two links between each edge switch and the core switch.
  // NetDeviceContainer csmaDevices;
  // csmaDevices = m_csmaHelper.Install (coreSwitchNode1, edgeSwitchNode1);

  // Create the server switches
  m_serverNodes.Create (4);
  m_serverDevices = m_switchHelper->InstallSwitch (m_serverNodes);

  // Set pointer to nodes and devices.
  Ptr<Node> coreServerNode1 = m_serverNodes.Get (0);
  Ptr<Node> coreServerNode2 = m_serverNodes.Get (1);
  Ptr<Node> edgeServerNode1 = m_serverNodes.Get (2);
  Ptr<Node> edgeServerNode2 = m_serverNodes.Get (3);
  Ptr<OFSwitch13Device> coreServerDev1 = m_serverDevices.Get (0);
  Ptr<OFSwitch13Device> coreServerDev2 = m_serverDevices.Get (1);
  Ptr<OFSwitch13Device> edgeServerDev1 = m_serverDevices.Get (2);
  Ptr<OFSwitch13Device> edgeServerDev2 = m_serverDevices.Get (3);

  // TODO
  // Connect each server switch to the proper network switch.

  // Create the host nodes.
  m_hostNodes.Create (2);

  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (m_hostNodes);


  // TODO
  // Connect each hosts to the edge switches.


  // Let's connect the OpenFlow switches to the controller. From this point
  // on it is not possible to change the OpenFlow network configuration.
  m_switchHelper->CreateOpenFlowChannels ();

  Object::NotifyConstructionCompleted ();
}
