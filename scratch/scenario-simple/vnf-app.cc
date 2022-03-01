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

#include "vnf-app.h"
#include "sdn-controller.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VnfApp");
NS_OBJECT_ENSURE_REGISTERED (VnfApp);

VnfApp::VnfApp ()
  : m_sendEvent (EventId ()),
  m_logicalPort (0)
{
  NS_LOG_FUNCTION (this);
}

VnfApp::~VnfApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
VnfApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VnfApp")
    .SetParent<Application> ()
    .AddConstructor<VnfApp> ()

    .AddAttribute ("LocalIpAddress", "Local IPv4 address.",
                   Ipv4AddressValue (Ipv4Address ()),
                   MakeIpv4AddressAccessor (&VnfApp::m_localIpAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("LocalUdpPort", "Local UDP port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&VnfApp::m_localUdpPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("NextIpAddress", "Next IPv4 address.",
                   Ipv4AddressValue (Ipv4Address ()),
                   MakeIpv4AddressAccessor (&VnfApp::m_nextIpAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("NextUdpPort", "Next UDP port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&VnfApp::m_nextUdpPort),
                   MakeUintegerChecker<uint16_t> ())

    .AddAttribute ("PktSizeScalingFactor",
                   "A scaling factor for the packet size.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&VnfApp::m_pktSizeScale),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

void
VnfApp::SetLocalIpAddress (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_localIpAddress = address;
}

void
VnfApp::SetLocalUdpPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_localUdpPort = port;
}

void
VnfApp::SetNextIpAddress (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_nextIpAddress = address;
}

void
VnfApp::SetNextUdpPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_nextUdpPort = port;
}

void
VnfApp::SetVirtualDevice (Ptr<VirtualNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);

  m_logicalPort = device;
  m_logicalPort->SetSendCallback (MakeCallback (&VnfApp::ProcessPacket, this));
}

bool
VnfApp::ProcessPacket (Ptr<Packet> inPacket, const Address& srcMac,
                       const Address& dstMac, uint16_t protocolNo)
{
  NS_LOG_FUNCTION (this << inPacket << srcMac << dstMac << protocolNo);

  // The packet got here with the IP and UDP headers. Let's remove them first.
  RemoveHeaders (inPacket);

  NS_LOG_INFO ("VNF app received a packet of " << inPacket->GetSize () << " bytes.");

  // Create the new packet with adjusted size.
  int newPacketSize = inPacket->GetSize () * m_pktSizeScale;
  Ptr<Packet> outPacket = Create<Packet> (newPacketSize);

  NS_LOG_INFO ("VNF app will send a packet of " << outPacket->GetSize () << " bytes.");

  // Insert new UDP, IP and Ethernet headers.
  Mac48Address nextMacAddr = SdnController::GetArpEntry (m_nextIpAddress);
  InsertHeaders (outPacket, m_localIpAddress, m_nextIpAddress, m_localUdpPort,
                 m_nextUdpPort, Mac48Address::ConvertFrom (dstMac), nextMacAddr);

  // // Send the new packet to the OpenFlow switch over the logical port.
  m_logicalPort->Receive (outPacket, Ipv4L3Protocol::PROT_NUMBER,
                          dstMac, srcMac, NetDevice::PACKET_HOST);

  return true;
}

void
VnfApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_logicalPort = 0;
  Application::DoDispose ();
}

void
VnfApp::RemoveHeaders (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);

  // Remove the IPv4 header.
  Ipv4Header ipHeader;
  if (Node::ChecksumEnabled ())
    {
      ipHeader.EnableChecksum ();
    }
  packet->RemoveHeader (ipHeader);
  if (ipHeader.GetPayloadSize () < packet->GetSize ())
    {
      // Trim any residual frame padding from underlying devices.
      packet->RemoveAtEnd (packet->GetSize () - ipHeader.GetPayloadSize ());
    }
  if (!ipHeader.IsChecksumOk ())
    {
      NS_LOG_WARN ("Bad checksum.");
    }

  // Remove the UDP header.
  UdpHeader udpHeader;
  if (Node::ChecksumEnabled ())
    {
      udpHeader.EnableChecksums ();
    }
  udpHeader.InitializeChecksum (
    ipHeader.GetSource (), ipHeader.GetDestination (), UdpL4Protocol::PROT_NUMBER);
  packet->RemoveHeader (udpHeader);
  if (!udpHeader.IsChecksumOk ())
    {
      NS_LOG_WARN ("Bad checksum.");
    }

  NS_ASSERT_MSG (ipHeader.GetDestination () == m_localIpAddress, "Inconsistent IP address.");
  NS_ASSERT_MSG (udpHeader.GetDestinationPort () == m_localUdpPort, "Inconsistente UDP port.");
}

void
VnfApp::InsertHeaders (
  Ptr<Packet> packet, Ipv4Address srcIp, Ipv4Address dstIp, uint16_t srcPort,
  uint16_t dstPort, Mac48Address srcMac, Mac48Address dstMac)
{
  NS_LOG_FUNCTION (this << packet << srcIp << dstIp << srcPort << dstPort);

  // Insert the UDP header
  UdpHeader udpHeader;
  if (Node::ChecksumEnabled ())
    {
      udpHeader.EnableChecksums ();
      udpHeader.InitializeChecksum (srcIp, dstIp, UdpL4Protocol::PROT_NUMBER);
    }
  udpHeader.SetDestinationPort (dstPort);
  udpHeader.SetSourcePort (srcPort);
  packet->AddHeader (udpHeader);

  // Insert the IP header
  Ipv4Header ipHeader;
  if (Node::ChecksumEnabled ())
    {
      ipHeader.EnableChecksum ();
    }
  ipHeader.SetSource (srcIp);
  ipHeader.SetDestination (dstIp);
  ipHeader.SetProtocol (UdpL4Protocol::PROT_NUMBER);
  ipHeader.SetPayloadSize (packet->GetSize ());
  ipHeader.SetTtl (32);
  ipHeader.SetTos (0);
  ipHeader.SetDontFragment ();
  ipHeader.SetIdentification (0);
  packet->AddHeader (ipHeader);

  // All Ethernet frames must carry a minimum payload of 46 bytes. We need to
  // pad out if we don't have enough bytes. These must be real bytes since they
  // will be written to pcap files and compared in regression trace files.
  if (packet->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
      packet->AddAtEnd (padd);
    }

  // Insert the Ethernet header and trailer
  EthernetHeader header (false);
  header.SetSource (srcMac);
  header.SetDestination (dstMac);
  header.SetLengthType (Ipv4L3Protocol::PROT_NUMBER);
  packet->AddHeader (header);

  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);
}

} // namespace ns3
