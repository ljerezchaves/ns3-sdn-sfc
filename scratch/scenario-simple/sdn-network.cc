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
    .AddAttribute ("NumberVnfs", "Total number of VNFs.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (5),
                   MakeUintegerAccessor (&SdnNetwork::m_numVnfs),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("NumberNodes", "Total number of network nodes.",
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

  Object::DoDispose ();
}

uint32_t
SdnNetwork::GetNetworkSwitchDpId (uint32_t nodeId) const
{
  NS_LOG_FUNCTION (this << nodeId);

  return m_networkSwitchDevs.Get (nodeId)->GetDatapathId ();
}

uint32_t
SdnNetwork::GetServerSwitchDpId (uint32_t serverId) const
{
  NS_LOG_FUNCTION (this << serverId);

  return m_serverSwitchDevs.Get (serverId)->GetDatapathId ();
}

void
SdnNetwork::EnablePcap (bool enable)
{
  NS_LOG_FUNCTION (this);

  if (enable)
    {
      m_csmaHelper.EnablePcap ("port", m_portDevices, true);
      m_csmaHelper.EnablePcap ("host", m_hostDevices, true);
      m_switchHelper->EnableOpenFlowPcap ("ofp", false);
    }
}

void
SdnNetwork::NotifyConstructionCompleted (void)
{
  NS_LOG_FUNCTION (this);

  // Create and configure the helpers.
  m_switchHelper = CreateObject<OFSwitch13InternalHelper> ();
  m_csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1492));

  // Configure network topology and VNFs (respect this order!).
  ConfigureTopology ();
  ConfigureFunctions ();

  // Let's connect the OpenFlow switches to the controller. From this point
  // on it is not possible to change the OpenFlow network configuration.
  m_switchHelper->CreateOpenFlowChannels ();

  Object::NotifyConstructionCompleted ();
}

void
SdnNetwork::ConfigureTopology (void)
{
  NS_LOG_FUNCTION (this);

  // ---------------------------------------------------------------------------
  // Create the SDN controller.
  Ptr<Node> controllerNode = CreateObject<Node> ();
  Names::Add ("ctrl", controllerNode);
  m_controllerApp = CreateObject<SdnController> (Ptr<SdnNetwork> (this));
  m_switchHelper->InstallController (controllerNode, m_controllerApp);

  // ---------------------------------------------------------------------------
  // Create the network (core and edge switch) nodes.
  m_networkNodes.Create (m_numNodes);
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream name;
      name << "node" << i;
      Names::Add (name.str (), m_networkNodes.Get (i));
    }
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (20)));
  m_networkSwitchDevs = m_switchHelper->InstallSwitch (m_networkNodes);

  // ---------------------------------------------------------------------------
  // Connect network switches in full topology.
  // FIXME: Initial DataRate and delay for network connections.
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("10Mbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));

  // Initialize the pointer matrix
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      m_networkToNetworkPorts.push_back (PortVector_t ());
      m_networkToNetworkChannels.push_back (ChannelVector_t ());
      for (uint32_t j = 0; j < m_numNodes; j++)
        {
          m_networkToNetworkPorts.at (i).push_back (0);
          m_networkToNetworkChannels.at (i).push_back (0);
        }
    }

  // Connect each pair of network switches
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      for (uint32_t j = 0; j < m_numNodes; j++)
        {
          if (i != j)
            {
              NetDeviceContainer csmaDevices = m_csmaHelper.Install (m_networkNodes.Get (i), m_networkNodes.Get (j));
              m_networkToNetworkPorts[i][j] = m_networkSwitchDevs.Get (i)->AddSwitchPort (csmaDevices.Get (0));
              m_networkToNetworkPorts[j][i] = m_networkSwitchDevs.Get (j)->AddSwitchPort (csmaDevices.Get (1));
              m_portDevices.Add (csmaDevices);

              Ptr<CsmaChannel> csmaChannel = DynamicCast<CsmaChannel> (
                DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());
              m_networkToNetworkChannels[i][j] = csmaChannel;
              m_networkToNetworkChannels[j][i] = csmaChannel;
            }
        }
    }

  // ---------------------------------------------------------------------------
  // Create the server (switch) nodes
  m_serverNodes.Create (m_numNodes);
  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      std::ostringstream name;
      name << "server" << i;
      Names::Add (name.str (), m_serverNodes.Get (i));
    }
  m_switchHelper->SetDeviceAttribute ("TcamDelay", TimeValue (MicroSeconds (0)));
  m_serverSwitchDevs = m_switchHelper->InstallSwitch (m_serverNodes);

  // ---------------------------------------------------------------------------
  // Connect each server to its network switch (only downlink connection here).
  // Maximum datarate and zero delay for these links.
  DataRate maxDataRate (std::numeric_limits<uint64_t>::max ());
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (maxDataRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (Time (0)));

  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      NetDeviceContainer csmaDevices = m_csmaHelper.Install (m_networkNodes.Get (i), m_serverNodes.Get (i));
      m_networkSwitchDevs.Get (i)->AddSwitchPort (csmaDevices.Get (0));
      m_serverToNetworkDlinkPorts.push_back (m_serverSwitchDevs.Get (i)->AddSwitchPort (csmaDevices.Get (1)));
      m_portDevices.Add (csmaDevices);
    }

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
  // Connect each host to its network switch.
  // Maximum datarate and zero delay for these links.
  m_csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (maxDataRate));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (Time (0)));

  for (uint32_t i = 0; i < m_numNodes; i++)
    {
      NetDeviceContainer csmaDevices = m_csmaHelper.Install (m_networkNodes.Get (i), m_hostNodes.Get (i));
      m_networkToHostPorts.push_back (m_networkSwitchDevs.Get (i)->AddSwitchPort (csmaDevices.Get (0)));
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
      m_controllerApp->NotifyHostAttach (
        m_networkSwitchDevs.Get (i), m_networkToHostPorts.at (i)->GetPortNo (), m_hostDevices.Get (i));
    }
}

void
SdnNetwork::ConfigureFunctions (void)
{
  NS_LOG_FUNCTION (this);

  // Create the VNFs.
  for (uint16_t i = 0; i < m_numVnfs; i++)
    {
      Ptr<VnfInfo> vnfInfo = CreateObject<VnfInfo> (i);
      SdnController::SaveArpEntry (vnfInfo->GetIpAddr (), vnfInfo->GetMacAddr ());
    }

  // FIXME: Initial DataRate and delay for VNF uplink connections.
  m_csmaHelper.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  m_csmaHelper.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (250)));

  // Initialize the pointer matrix.
  for (uint32_t n = 0; n < m_numNodes; n++)
    {
      m_networkToVnfUlinkPorts.push_back (PortVector_t ());
      m_networkToVnfUlinkChannels.push_back (ChannelVector_t ());
      for (uint16_t v = 0; v < m_numVnfs; v++)
        {
          m_networkToVnfUlinkPorts.at (n).push_back (0);
          m_networkToVnfUlinkChannels.at (n).push_back (0);
        }
    }

  // Install a copy of each VNF on each server.
  for (uint32_t n = 0; n < m_numNodes; n++)
    {
      // Getting pointer to network and server nodes and devices.
      Ptr<Node> networkNode = m_networkNodes.Get (n);
      Ptr<Node> serverNode = m_serverNodes.Get (n);
      Ptr<OFSwitch13Device> networkSwitchDevice = m_networkSwitchDevs.Get (n);
      Ptr<OFSwitch13Device> serverSwitchDevice = m_serverSwitchDevs.Get (n);
      uint32_t downlinkPortNo = m_serverToNetworkDlinkPorts.at (n)->GetPortNo ();

      for (uint16_t v = 0; v < m_numVnfs; v++)
        {
          Ptr<VnfInfo> vnfInfo = VnfInfo::GetPointer (v);

          // Create the individual connection from network to server for this VNF.
          NetDeviceContainer csmaDevices = m_csmaHelper.Install (networkNode, serverNode);
          m_networkToVnfUlinkPorts[n][v] = networkSwitchDevice->AddSwitchPort (csmaDevices.Get (0));
          serverSwitchDevice->AddSwitchPort (csmaDevices.Get (1))->GetPortNo ();
          m_portDevices.Add (csmaDevices);
          m_networkToVnfUlinkChannels[n][v] = DynamicCast<CsmaChannel> (
            DynamicCast<CsmaNetDevice> (csmaDevices.Get (0))->GetChannel ());

          // Create the pair of applications for this VNF.
          Ptr<VnfApp> vnfApp1, vnfApp2;
          std::tie (vnfApp1, vnfApp2) = vnfInfo->CreateVnfApps ();

          // Install the first application on the network node.
          Ptr<VirtualNetDevice> virtualDevice1 = CreateObject<VirtualNetDevice> ();
          virtualDevice1->SetAddress (vnfInfo->GetMacAddr ());
          Ptr<OFSwitch13Port> logicalPort1 = networkSwitchDevice->AddSwitchPort (virtualDevice1);
          vnfApp1->SetVirtualDevice (virtualDevice1);
          networkNode->AddApplication (vnfApp1);

          // Install the second application on the server node.
          Ptr<VirtualNetDevice> virtualDevice2 = CreateObject<VirtualNetDevice> ();
          virtualDevice2->SetAddress (vnfInfo->GetMacAddr ());
          Ptr<OFSwitch13Port> logicalPort2 = serverSwitchDevice->AddSwitchPort (virtualDevice2);
          vnfApp2->SetVirtualDevice (virtualDevice2);
          serverNode->AddApplication (vnfApp2);

          // Notify the controller about this VNF copy.
          m_controllerApp->NotifyVnfAttach (
            networkSwitchDevice, logicalPort1->GetPortNo (),
            serverSwitchDevice, logicalPort2->GetPortNo (),
            m_networkToVnfUlinkPorts[n][v]->GetPortNo (),
            downlinkPortNo, vnfInfo);
        }
    }
}

void
SdnNetwork::NewServiceTraffic (
  uint32_t srcHostId, uint32_t dstHostId,
  std::vector<uint8_t> vnfList, Time startTime, Time stopTime,
  std::string pktSizeDesc, std::string pktIntervalDesc)
{
  NS_LOG_FUNCTION (this << srcHostId << dstHostId << startTime << stopTime);

  static uint16_t serviceFlowCounter = 0;

  // Increase the flow counter
  serviceFlowCounter++;

  // Define UDP port numbers (which are used as flow IDs)
  uint16_t srcPortNo = 10000 + serviceFlowCounter;
  uint16_t dstPortNo = 20000 + serviceFlowCounter;

  // Create the source application
  Ptr<SourceApp> sourceApp = CreateObjectWithAttributes<SourceApp> (
    "LocalIpAddress", Ipv4AddressValue (m_hostIfaces.GetAddress (srcHostId)),
    "LocalUdpPort",   UintegerValue (srcPortNo),
    "FinalIpAddress", Ipv4AddressValue (m_hostIfaces.GetAddress (dstHostId)),
    "FinalUdpPort",   UintegerValue (dstPortNo));
  sourceApp->SetVnfList (vnfList);
  sourceApp->SetStartTime (startTime);
  sourceApp->SetStopTime (stopTime);
  if (!pktSizeDesc.empty ())
    {
      sourceApp->SetAttribute ("PktSize", StringValue (pktSizeDesc));
    }
  if (!pktIntervalDesc.empty ())
    {
      sourceApp->SetAttribute ("PktInterval", StringValue (pktIntervalDesc));
    }
  m_hostNodes.Get (srcHostId)->AddApplication (sourceApp);

  // Create the sink application
  Ptr<SinkApp> sinkApp = CreateObjectWithAttributes<SinkApp> (
    "LocalIpAddress", Ipv4AddressValue (m_hostIfaces.GetAddress (dstHostId)),
    "LocalUdpPort",   UintegerValue (dstPortNo));
  sinkApp->SetStartTime (Seconds (0));
  m_hostNodes.Get (dstHostId)->AddApplication (sinkApp);

  // Notify the controller about this new traffic
  m_controllerApp->NotifyNewServiceTraffic (
    srcHostId, dstHostId, srcPortNo, dstPortNo, vnfList, startTime, stopTime);
}

void
SdnNetwork::NewBackgroundTraffic (
  uint32_t srcHostId, uint32_t dstHostId, Time startTime, Time stopTime,
  std::string pktSizeDesc, std::string pktIntervalDesc)
{
  NS_LOG_FUNCTION (this << srcHostId << dstHostId << startTime << stopTime);

  static uint16_t backgroundFlowCounter = 0;

  // Increase the flow counter
  backgroundFlowCounter++;

  // Define UDP port numbers (which are used as flow IDs)
  uint16_t srcPortNo = 30000 + backgroundFlowCounter;
  uint16_t dstPortNo = 40000 + backgroundFlowCounter;

  // Create the source application
  Ptr<SourceApp> sourceApp = CreateObjectWithAttributes<SourceApp> (
    "LocalIpAddress", Ipv4AddressValue (m_hostIfaces.GetAddress (srcHostId)),
    "LocalUdpPort",   UintegerValue (srcPortNo),
    "FinalIpAddress", Ipv4AddressValue (m_hostIfaces.GetAddress (dstHostId)),
    "FinalUdpPort",   UintegerValue (dstPortNo));
  sourceApp->SetStartTime (startTime);
  sourceApp->SetStopTime (stopTime);
  if (!pktSizeDesc.empty ())
    {
      sourceApp->SetAttribute ("PktSize", StringValue (pktSizeDesc));
    }
  if (!pktIntervalDesc.empty ())
    {
      sourceApp->SetAttribute ("PktInterval", StringValue (pktIntervalDesc));
    }
  m_hostNodes.Get (srcHostId)->AddApplication (sourceApp);

  // Create the sink application
  Ptr<SinkApp> sinkApp = CreateObjectWithAttributes<SinkApp> (
    "LocalIpAddress", Ipv4AddressValue (m_hostIfaces.GetAddress (dstHostId)),
    "LocalUdpPort",   UintegerValue (dstPortNo));
  sinkApp->SetStartTime (Seconds (0));
  m_hostNodes.Get (dstHostId)->AddApplication (sinkApp);

  // Notify the controller about this new traffic
  m_controllerApp->NotifyNewBackgroundTraffic (
    srcHostId, dstHostId, srcPortNo, dstPortNo, startTime, stopTime);
}

} // namespace ns3
