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

    .AddAttribute ("Port", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&SinkApp::m_port),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

void
SinkApp::SetLocalRxPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_port = port;
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
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_port));
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

  Ptr<Packet> packet = socket->Recv ();
  NS_LOG_DEBUG ("Sink app received a packet with " << packet->GetSize () << " bytes.");
}

} // namespace ns3
