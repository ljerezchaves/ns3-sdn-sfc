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

#include <ns3/virtual-net-device-module.h>
#include "sdn-network.h"
#include "vnf-info.h"
#include "vnf-app.h"
#include "source-app.h"
#include "sink-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SdnNetwork");
NS_OBJECT_ENSURE_REGISTERED (SdnNetwork);

SdnNetwork::SdnNetwork ()
  : m_controllerApp (0),
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
  m_switchHelper = 0;
  Object::DoDispose ();
}

void
SdnNetwork::EnablePcap (bool enable)
{
  NS_LOG_FUNCTION (this);

  if (enable)
    {
      CsmaHelper csmaHelper;
      csmaHelper.EnablePcap ("switch-port", m_portDevices, true);
      csmaHelper.EnablePcap ("host", m_hostDevices, true);
      m_switchHelper->EnableOpenFlowPcap ("ofchannel", false);
    }
}

void
SdnNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  ConfigureTopology ();
  ConfigureFunctions ();
  ConfigureApplications ();

  // Let's connect the OpenFlow switches to the controller. From this point
  // on it is not possible to change the OpenFlow network configuration.
  m_switchHelper->CreateOpenFlowChannels ();

  Object::NotifyConstructionCompleted ();
}

void
SdnNetwork::ConfigureTopology (void)
{
  NS_LOG_FUNCTION (this);
  // Create the OFSwitch13 helper.
  m_switchHelper = CreateObject<OFSwitch13InternalHelper> ();

  // Create the SDN controller.
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Names::Add ("ctrl", controllerNode);
  m_controllerApp = CreateObject<SdnController> ();
  m_controllerApp->SetSdnNetwork (Ptr<SdnNetwork> (this));
  m_switchHelper->InstallController (controllerNode, m_controllerApp);

  // Create the switch.
  m_switchNode  = CreateObject<Node> ();
  Names::Add ("switch",  m_switchNode);
  m_switchDevice = m_switchHelper->InstallSwitch (m_switchNode);

  // Create the server.
  m_serverNode  = CreateObject<Node> ();
  Names::Add ("server",  m_serverNode);
  m_serverDevice = m_switchHelper->InstallSwitch (m_serverNode);

  // Create the host.
  m_host1Node = CreateObject<Node> ();
  m_host2Node = CreateObject<Node> ();
  Names::Add ("host1", m_host1Node);
  Names::Add ("host2", m_host2Node);

  // Configure helper for CSMA connections;
  CsmaHelper csmaHelper;
  csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // Connect the switch to the server (uplink and downlink)
  csmaDevices = csmaHelper.Install (m_switchNode, m_serverNode);
  m_switchToServerUlinkPort = m_switchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToSwitchUlinkPort = m_serverDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  csmaDevices = csmaHelper.Install (m_switchNode, m_serverNode);
  m_switchToServerDlinkPort = m_switchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToSwitchDlinkPort = m_serverDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Connect each host to the network switch
  csmaDevices = csmaHelper.Install (m_switchNode, m_host1Node);
  m_switchToHost1Port = m_switchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_portDevices.Add (csmaDevices.Get (0));
  m_host1Device = csmaDevices.Get (1);
  m_hostDevices.Add (m_host1Device);

  csmaDevices = csmaHelper.Install (m_switchNode, m_host2Node);
  m_switchToHost2Port = m_switchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_portDevices.Add (csmaDevices.Get (0));
  m_host2Device = csmaDevices.Get (1);
  m_hostDevices.Add (m_host2Device);

  // Configure IP address in hosts.
  InternetStackHelper internetStackHelper;
  Ipv4InterfaceContainer hostIfaces;
  Ipv4AddressHelper hostAddressHelper;
  internetStackHelper.Install (m_host1Node);
  internetStackHelper.Install (m_host2Node);
  hostAddressHelper.SetBase ("10.0.0.0", "255.0.0.0");
  hostIfaces = hostAddressHelper.Assign (m_hostDevices);
  m_host1Address = hostIfaces.GetAddress (0);
  m_host2Address = hostIfaces.GetAddress (1);

  // Notify the controller about the host nodes.
  m_controllerApp->NotifyHostAttach (
    m_switchDevice, m_switchToHost1Port->GetPortNo (), m_host1Device);
  m_controllerApp->NotifyHostAttach (
    m_switchDevice, m_switchToHost2Port->GetPortNo (), m_host2Device);
}

void
SdnNetwork::ConfigureFunctions (void)
{
  NS_LOG_FUNCTION (this);

  // Create and configure the VNF (ID 1).
  Ptr<VnfInfo> vnfInfo1 = CreateObject<VnfInfo> (1);
  vnfInfo1->SetSwitchScaling (1.5);
  vnfInfo1->SetServerScaling (1/1.5);
  m_controllerApp->NotifyNewVnf (vnfInfo1);

  // Install a copy of this VNF in the switch and server nodes
  InstallVnfCopy (m_serverNode, m_serverDevice, m_switchNode, m_switchDevice, vnfInfo1);

  // Create and configure the VNF (ID 1).
  Ptr<VnfInfo> vnfInfo2 = CreateObject<VnfInfo> (2);
  vnfInfo2->SetSwitchScaling (3);
  vnfInfo2->SetServerScaling (0.8);
  m_controllerApp->NotifyNewVnf (vnfInfo2);

  // Install a copy of this VNF in the switch and server nodes
  InstallVnfCopy (m_serverNode, m_serverDevice, m_switchNode, m_switchDevice, vnfInfo2);
}

void
SdnNetwork::ConfigureApplications (void)
{
  NS_LOG_FUNCTION (this);

  // Configure the source application on host 1.
  uint16_t portNo = 40001;
  Ptr<SourceApp> sourceApp = CreateObject<SourceApp> ();
  sourceApp->SetStartTime (Seconds (1));
  sourceApp->SetLocalUdpPort (portNo);
  sourceApp->SetFinalIpAddress (m_host2Address);
  sourceApp->SetFinalUdpPort (portNo);
  m_host1Node->AddApplication (sourceApp);

  // Configure the sink application on host 2.
  Ptr<SinkApp> sinkApp = CreateObject<SinkApp> ();
  sinkApp->SetLocalUdpPort (portNo);
  sinkApp->SetStartTime (Seconds (0));
  m_host2Node->AddApplication (sinkApp);

  // SFC: host 1 --> VNF 1 --> host 2.
  sourceApp->SetVnfList ({1});
}

void
SdnNetwork::InstallVnfCopy (
  Ptr<Node> serverNode, Ptr<OFSwitch13Device> serverDevice,
  Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
  Ptr<VnfInfo> vnfInfo)
{
  NS_LOG_FUNCTION (this << serverNode << serverDevice <<
                   switchNode << switchDevice << vnfInfo);

  // First, install the application on the server switch
  // Create the virtual net device to work as the logical port on the switch.
  Ptr<VirtualNetDevice> virtualDevServer = CreateObject<VirtualNetDevice> ();
  virtualDevServer->SetAddress (vnfInfo->GetServerMacAddr ());
  Ptr<OFSwitch13Port> logicalPortServer = serverDevice->AddSwitchPort (virtualDevServer);
  m_portDevices.Add (virtualDevServer);

  // Create the VNF application.
  Ptr<VnfApp> serverApp = vnfInfo->CreateServerApp ();
  serverApp->SetVirtualDevice (virtualDevServer);
  serverNode->AddApplication (serverApp);

  // Then, install the application on the network switch
  // Create the virtual net device to work as the logical port on the switch.
  Ptr<VirtualNetDevice> virtualDevSwitch = CreateObject<VirtualNetDevice> ();
  virtualDevSwitch->SetAddress (vnfInfo->GetSwitchMacAddr ());
  Ptr<OFSwitch13Port> logicalPortSwitch = switchDevice->AddSwitchPort (virtualDevSwitch);
  m_portDevices.Add (virtualDevSwitch);

  // Create the VNF application.
  Ptr<VnfApp> switchApp = vnfInfo->CreateSwitchApp ();
  switchApp->SetVirtualDevice (virtualDevSwitch);
  switchNode->AddApplication (switchApp);

  // Notify the controller about this new VNF copy
  m_controllerApp->NotifyVnfAttach (
    serverDevice, logicalPortServer->GetPortNo (),
    switchDevice, logicalPortSwitch->GetPortNo (),
    vnfInfo, 0); // FIXME
}

} // namespace ns3
