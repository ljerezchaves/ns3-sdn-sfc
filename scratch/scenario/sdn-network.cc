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
SdnNetwork::EnablePcap (bool enable)
{
  NS_LOG_FUNCTION (this);

  if (enable)
    {
      m_switchHelper->EnableOpenFlowPcap ("ofchannel", false);
      m_csmaHelper.EnablePcap ("switch-port", m_csmaPortDevices, true);
      m_csmaHelper.EnablePcap ("host", m_hostDevices, true);
    }
}

void
SdnNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create the OFSwitch13 helper.
  m_switchHelper = CreateObject<OFSwitch13InternalHelper> ();

  // Create the SDN controller.
  m_controllerNode = CreateObject<Node> ();
  Names::Add ("ctrl", m_controllerNode);
  m_controllerApp = CreateObject<SdnController> ();
  m_controllerApp->SetSdnNetwork (Ptr<SdnNetwork> (this));
  m_switchHelper->InstallController (m_controllerNode, m_controllerApp);

  // Create the edge and core switches.
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (20)));
  m_networkNodes.Create (3);
  m_networkDevices = m_switchHelper->InstallSwitch (m_networkNodes);

  // Create the server switches
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (0)));
  m_serverNodes.Create (4);
  m_serverDevices = m_switchHelper->InstallSwitch (m_serverNodes);

  // Create the host nodes.
  m_hostNodes.Create (3);

  // Set pointer to nodes and devices.
  m_coreSwitchNode = m_networkNodes.Get (0);
  m_edge1SwitchNode = m_networkNodes.Get (1);
  m_edge2SwitchNode = m_networkNodes.Get (2);
  Names::Add ("core", m_coreSwitchNode);
  Names::Add ("edge1", m_edge1SwitchNode);
  Names::Add ("edge2", m_edge2SwitchNode);
  m_coreSwitchDevice = m_networkDevices.Get (0);
  m_edge1SwitchDevice = m_networkDevices.Get (1);
  m_edge2SwitchDevice = m_networkDevices.Get (2);

  m_coreServer1Node = m_serverNodes.Get (0);
  m_coreServer2Node = m_serverNodes.Get (1);
  m_edge1ServerNode = m_serverNodes.Get (2);
  m_edge2ServerNode = m_serverNodes.Get (3);
  Names::Add ("server1(core)", m_coreServer1Node);
  Names::Add ("server2(core)", m_coreServer2Node);
  Names::Add ("server(edge1)", m_edge1ServerNode);
  Names::Add ("server(edge2)", m_edge2ServerNode);
  m_coreServer1Device = m_serverDevices.Get (0);
  m_coreServer2Device = m_serverDevices.Get (1);
  m_edge1ServerDevice = m_serverDevices.Get (2);
  m_edge2ServerDevice = m_serverDevices.Get (3);

  m_coreHostNode = m_hostNodes.Get (0);
  m_edge1HostNode = m_hostNodes.Get (1);
  m_edge2HostNode = m_hostNodes.Get (2);
  Names::Add ("host(core)", m_coreHostNode);
  Names::Add ("host(edge1)", m_edge1HostNode);
  Names::Add ("host(edge2)", m_edge2HostNode);

  // Configure helper for CSMA connections;
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // ---------------------------------------------------------------------------
  // Connect each edge switch to the core switch.
  // TODO: Configure DataRate and delay for connections
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Core to edge 1 (link A)
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_edge1SwitchNode);
  m_coreToEdge1APort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1ToCoreAPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to edge 1 (link B)
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_edge1SwitchNode);
  m_coreToEdge1BPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1ToCoreBPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to edge 2 (link A)
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_edge2SwitchNode);
  m_coreToEdge2APort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToCoreAPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to edge 2 (link B)
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_edge2SwitchNode);
  m_coreToEdge2BPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToCoreBPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 1 to edge 2
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_edge2SwitchNode);
  m_edge1ToEdge2Port = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToEdge1Port = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // ---------------------------------------------------------------------------
  // Connect each server to the proper network switch (uplink and downlink)
  // TODO: Configure DataRate and delay for connections
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Core to server 1 downlink.
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_coreServer1Node);
  m_coreToServer1DlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCoreDlinkPort = m_coreServer1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to server 1 uplink.
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_coreServer1Node);
  m_coreToServer1UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCoreUlinkPort = m_coreServer1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to server 2 downlink.
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_coreServer2Node);
  m_coreToServer2DlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCoreDlinkPort = m_coreServer2Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to server 2 uplink.
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_coreServer2Node);
  m_coreToServer2UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCoreUlinkPort = m_coreServer2Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 1 to server downlink.
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_edge1ServerNode);
  m_edge1ToServerDlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1DlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 1 to server uplink.
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_edge1ServerNode);
  m_edge1ToServerUlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1UlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 2 to server downlink.
  csmaDevices = m_csmaHelper.Install (m_edge2SwitchNode, m_edge2ServerNode);
  m_edge2ToServerDlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2DlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 2 to server uplink.
  csmaDevices = m_csmaHelper.Install (m_edge2SwitchNode, m_edge2ServerNode);
  m_edge2ToServerUlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2UlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // ---------------------------------------------------------------------------
  // Connect each host to the proper network switch
  // TODO: Configure DataRate and delay for connections
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Edge 1 to host
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_edge1HostNode);
  m_edge1ToHostPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_csmaPortDevices.Add (csmaDevices.Get (0));
  m_edge1HostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_edge1HostDevice);

  // Edge 2 to host
  csmaDevices = m_csmaHelper.Install (m_edge2SwitchNode, m_edge2HostNode);
  m_edge2ToHostPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_csmaPortDevices.Add (csmaDevices.Get (0));
  m_edge2HostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_edge2HostDevice);

  // Core to host
  csmaDevices = m_csmaHelper.Install (m_coreSwitchNode, m_coreHostNode);
  m_coreToHostPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_csmaPortDevices.Add (csmaDevices.Get (0));
  m_coreHostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_coreHostDevice);

  // ---------------------------------------------------------------------------

  // Configure IP address in hosts.
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (m_hostNodes);
  m_hostAddrHelper.SetBase ("10.1.0.0", "255.255.0.0");
  m_hostIfaces = m_hostAddrHelper.Assign (m_hostDevices);
  m_edge1HostAddress = m_hostIfaces.GetAddress (0);
  m_edge2HostAddress = m_hostIfaces.GetAddress (1);
  m_coreHostAddress = m_hostIfaces.GetAddress (2);

  // Notify the controller about the host nodes.
  m_controllerApp->NotifyHostAttach (
    m_edge1SwitchDevice, m_edge1ToHostPort->GetPortNo (), m_edge1HostDevice);
  m_controllerApp->NotifyHostAttach (
    m_edge2SwitchDevice, m_edge2ToHostPort->GetPortNo (), m_edge2HostDevice);
  m_controllerApp->NotifyHostAttach (
    m_coreSwitchDevice, m_coreToHostPort->GetPortNo (), m_coreHostDevice);

  // Let's connect the OpenFlow switches to the controller. From this point
  // on it is not possible to change the OpenFlow network configuration.
  m_switchHelper->CreateOpenFlowChannels ();

  Object::NotifyConstructionCompleted ();
}
