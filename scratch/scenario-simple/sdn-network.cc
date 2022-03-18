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
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (3),
                   MakeUintegerAccessor (&SdnNetwork::m_numVnfs),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("NumberNodes", "Total number of network nodes in this scenario.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (3),
                   MakeUintegerAccessor (&SdnNetwork::m_numNodes),
                   MakeUintegerChecker<uint16_t> (1));
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
  // Create the network (core and edge) switch nodes.
  m_networkNodes.Create (m_numNodes);
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream name;
      name << "node" << i;
      Names::Add (name.str (), m_networkNodes.Get (i));
    }
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (20)));
  m_networkDevices = m_switchHelper->InstallSwitch (m_networkNodes);

  // ---------------------------------------------------------------------------
  // Create the server switch nodes
  m_serverNodes.Create (m_numNodes);
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream name;
      name << "server" << i;
      Names::Add (name.str (), m_serverNodes.Get (i));
    }

  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (0)));
  m_serverDevices = m_switchHelper->InstallSwitch (m_serverNodes);

  // ---------------------------------------------------------------------------
  // Create the host nodes.
  m_hostNodes.Create (m_numNodes);
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream name;
      name << "host" << i;
      Names::Add (name.str (), m_hostNodes.Get (i));
    }

  // ---------------------------------------------------------------------------
  // Configure helper for CSMA connections.
  CsmaHelper csmaHelper;
  csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492)); // Ethernet II - PPoE
  NetDeviceContainer csmaDevices;

  // ---------------------------------------------------------------------------
  // Connect each server to its network switch (only downlink here).
  // Maximum datarate and zero delay for these links.
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate (std::numeric_limits<uint64_t>::max ())));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (Time (0)));

  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      csmaDevices = csmaHelper.Install (m_networkNodes.Get (i), m_serverNodes.Get (i));
      m_networkToServerDlPorts.push_back (m_networkDevices.Get (i)->AddSwitchPort (csmaDevices.Get (0)));
      m_serverToNetworkDlPorts.push_back (m_serverDevices.Get (i)->AddSwitchPort (csmaDevices.Get (1)));
      m_portDevices.Add (csmaDevices);
    }

  // ---------------------------------------------------------------------------
  // Connect each host to its network switch.
  // Maximum datarate and zero delay for these links.
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate (std::numeric_limits<uint64_t>::max ())));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (Time (0)));

  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      csmaDevices = csmaHelper.Install (m_networkNodes.Get (i), m_hostNodes.Get (i));
      m_networkToHostPorts.push_back (m_networkDevices.Get (i)->AddSwitchPort (csmaDevices.Get (0)));
      m_portDevices.Add (csmaDevices.Get (0));
      m_hostDevices.Add (csmaDevices.Get (1));
    }

  // ---------------------------------------------------------------------------
  // Configure IP address in hosts.
  InternetStackHelper internetStackHelper;
  Ipv4AddressHelper hostAddressHelper;
  internetStackHelper.Install (m_hostNodes);
  hostAddressHelper.SetBase ("10.0.0.0", "255.0.0.0");
  m_hostIfaces = hostAddressHelper.Assign (m_hostDevices);

  // ---------------------------------------------------------------------------
  // Notify the controller about the host nodes.
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      m_controllerApp->NotifyHostAttach (m_networkDevices.Get (i),
          m_networkToHostPorts.at (i)->GetPortNo (), m_hostDevices.Get (i));
    }

  // ---------------------------------------------------------------------------
  // Connect edge and core switches.
  // TODO: Configure DataRate and delay for connections
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

  // Core to edge 1
  csmaDevices = csmaHelper.Install (m_networkNodes.Get (0), m_networkNodes.Get (1));
  m_core0ToEdge1Port = m_networkDevices.Get (0)->AddSwitchPort (csmaDevices.Get (0));
  m_edge1Tocore0Port = m_networkDevices.Get (1)->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Core to edge 2
  csmaDevices = csmaHelper.Install (m_networkNodes.Get (0), m_networkNodes.Get (2));
  m_core0ToEdge2Port = m_networkDevices.Get (0)->AddSwitchPort (csmaDevices.Get (0));
  m_edge2Tocore0Port = m_networkDevices.Get (2)->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);

  // Edge 1 to edge 2
  csmaDevices = csmaHelper.Install (m_networkNodes.Get (1), m_networkNodes.Get (2));
  m_edge1ToEdge2Port = m_networkDevices.Get (1)->AddSwitchPort (csmaDevices.Get (0));
  m_edge2ToEdge1Port = m_networkDevices.Get (2)->AddSwitchPort (csmaDevices.Get (1));
  m_portDevices.Add (csmaDevices);
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

      for (uint32_t j = 0; j < m_numNodes; j++)
        {
          csmaDevices = csmaHelper.Install (m_networkNodes.Get (j), m_serverNodes.Get (j));
          uint32_t toServerPort = m_networkDevices.Get (j)->AddSwitchPort (csmaDevices.Get (0))->GetPortNo ();
          uint32_t toNetworkPort = m_serverDevices.Get (j)->AddSwitchPort (csmaDevices.Get (1))->GetPortNo ();
          m_portDevices.Add (csmaDevices);
          // DynamicCast<CsmaChannel> (DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());
          InstallVnfCopy (m_networkNodes.Get (j), m_networkDevices.Get (j), m_serverNodes.Get (j), m_serverDevices.Get (j),
                          toServerPort, toNetworkPort, vnfInfo);
        }
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
  m_controllerApp->ActivateVnf (m_networkDevices.Get (0), vnfInfo1);
  m_controllerApp->ActivateVnf (m_networkDevices.Get (0), vnfInfo2);
  m_controllerApp->ActivateVnf (m_networkDevices.Get (0), vnfInfo3);

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
  sourceApp->SetFinalIpAddress (m_hostIfaces.GetAddress (0));
  sourceApp->SetFinalUdpPort (portNo + 1);
  m_hostNodes.Get (0)->AddApplication (sourceApp);

  // Configure the sink application on host 2.
  Ptr<SinkApp> sinkApp = CreateObject<SinkApp> ();
  sinkApp->SetLocalUdpPort (portNo + 1);
  sinkApp->SetStartTime (Seconds (0));
  m_hostNodes.Get (0)->AddApplication (sinkApp);

  // SFC: host --> VNF 1 --> VNF 3 --> VNF 2 --> host.
  sourceApp->SetVnfList ({1, 3, 2});
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
