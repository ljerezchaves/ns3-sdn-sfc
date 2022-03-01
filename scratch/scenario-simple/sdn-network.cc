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

  // Create and configure the VNF information and application factories.
  Ptr<VnfInfo> vnfInfo1 = CreateObject<VnfInfo> (1);
  vnfInfo1->SetSwitchScaling (1.5);
  vnfInfo1->SetServerScaling (1/1.5);

  ObjectFactory vnfSwitchFactory;
  vnfSwitchFactory.SetTypeId (VnfApp::GetTypeId ());
  vnfSwitchFactory.Set ("VnfId", UintegerValue (vnfInfo1->GetVnfId ()));
  vnfSwitchFactory.Set ("Ipv4Address", Ipv4AddressValue (vnfInfo1->GetSwitchIpAddr ()));
  vnfSwitchFactory.Set ("PktSizeScalingFactor", DoubleValue (vnfInfo1->GetSwitchScaling ()));

  ObjectFactory vnfServerFactory;
  vnfServerFactory.SetTypeId (VnfApp::GetTypeId ());
  vnfServerFactory.Set ("VnfId", UintegerValue (vnfInfo1->GetVnfId ()));
  vnfServerFactory.Set ("Ipv4Address", Ipv4AddressValue (vnfInfo1->GetServerIpAddr ()));
  vnfServerFactory.Set ("PktSizeScalingFactor", DoubleValue (vnfInfo1->GetServerScaling ()));

  NS_LOG_UNCOND ("VNF1: ip " << vnfInfo1->GetServerIpAddr () << " " << vnfInfo1->GetSwitchIpAddr ());

  // Notify the controller about this new VNF
  // m_controllerApp->

  // Install a copy of this VNF in all switches
}

void
SdnNetwork::ConfigureApplications (void)
{
  NS_LOG_FUNCTION (this);

  // uint16_t sourceSinkPorts = 40001;
  // uint16_t vnfUdpPort = 40999;
  // Ipv4Address vnfIpAddress ("10.10.0.1");
  // Mac48Address vnfMacAddress = Mac48Address::Allocate ();

  // // Chain from host 1 (10.0.0.1) to VNF 1 (10.10.0.1) to host 2 (10.0.0.2).

  // // Configure the source application on host 1.
  // Ptr<SourceApp> sourceApp = CreateObject<SourceApp> ();
  // sourceApp->SetStartTime (Seconds (1));
  // sourceApp->SetLocalUdpPort (sourceSinkPorts);
  // sourceApp->SetNextIpAddress (vnfIpAddress);
  // sourceApp->SetNextUdpPort (vnfUdpPort);
  // m_host1Node->AddApplication (sourceApp);

  // // Configure the VNF application on the switch.
  // ObjectFactory vnfFactory;
  // vnfFactory.SetTypeId (VnfApp::GetTypeId ());
  // vnfFactory.Set ("LocalIpAddress", Ipv4AddressValue (vnfIpAddress));
  // vnfFactory.Set ("LocalUdpPort", UintegerValue (vnfUdpPort));
  // vnfFactory.Set ("NextIpAddress", Ipv4AddressValue (m_host2Address));
  // vnfFactory.Set ("NextUdpPort", UintegerValue (sourceSinkPorts));
  // vnfFactory.Set ("PktSizeScalingFactor", DoubleValue (1.5));

  // Ptr<VnfApp> vnfApp = vnfFactory.Create ()->GetObject<VnfApp> ();
  // InstallVnf (m_switchNode, m_switchDevice, vnfApp, vnfIpAddress, vnfMacAddress);

  // // Configure the sink  application on host 2.
  // Ptr<SinkApp> sinkApp = CreateObject<SinkApp> ();
  // sinkApp->SetLocalUdpPort (sourceSinkPorts);
  // sinkApp->SetStartTime (Seconds (0));
  // m_host2Node->AddApplication (sinkApp);
}

void
SdnNetwork::InstallVnf (
  Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
  Ptr<VnfApp> application, Ipv4Address ipv4Address, Mac48Address macAddress)
{
  NS_LOG_FUNCTION (this << switchDevice << application << ipv4Address << macAddress);

  // Create the virtual net device to work as the logical port on the switch.
  Ptr<VirtualNetDevice> virtualDevice = CreateObject<VirtualNetDevice> ();
  virtualDevice->SetAddress (macAddress);
  Ptr<OFSwitch13Port> logicalPort = switchDevice->AddSwitchPort (virtualDevice);
  m_portDevices.Add (virtualDevice);

  // Configure the VNF application and notify the controller.
  application->SetVirtualDevice (virtualDevice);
  m_controllerApp->NotifyVnfAttach (
    switchDevice, logicalPort->GetPortNo (), ipv4Address, macAddress);

  switchNode->AddApplication (application);
}

} // namespace ns3
