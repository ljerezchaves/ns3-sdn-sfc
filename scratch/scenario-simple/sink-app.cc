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

#include "sink-app.h"
#include "sfc-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SinkApp");
NS_OBJECT_ENSURE_REGISTERED (SinkApp);

SinkApp::SinkApp ()
  : m_socket (0)
{
  NS_LOG_FUNCTION (this);
}

SinkApp::~SinkApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SinkApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SinkApp")
    .SetParent<Application> ()
    .AddConstructor<SinkApp> ()

    .AddAttribute ("LocalIpAddress", "Local IPv4 address.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&SinkApp::m_localIpAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("LocalUdpPort", "Local UDP port.",
                   TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT,
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SinkApp::m_localUdpPort),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
SinkApp::SetLocalIpAddress (Ipv4Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_localIpAddress = address;
}

void
SinkApp::SetLocalUdpPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_localUdpPort = port;
}

void
SinkApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  Application::DoDispose ();
}

void
SinkApp::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the RX UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (m_localIpAddress, m_localUdpPort));
  m_socket->SetRecvCallback (MakeCallback (&SinkApp::ReadPacket, this));
}

void
SinkApp::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->Dispose ();
      m_socket = 0;
    }
}

void
SinkApp::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Address fromAddr;
  Ptr<Packet> packet = socket->RecvFrom (fromAddr);

  SfcTag sfcTag;
  packet->PeekPacketTag (sfcTag);
  Time delay = Simulator::Now () - sfcTag.GetTimestamp ();
  NS_LOG_INFO ("Sink app at IP " << m_localIpAddress <<
               " port " << m_localUdpPort <<
               " received a packet of " << packet->GetSize () <<
               " bytes from source app at IP " << InetSocketAddress::ConvertFrom (fromAddr).GetIpv4 () <<
               " port " << InetSocketAddress::ConvertFrom (fromAddr).GetPort () <<
               " with end-to-end delay " << delay.As (Time::MS));
}

} // namespace ns3
