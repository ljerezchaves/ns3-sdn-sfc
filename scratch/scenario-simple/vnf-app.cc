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

    .AddAttribute ("Ipv4Address", "VNF IPv4 address.",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&VnfApp::m_ipv4Address),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("TargetAddress", "Destination socket address.",
                   AddressValue (),
                   MakeAddressAccessor (&VnfApp::m_targetAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&VnfApp::m_port),
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
VnfApp::SetTargetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_targetAddress = address;
}

void
VnfApp::SetLocalAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_ipv4Address = address;
}

void
VnfApp::SetLocalPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_port = port;
}

void
VnfApp::SetVirtualDevice (Ptr<VirtualNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);

  m_logicalPort = device;
  m_logicalPort->SetSendCallback (MakeCallback (&VnfApp::ProcessPacket, this));
}

bool
VnfApp::ProcessPacket (Ptr<Packet> packet, const Address& source,
                       const Address& dest, uint16_t protocolNo)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNo);

  // The packet got here with IP and UDP headers. Let's remove them first.
  Ipv4Header ipHeader;
  packet->RemoveHeader (ipHeader);
  NS_LOG_DEBUG (ipHeader);

  UdpHeader udpHeader;
  packet->RemoveHeader (udpHeader);
  NS_LOG_DEBUG (udpHeader);

  // // Create the new packet with adjusted size.
  // int newPacketSize = packet->GetSize () * m_pktSizeScale;
  // Ptr<Packet> newPacket = Create<Packet> (newPacketSize);


  // // Encapsulating the packet withing UDP, IP and Ethernet.
  // UdpHeader newUdpHeader;
  // newUdpHeader.SetSourcePort (m_port);
  // newUdpHeader.SetDestinationPort (m_port); // FIXME


  // // ipHeader.SetDestination ();



  // // Send the new packet to the OpenFlow switch over the logical port.
  // m_logicalPort->Receive (newPacket, Ipv4L3Protocol::PROT_NUMBER, Mac48Address (),
  //                         Mac48Address (), NetDevice::PACKET_HOST);

  return true;
}

void
VnfApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_logicalPort = 0;
  Application::DoDispose ();
}

} // namespace ns3
