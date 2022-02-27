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
  m_core1SwitchNode = m_networkNodes.Get (0);
  m_edge1SwitchNode = m_networkNodes.Get (1);
  m_edge2SwitchNode = m_networkNodes.Get (2);
  Names::Add ("core", m_core1SwitchNode);
  Names::Add ("edge1", m_edge1SwitchNode);
  Names::Add ("edge2", m_edge2SwitchNode);
  m_core1SwitchDevice = m_networkDevices.Get (0);
  m_edge1SwitchDevice = m_networkDevices.Get (1);
  m_edge2SwitchDevice = m_networkDevices.Get (2);

  m_core1Server1Node = m_serverNodes.Get (0);
  m_core1Server2Node = m_serverNodes.Get (1);
  m_edge1Server1Node = m_serverNodes.Get (2);
  m_edge2Server1Node = m_serverNodes.Get (3);
  Names::Add ("server1(core)", m_core1Server1Node);
  Names::Add ("server2(core)", m_core1Server2Node);
  Names::Add ("server(edge1)", m_edge1Server1Node);
  Names::Add ("server(edge2)", m_edge2Server1Node);
  m_core1Server1Device = m_serverDevices.Get (0);
  m_core1Server2Device = m_serverDevices.Get (1);
  m_edge1Server1Device = m_serverDevices.Get (2);
  m_edge2Server1Device = m_serverDevices.Get (3);

  m_host1Node = m_hostNodes.Get (0);
  m_host2Node = m_hostNodes.Get (1);
  m_host3Node = m_hostNodes.Get (2);
  Names::Add ("h1", m_host1Node);
  Names::Add ("h2", m_host2Node);
  Names::Add ("h3", m_host3Node);

  // Configure helper for CSMA connections;
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // ---------------------------------------------------------------------------
  // Connect each edge switch to the core switch.
  // TODO: Configure DataRate and delay for connections
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Core to edge 1 (link A)
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_edge1SwitchNode);
  m_core1ToEdge1APort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1ToCore1APort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to edge 1 (link B)
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_edge1SwitchNode);
  m_core1ToEdge1BPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1ToCore1BPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to edge 2 (link A)
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_edge2SwitchNode);
  m_core1ToEdge2APort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToCore1APort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to edge 2 (link B)
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_edge2SwitchNode);
  m_core1ToEdge2BPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToCore1BPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
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
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_core1Server1Node);
  m_core1ToServer1DlinkPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCore1DlinkPort = m_core1Server1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to server 1 uplink.
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_core1Server1Node);
  m_core1ToServer1UlinkPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCore1UlinkPort = m_core1Server1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to server 2 downlink.
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_core1Server2Node);
  m_core1ToServer2DlinkPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCore1DlinkPort = m_core1Server2Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Core to server 2 uplink.
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_core1Server2Node);
  m_core1ToServer2UlinkPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCore1UlinkPort = m_core1Server2Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 1 to server downlink.
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_edge1Server1Node);
  m_edge1ToServer1DlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToEdge1DlinkPort = m_edge1Server1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 1 to server uplink.
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_edge1Server1Node);
  m_edge1ToServer1UlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToEdge1UlinkPort = m_edge1Server1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 2 to server downlink.
  csmaDevices = m_csmaHelper.Install (m_edge2SwitchNode, m_edge2Server1Node);
  m_edge2ToServer1DlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToEdge2DlinkPort = m_edge2Server1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // Edge 2 to server uplink.
  csmaDevices = m_csmaHelper.Install (m_edge2SwitchNode, m_edge2Server1Node);
  m_edge2ToServer1UlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToEdge2UlinkPort = m_edge2Server1Device->AddSwitchPort (csmaDevices.Get (1));
  m_csmaPortDevices.Add (csmaDevices);

  // ---------------------------------------------------------------------------
  // Connect each host to the proper network switch
  // TODO: Configure DataRate and delay for connections
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Edge 1 to host 1
  csmaDevices = m_csmaHelper.Install (m_edge1SwitchNode, m_host1Node);
  m_edge1ToHostPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_csmaPortDevices.Add (csmaDevices.Get (0));
  m_hostDevices.Add (csmaDevices.Get (1));
  m_host1Device = csmaDevices.Get (1);

  // Edge 2 to host 2
  csmaDevices = m_csmaHelper.Install (m_edge2SwitchNode, m_host2Node);
  m_edge2ToHostPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_csmaPortDevices.Add (csmaDevices.Get (0));
  m_hostDevices.Add (csmaDevices.Get (1));
  m_host2Device = csmaDevices.Get (1);

  // Core to host 3
  csmaDevices = m_csmaHelper.Install (m_core1SwitchNode, m_host3Node);
  m_core1ToHostPort = m_core1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_csmaPortDevices.Add (csmaDevices.Get (0));
  m_hostDevices.Add (csmaDevices.Get (1));
  m_host3Device = csmaDevices.Get (1);

  // ---------------------------------------------------------------------------

  // Configure IP address in hosts.
  InternetStackHelper internetStackHelper;
  internetStackHelper.Install (m_hostNodes);
  m_hostAddrHelper.SetBase ("10.1.0.0", "255.255.0.0");
  m_hostIfaces = m_hostAddrHelper.Assign (m_hostDevices);
  m_host1Address = m_hostIfaces.GetAddress (0);
  m_host2Address = m_hostIfaces.GetAddress (1);
  m_host3Address = m_hostIfaces.GetAddress (2);

  // Notify the controller about the host nodes.
  m_controllerApp->NotifyHostAttach (
    m_edge1SwitchDevice, m_edge1ToHostPort->GetPortNo (), m_host1Device);
  m_controllerApp->NotifyHostAttach (
    m_edge2SwitchDevice, m_edge2ToHostPort->GetPortNo (), m_host2Device);
  m_controllerApp->NotifyHostAttach (
    m_core1SwitchDevice, m_core1ToHostPort->GetPortNo (), m_host3Device);

  // Let's connect the OpenFlow switches to the controller. From this point
  // on it is not possible to change the OpenFlow network configuration.
  m_switchHelper->CreateOpenFlowChannels ();

  Object::NotifyConstructionCompleted ();
}
