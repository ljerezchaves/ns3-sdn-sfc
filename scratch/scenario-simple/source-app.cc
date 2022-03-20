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

#include "source-app.h"
#include "sfc-tag.h"
#include "vnf-info.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SourceApp");
NS_OBJECT_ENSURE_REGISTERED (SourceApp);

// Static traffic ID counter.
uint16_t SourceApp::m_trafficCounter = 0;

SourceApp::SourceApp ()
  : m_socket (0),
    m_sendEvent (EventId ()),
    m_trafficId (m_trafficCounter++)
{
  NS_LOG_FUNCTION (this);
}

SourceApp::~SourceApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SourceApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SourceApp")
    .SetParent<Application> ()
    .AddConstructor<SourceApp> ()

    .AddAttribute ("LocalUdpPort", "Local UDP port.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SourceApp::m_localUdpPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("FinalIpAddress", "Final IPv4 address.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&SourceApp::m_finalIpAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("FinalUdpPort", "Final UDP port.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SourceApp::m_finalUdpPort),
                   MakeUintegerChecker<uint16_t> ())

    // These attributes must be configured for the desired traffic pattern.
    .AddAttribute ("PktInterval",
                   "A random variable for the packet inter-arrival time [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&SourceApp::m_pktInterRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("PktSize",
                   "A random variable for the packet size [bytes].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=100]"),
                   MakePointerAccessor (&SourceApp::m_pktSizeRng),
                   MakePointerChecker <RandomVariableStream> ())

    // Trace sources for start and stop events
    .AddTraceSource ("AppStart", "Application start trace source.",
                     MakeTraceSourceAccessor (&SourceApp::m_appStartTrace),
                     "ns3::Uni5onClient::SourceAppTracedCallback")
    .AddTraceSource ("AppStop", "Application stop trace source.",
                     MakeTraceSourceAccessor (&SourceApp::m_appStopTrace),
                     "ns3::Uni5onClient::SourceAppTracedCallback")
  ;
  return tid;
}

uint16_t
SourceApp::GetTrafficId (void) const
{
  NS_LOG_FUNCTION (this);

  return m_trafficId;
}

void
SourceApp::SetLocalUdpPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_localUdpPort = port;
}

void
SourceApp::SetFinalIpAddress (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_finalIpAddress = address;
}

void
SourceApp::SetFinalUdpPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_finalUdpPort = port;
}

void
SourceApp::SetVnfList (std::vector<uint8_t> vnfList)
{
  NS_LOG_FUNCTION (this << vnfList);
  NS_ASSERT_MSG (vnfList.size () <= 8, "Maximum of 8 VNFs allowed in SFC.");

  m_vnfList = vnfList;
}

void
SourceApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  Application::DoDispose ();
}

void
SourceApp::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the TX UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_localUdpPort));
  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());

  // Schedule the first packet transmission.
  m_sendEvent.Cancel ();
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &SourceApp::SendPacket, this, newSize);

  // Fire trace source
  m_appStartTrace (this);
}

void
SourceApp::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  // Cancel any further packet transmission.
  m_sendEvent.Cancel ();

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->Dispose ();
      m_socket = 0;
    }

  // Fire trace source
  m_appStopTrace (this);
}

void
SourceApp::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  Ptr<Packet> packet = Create<Packet> (size);

  // Create the SFC packet tag and identify the next address based on the tag.
  InetSocketAddress finalAddress (m_finalIpAddress, m_finalUdpPort);
  SfcTag sfcTag (m_trafficId, m_vnfList, finalAddress);
  InetSocketAddress nextAddress (sfcTag.GetNextAddress ());
  packet->AddPacketTag (sfcTag);

  int bytes = m_socket->SendTo (packet, 0, nextAddress);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_INFO ("Source app transmitted a packet of " << bytes <<
                   " bytes to final IP " << finalAddress.GetIpv4 () <<
                   " port " << finalAddress.GetPort () <<
                   " with traffic ID " << m_trafficId);
    }

  // Schedule the next packet transmission.
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &SourceApp::SendPacket, this, newSize);
}

} // namespace ns3
