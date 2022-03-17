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
    .AddAttribute ("NumberVnfs", "Total number of VNFs in this scenario.",
                   UintegerValue (3),
                   MakeUintegerAccessor (&SdnNetwork::m_numVnfs),
                   MakeUintegerChecker<uint16_t> ());
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
      csmaHelper.EnablePcap ("port", m_portDevices, true);
      csmaHelper.EnablePcap ("host", m_hostDevices, true);
      m_switchHelper->EnableOpenFlowPcap ("ofch", false);
    }
}

void
SdnNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Configure network topology, VNFs and applications (respect this order!).
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

  // ---------------------------------------------------------------------------
  // Create the SDN controller.
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Names::Add ("ctrl", controllerNode);
  m_controllerApp = CreateObject<SdnController> ();
  m_controllerApp->SetSdnNetwork (Ptr<SdnNetwork> (this));
  m_switchHelper->InstallController (controllerNode, m_controllerApp);

  // ---------------------------------------------------------------------------
  // Create the edge and core switches.
  m_core0SwitchNode = CreateObject<Node> ();
  m_edge1SwitchNode = CreateObject<Node> ();
  m_edge2SwitchNode = CreateObject<Node> ();
  Names::Add ("core0", m_core0SwitchNode);
  Names::Add ("edge1", m_edge1SwitchNode);
  Names::Add ("edge2", m_edge2SwitchNode);
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (20)));
  m_core0SwitchDevice = m_switchHelper->InstallSwitch (m_core0SwitchNode);
  m_edge1SwitchDevice = m_switchHelper->InstallSwitch (m_edge1SwitchNode);
  m_edge2SwitchDevice = m_switchHelper->InstallSwitch (m_edge2SwitchNode);

  // ---------------------------------------------------------------------------
  // Create the server switches
  m_core0ServerNode = CreateObject<Node> ();
  m_edge1ServerNode = CreateObject<Node> ();
  m_edge2ServerNode = CreateObject<Node> ();
  Names::Add ("server@core0", m_core0ServerNode);
  Names::Add ("server@edge1", m_edge1ServerNode);
  Names::Add ("server@edge2", m_edge2ServerNode);
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (0)));
  m_core0ServerDevice = m_switchHelper->InstallSwitch (m_core0ServerNode);
  m_edge1ServerDevice = m_switchHelper->InstallSwitch (m_edge1ServerNode);
  m_edge2ServerDevice = m_switchHelper->InstallSwitch (m_edge2ServerNode);

  // ---------------------------------------------------------------------------
  // Create the host nodes.
  m_core0HostNode = CreateObject<Node> ();
  m_edge1HostNode = CreateObject<Node> ();
  m_edge2HostNode = CreateObject<Node> ();
  Names::Add ("host@core0", m_core0HostNode);
  Names::Add ("host@edge1", m_edge1HostNode);
  Names::Add ("host@edge2", m_edge2HostNode);

  // ---------------------------------------------------------------------------
  // Configure helper for CSMA connections;
  CsmaHelper csmaHelper;
  csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // ---------------------------------------------------------------------------
  // Connect edge and core switches.
  // TODO: Configure DataRate and delay for connections
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

  // Core to edge 1
  csmaDevices = csmaHelper.Install (m_core0SwitchNode, m_edge1SwitchNode);
  m_core0ToEdge1Port = m_core0SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1Tocore0Port = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to edge 2
  csmaDevices = csmaHelper.Install (m_core0SwitchNode, m_edge2SwitchNode);
  m_core0ToEdge2Port = m_core0SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2Tocore0Port = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 1 to edge 2
  csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge2SwitchNode);
  m_edge1ToEdge2Port = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToEdge1Port = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // ---------------------------------------------------------------------------
  // Connect each server to the proper network switch (only downlink here)
  // Maximum datarate and zero delay for these links.
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate (std::numeric_limits<uint64_t>::max ())));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (Time (0)));

  // Core to server
  csmaDevices = csmaHelper.Install (m_core0SwitchNode, m_core0ServerNode);
  m_core0ToServerDlinkPort = m_core0SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverTocore0DlinkPort = m_core0ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 1 to server
  csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge1ServerNode);
  m_edge1ToServerDlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1DlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 2 to server
  csmaDevices = csmaHelper.Install (m_edge2SwitchNode, m_edge2ServerNode);
  m_edge2ToServerDlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2DlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // ---------------------------------------------------------------------------
  // Connect each host to the proper network switch
  // Maximum datarate and zero delay for these links.
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate (std::numeric_limits<uint64_t>::max ())));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (Time (0)));

  // Core to host
  csmaDevices = csmaHelper.Install (m_core0SwitchNode, m_core0HostNode);
  m_core0ToHostPort = m_core0SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_portDevices.Add (csmaDevices.Get (0));
  m_core0HostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_core0HostDevice);

  // Edge 1 to host
  csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge1HostNode);
  m_edge1ToHostPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_portDevices.Add (csmaDevices.Get (0));
  m_edge1HostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_edge1HostDevice);

  // Edge 2 to host
  csmaDevices = csmaHelper.Install (m_edge2SwitchNode, m_edge2HostNode);
  m_edge2ToHostPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_portDevices.Add (csmaDevices.Get (0));
  m_edge2HostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_edge2HostDevice);

  // ---------------------------------------------------------------------------
  // Configure IP address in hosts.
  InternetStackHelper internetStackHelper;
  Ipv4InterfaceContainer hostIfaces;
  Ipv4AddressHelper hostAddressHelper;
  internetStackHelper.Install (m_core0HostNode);
  internetStackHelper.Install (m_edge1HostNode);
  internetStackHelper.Install (m_edge2HostNode);
  hostAddressHelper.SetBase ("10.0.0.0", "255.0.0.0");
  hostIfaces = hostAddressHelper.Assign (m_hostDevices);
  m_core0HostAddress = hostIfaces.GetAddress (0);
  m_edge1HostAddress = hostIfaces.GetAddress (1);
  m_edge2HostAddress = hostIfaces.GetAddress (2);

  // ---------------------------------------------------------------------------
  // Notify the controller about the host nodes.
  m_controllerApp->NotifyHostAttach (m_core0SwitchDevice, m_core0ToHostPort->GetPortNo (), m_core0HostDevice);
  m_controllerApp->NotifyHostAttach (m_edge1SwitchDevice, m_edge1ToHostPort->GetPortNo (), m_edge1HostDevice);
  m_controllerApp->NotifyHostAttach (m_edge2SwitchDevice, m_edge2ToHostPort->GetPortNo (), m_edge2HostDevice);
}

void
SdnNetwork::ConfigureFunctions (void)
{
  NS_LOG_FUNCTION (this);

  // Configure helper for CSMA connections;
  CsmaHelper csmaHelper;
  csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // TODO: Configure initial DataRate and delay for VNF uplink connections.
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

  // Create and configure each VNF on each server.
  for (uint16_t i = 1; i <= m_numVnfs; i++)
    {
      Ptr<VnfInfo> vnfInfo = CreateObject<VnfInfo> (i);
      SdnController::SaveArpEntry (vnfInfo->GetIpAddr (), vnfInfo->GetMacAddr ());

      // Server 1 at core 1.
      csmaDevices = csmaHelper.Install (m_core0SwitchNode, m_core0ServerNode);
      m_core0ToServerVnfPorts.push_back (m_core0SwitchDevice->AddSwitchPort (csmaDevices.Get (0)));
      m_serverTocore0VnfPorts.push_back (m_core0ServerDevice->AddSwitchPort (csmaDevices.Get (1)));
      m_portDevices.Add (csmaDevices);
      m_core0ServerVnfLinks.push_back (DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ()));
      InstallVnfCopy (m_core0SwitchNode, m_core0SwitchDevice, m_core0ServerNode, m_core0ServerDevice,
                      m_core0ToServerVnfPorts.back ()->GetPortNo (), m_serverTocore0DlinkPort->GetPortNo (), vnfInfo);

      // Server 1 at edge 1.
      csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge1ServerNode);
      m_edge1ToServerVnfPorts.push_back (m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0)));
      m_serverToEdge1VnfPorts.push_back (m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1)));
      m_portDevices.Add (csmaDevices);
      m_edge1ServerVnfLinks.push_back (DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ()));
      InstallVnfCopy (m_edge1SwitchNode, m_edge1SwitchDevice, m_edge1ServerNode, m_edge1ServerDevice,
                      m_edge1ToServerVnfPorts.back ()->GetPortNo (), m_serverToEdge1DlinkPort->GetPortNo (), vnfInfo);

      // Server 1 at edge 2.
      csmaDevices = csmaHelper.Install (m_edge2SwitchNode, m_edge2ServerNode);
      m_edge2ToServerVnfPorts.push_back (m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0)));
      m_serverToEdge2VnfPorts.push_back (m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1)));
      m_portDevices.Add (csmaDevices);
      m_edge2ServerVnfLinks.push_back (DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ()));
      InstallVnfCopy (m_edge2SwitchNode, m_edge2SwitchDevice, m_edge2ServerNode, m_edge2ServerDevice,
                      m_edge2ToServerVnfPorts.back ()->GetPortNo (), m_serverToEdge2DlinkPort->GetPortNo (), vnfInfo);
    }

  // Configure the scaling factors for the VNFs.
  // VNF 1: network service
  Ptr<VnfInfo> vnfInfo1 = VnfInfo::GetPointer (1);
  vnfInfo1->SetScalingFactors (0.3, 0.9);

  // VNF 2 : compression service
  Ptr<VnfInfo> vnfInfo2 = VnfInfo::GetPointer (2);
  vnfInfo2->SetScalingFactors (2.2, 0.7);

  // VNF 2 : expansion service
  Ptr<VnfInfo> vnfInfo3 = VnfInfo::GetPointer (3);
  vnfInfo3->SetScalingFactors (1.4, 1.8);

  // Initial activations of VNFs.
  m_controllerApp->ActivateVnf (m_core0SwitchDevice, vnfInfo1);
  m_controllerApp->ActivateVnf (m_core0SwitchDevice, vnfInfo2);
  m_controllerApp->ActivateVnf (m_core0SwitchDevice, vnfInfo3);

  // Move VNF 1 from server to server2 at time 5 seconds.
  // Simulator::Schedule (Seconds (5), &SdnController::DeactivateVnf, m_controllerApp, m_core0SwitchDevice, vnfInfo1, 1);
  // Simulator::Schedule (Seconds (5), &SdnController::ActivateVnf, m_controllerApp, m_core0SwitchDevice, vnfInfo1, 2);
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
  sourceApp->SetFinalIpAddress (m_edge1HostAddress);
  sourceApp->SetFinalUdpPort (portNo);
  m_core0HostNode->AddApplication (sourceApp);

  // Configure the sink application on host 2.
  Ptr<SinkApp> sinkApp = CreateObject<SinkApp> ();
  sinkApp->SetLocalUdpPort (portNo);
  sinkApp->SetStartTime (Seconds (0));
  m_edge1HostNode->AddApplication (sinkApp);

  // SFC: host 1 --> VNF 1 --> VNF 2 --> host 2.
  sourceApp->SetVnfList ({1, 2, 3});
}

void
SdnNetwork::InstallVnfCopy (Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
                            Ptr<Node> serverNode, Ptr<OFSwitch13Device> serverDevice,
                            uint32_t switchToServerPortNo, uint32_t serverToSwitchPortNo,
                            Ptr<VnfInfo> vnfInfo)
{
  NS_LOG_FUNCTION (this << serverNode << serverDevice << switchNode << switchDevice << vnfInfo);

  // Create the pair of applications for this VNF
  Ptr<VnfApp> vnfApp1, vnfApp2;
  std::tie (vnfApp1, vnfApp2) = vnfInfo->CreateVnfApps ();

  // Install the first application on the network switch
  Ptr<VirtualNetDevice> virtualDevice1 = CreateObject<VirtualNetDevice> ();
  virtualDevice1->SetAddress (vnfInfo->GetMacAddr ());
  Ptr<OFSwitch13Port> logicalPort1 = switchDevice->AddSwitchPort (virtualDevice1);
  vnfApp1->SetVirtualDevice (virtualDevice1);
  switchNode->AddApplication (vnfApp1);

  // Install the second application on the server switch
  Ptr<VirtualNetDevice> virtualDevice2 = CreateObject<VirtualNetDevice> ();
  virtualDevice2->SetAddress (vnfInfo->GetMacAddr ());
  Ptr<OFSwitch13Port> logicalPort2 = serverDevice->AddSwitchPort (virtualDevice2);
  vnfApp2->SetVirtualDevice (virtualDevice2);
  serverNode->AddApplication (vnfApp2);

  // Notify the controller about this VNF copy
  m_controllerApp->NotifyVnfAttach (
    switchDevice, logicalPort1->GetPortNo (),
    serverDevice, logicalPort2->GetPortNo (),
    switchToServerPortNo, serverToSwitchPortNo, vnfInfo);
}

} // namespace ns3
