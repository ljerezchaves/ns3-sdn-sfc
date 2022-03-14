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

SdnNetwork::SdnNetwork () : m_controllerApp (0), m_switchHelper (0)
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
  static TypeId tid = TypeId ("ns3::SdnNetwork").SetParent<Object> ().AddConstructor<SdnNetwork> ();
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

  // Create the SDN controller.
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Names::Add ("ctrl", controllerNode);
  m_controllerApp = CreateObject<SdnController> ();
  m_controllerApp->SetSdnNetwork (Ptr<SdnNetwork> (this));
  m_switchHelper->InstallController (controllerNode, m_controllerApp);

  // Create the edge and core switches.
  m_coreSwitchNode = CreateObject<Node> ();
  m_edge1SwitchNode = CreateObject<Node> ();
  m_edge2SwitchNode = CreateObject<Node> ();
  Names::Add ("core", m_coreSwitchNode);
  Names::Add ("edge1", m_edge1SwitchNode);
  Names::Add ("edge2", m_edge2SwitchNode);
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (20)));
  m_coreSwitchDevice = m_switchHelper->InstallSwitch (m_coreSwitchNode);
  m_edge1SwitchDevice = m_switchHelper->InstallSwitch (m_edge1SwitchNode);
  m_edge2SwitchDevice = m_switchHelper->InstallSwitch (m_edge2SwitchNode);

  // Create the server switches
  m_coreServer1Node = CreateObject<Node> ();
  m_coreServer2Node = CreateObject<Node> ();
  m_edge1ServerNode = CreateObject<Node> ();
  m_edge2ServerNode = CreateObject<Node> ();
  Names::Add ("server1(core)", m_coreServer1Node);
  Names::Add ("server2(core)", m_coreServer2Node);
  Names::Add ("server(edge1)", m_edge1ServerNode);
  Names::Add ("server(edge2)", m_edge2ServerNode);
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (0)));
  m_coreServer1Device = m_switchHelper->InstallSwitch (m_coreServer1Node);
  m_coreServer2Device = m_switchHelper->InstallSwitch (m_coreServer2Node);
  m_edge1ServerDevice = m_switchHelper->InstallSwitch (m_edge1ServerNode);
  m_edge2ServerDevice = m_switchHelper->InstallSwitch (m_edge2ServerNode);

  // Create the host nodes.
  m_coreHostNode = CreateObject<Node> ();
  m_edge1HostNode = CreateObject<Node> ();
  m_edge2HostNode = CreateObject<Node> ();
  Names::Add ("host(core)", m_coreHostNode);
  Names::Add ("host(edge1)", m_edge1HostNode);
  Names::Add ("host(edge2)", m_edge2HostNode);

  // Configure helper for CSMA connections;
  CsmaHelper csmaHelper;
  csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // ---------------------------------------------------------------------------
  // Connect each edge switch to the core switch.
  // TODO: Configure DataRate and delay for connections
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Core to edge 1 (link A)
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_edge1SwitchNode);
  m_coreToEdge1APort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1ToCoreAPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to edge 1 (link B)
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_edge1SwitchNode);
  m_coreToEdge1BPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge1ToCoreBPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to edge 2 (link A)
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_edge2SwitchNode);
  m_coreToEdge2APort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToCoreAPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to edge 2 (link B)
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_edge2SwitchNode);
  m_coreToEdge2BPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToCoreBPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 1 to edge 2
  csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge2SwitchNode);
  m_edge1ToEdge2Port = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToEdge1Port = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // ---------------------------------------------------------------------------
  // Connect each server to the proper network switch (uplink and downlink)
  // TODO: Configure DataRate and delay for connections
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Core to server 1 downlink.
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_coreServer1Node);
  m_coreToServer1DlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCoreDlinkPort = m_coreServer1Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to server 1 uplink_1.
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_coreServer1Node);
  m_coreToServer1UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCoreUlinkPort = m_coreServer1Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_coreToServer1Ulink1Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());
  //modifica 186-198
  //Core to server 1 uplink_2.
  m_coreToServer1UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCoreUlinkPort = m_coreServer1Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_coreToServer1Ulink2Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Core to server 1 uplink_3.
  m_coreToServer1UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server1ToCoreUlinkPort = m_coreServer1Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_coreToServer1Ulink3Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Core to server 2 downlink.
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_coreServer2Node);
  m_coreToServer2DlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCoreDlinkPort = m_coreServer2Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to server 2 uplink_1.
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_coreServer2Node);
  m_coreToServer2UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCoreUlinkPort = m_coreServer2Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_coreToServer2Ulink1Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());
  //modifica214-226
  // Core to server 2 uplink_2.
  m_coreToServer2UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCoreUlinkPort = m_coreServer2Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_coreToServer2Ulink2Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Core to server 2 uplink_3.
  m_coreToServer2UlinkPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_server2ToCoreUlinkPort = m_coreServer2Device->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_coreToServer2Ulink3Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Edge 1 to server downlink.
  csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge1ServerNode);
  m_edge1ToServerDlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1DlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 1 to server uplink_1.
  csmaDevices = csmaHelper.Install (m_edge1SwitchNode, m_edge1ServerNode);
  m_edge1ToServerUlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1UlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_edge1ToServerUlink1Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());
  //modifica 242-254
  // Edge 1 to server uplink_2.
  m_edge1ToServerUlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1UlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_edge1ToServerUlink2Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Edge 1 to server uplink_3.
  m_edge1ToServerUlinkPort = m_edge1SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge1UlinkPort = m_edge1ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_edge1ToServerUlink3Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Edge 2 to server downlink.
  csmaDevices = csmaHelper.Install (m_edge2SwitchNode, m_edge2ServerNode);
  m_edge2ToServerDlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2DlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 2 to server uplink_1.
  csmaDevices = csmaHelper.Install (m_edge2SwitchNode, m_edge2ServerNode);
  m_edge2ToServerUlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2UlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_edge2ToServerUlink1Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());
  //modifica 270-282
  // Edge 2 to server uplink_2.
  m_edge2ToServerUlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2UlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_edge2ToServerUlink2Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // Edge 2 to server uplink_3.
  m_edge2ToServerUlinkPort = m_edge2SwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_serverToEdge2UlinkPort = m_edge2ServerDevice->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
  m_edge2ToServerUlink3Channel =
      DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

  // ---------------------------------------------------------------------------
  // Connect each host to the proper network switch
  // TODO: Configure DataRate and delay for connections
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1Gbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Core to host
  csmaDevices = csmaHelper.Install (m_coreSwitchNode, m_coreHostNode);
  m_coreToHostPort = m_coreSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
  m_portDevices.Add (csmaDevices.Get (0));
  m_coreHostDevice = csmaDevices.Get (1);
  m_hostDevices.Add (m_coreHostDevice);

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
  internetStackHelper.Install (m_coreHostNode);
  internetStackHelper.Install (m_edge1HostNode);
  internetStackHelper.Install (m_edge2HostNode);
  hostAddressHelper.SetBase ("10.0.0.0", "255.0.0.0");
  hostIfaces = hostAddressHelper.Assign (m_hostDevices);
  m_coreHostAddress = hostIfaces.GetAddress (0);
  m_edge1HostAddress = hostIfaces.GetAddress (1);
  m_edge2HostAddress = hostIfaces.GetAddress (2);

  // Notify the controller about the host nodes.
  m_controllerApp->NotifyHostAttach (m_coreSwitchDevice, m_coreToHostPort->GetPortNo (),
                                     m_coreHostDevice);
  m_controllerApp->NotifyHostAttach (m_edge1SwitchDevice, m_edge1ToHostPort->GetPortNo (),
                                     m_edge1HostDevice);
  m_controllerApp->NotifyHostAttach (m_edge2SwitchDevice, m_edge2ToHostPort->GetPortNo (),
                                     m_edge2HostDevice);
}

void
SdnNetwork::ConfigureFunctions (void)
{
  NS_LOG_FUNCTION (this);

  // Create and configure the VNF (ID 1).
  Ptr<VnfInfo> vnfInfo1 = CreateObject<VnfInfo> (1);
  vnfInfo1->Set1stScaling (0.3);
  vnfInfo1->Set2ndScaling (0.9 / 0.3); //Network Service
  SdnController::SaveArpEntry (vnfInfo1->GetIpAddr (), vnfInfo1->GetMacAddr ());

  // Install a copy of this VNF in switch/server1 and switch/server2
  InstallVnfCopy (m_coreSwitchNode, m_coreSwitchDevice, m_coreServer1Node, m_coreServer1Device,
                  m_coreToServer1UlinkPort->GetPortNo (),
                  m_server1ToCoreDlinkPort->GetPortNo (), vnfInfo1, 1);
  InstallVnfCopy (m_coreSwitchNode, m_coreSwitchDevice, m_coreServer2Node, m_coreServer2Device,
                  m_coreToServer2UlinkPort->GetPortNo (),
                  m_server2ToCoreDlinkPort->GetPortNo (), vnfInfo1, 2);
  InstallVnfCopy (m_edge1SwitchNode, m_edge1SwitchDevice, m_edge1ServerNode, m_edge1ServerDevice,
                  m_edge1ToServerUlinkPort->GetPortNo (),
                  m_serverToEdge1DlinkPort->GetPortNo (), vnfInfo1, 1);
  InstallVnfCopy (m_edge2SwitchNode, m_edge2SwitchDevice, m_edge2ServerNode, m_edge2ServerDevice,
                  m_edge2ToServerUlinkPort->GetPortNo (),
                  m_serverToEdge2DlinkPort->GetPortNo (), vnfInfo1, 1);
  
  // Create and configure the VNF (ID 2).
  Ptr<VnfInfo> vnfInfo2 = CreateObject<VnfInfo> (2);
  vnfInfo2->Set1stScaling (2.2);
  vnfInfo2->Set2ndScaling (0.7 / 2.2); // Compression Service
  SdnController::SaveArpEntry (vnfInfo2->GetIpAddr (), vnfInfo2->GetMacAddr ());
// Install a copy of this VNF in switch/server1 and switch/server2
  InstallVnfCopy (m_coreSwitchNode, m_coreSwitchDevice, m_coreServer1Node, m_coreServer1Device,
                  m_coreToServer1UlinkPort->GetPortNo (),
                  m_server1ToCoreDlinkPort->GetPortNo (), vnfInfo2, 1);
  InstallVnfCopy (m_coreSwitchNode, m_coreSwitchDevice, m_coreServer2Node, m_coreServer2Device,
                  m_coreToServer2UlinkPort->GetPortNo (),
                  m_server2ToCoreDlinkPort->GetPortNo (), vnfInfo2, 2);
  InstallVnfCopy (m_edge1SwitchNode, m_edge1SwitchDevice, m_edge1ServerNode, m_edge1ServerDevice,
                  m_edge1ToServerUlinkPort->GetPortNo (),
                  m_serverToEdge1DlinkPort->GetPortNo (), vnfInfo2, 1);
  InstallVnfCopy (m_edge2SwitchNode, m_edge2SwitchDevice, m_edge2ServerNode, m_edge2ServerDevice,
                  m_edge2ToServerUlinkPort->GetPortNo (),
                  m_serverToEdge2DlinkPort->GetPortNo (), vnfInfo2, 1);
  
  // Create and configure the VNF (ID 3).
  Ptr<VnfInfo> vnfInfo3 = CreateObject<VnfInfo> (3);
  vnfInfo3->Set1stScaling (1.4);
  vnfInfo3->Set2ndScaling (1.8 / 1.4); //Expansion Service
  SdnController::SaveArpEntry (vnfInfo1->GetIpAddr (), vnfInfo1->GetMacAddr ());

  // Install a copy of this VNF in switch/server1 and switch/server2
  InstallVnfCopy (m_coreSwitchNode, m_coreSwitchDevice, m_coreServer1Node, m_coreServer1Device,
                  m_coreToServer1UlinkPort->GetPortNo (),
                  m_server1ToCoreDlinkPort->GetPortNo (), vnfInfo3, 1);
  InstallVnfCopy (m_coreSwitchNode, m_coreSwitchDevice, m_coreServer2Node, m_coreServer2Device,
                  m_coreToServer2UlinkPort->GetPortNo (),
                  m_server2ToCoreDlinkPort->GetPortNo (), vnfInfo3, 2);
  InstallVnfCopy (m_edge1SwitchNode, m_edge1SwitchDevice, m_edge1ServerNode, m_edge1ServerDevice,
                  m_edge1ToServerUlinkPort->GetPortNo (),
                  m_serverToEdge1DlinkPort->GetPortNo (), vnfInfo3, 1);
  InstallVnfCopy (m_edge2SwitchNode, m_edge2SwitchDevice, m_edge2ServerNode, m_edge2ServerDevice,
                  m_edge2ToServerUlinkPort->GetPortNo (),
                  m_serverToEdge2DlinkPort->GetPortNo (), vnfInfo3, 1);
  
  // Activate VNF 1 in server1 and VNF 2 in server2
  m_controllerApp->ActivateVnf (m_coreSwitchDevice, VnfInfo::GetPointer (1), 1);
  m_controllerApp->ActivateVnf (m_coreSwitchDevice, VnfInfo::GetPointer (2), 1);
  m_controllerApp->ActivateVnf (m_coreSwitchDevice, VnfInfo::GetPointer (3), 1);

  // Move VNF 1 from server1 to server2 at time 5 seconds.
  Simulator::Schedule (Seconds (5), &SdnController::DeactivateVnf, m_controllerApp, m_coreSwitchDevice,
                       vnfInfo1, 1);
  Simulator::Schedule (Seconds (5), &SdnController::ActivateVnf, m_controllerApp, m_coreSwitchDevice,
                       vnfInfo1, 2);
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
  m_coreHostNode->AddApplication (sourceApp);

  // Configure the sink application on host 2.
  Ptr<SinkApp> sinkApp = CreateObject<SinkApp> ();
  sinkApp->SetLocalUdpPort (portNo);
  sinkApp->SetStartTime (Seconds (0));
  m_edge1HostNode->AddApplication (sinkApp);

  // SFC: host 1 --> VNF 1 --> VNF 2 --> host 2.
  sourceApp->SetVnfList ({1});
}

void
SdnNetwork::InstallVnfCopy (Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
                            Ptr<Node> serverNode, Ptr<OFSwitch13Device> serverDevice,
                            uint32_t switchToServerPortNo, uint32_t serverToSwitchPortNo,
                            Ptr<VnfInfo> vnfInfo, int serverId)
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
  m_controllerApp->NotifyVnfAttach (switchDevice, logicalPort1->GetPortNo (), serverDevice,
                                    logicalPort2->GetPortNo (), switchToServerPortNo,
                                    serverToSwitchPortNo, vnfInfo, serverId);
}

} // namespace ns3
