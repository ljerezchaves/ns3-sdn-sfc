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
 *
 * Author: Luciano Jerez Chaves <ljerezchaves@gmail.com>
 */

#include "vnf-first-app.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VnfFirstApp");
NS_OBJECT_ENSURE_REGISTERED (VnfFirstApp);

VnfFirstApp::VnfFirstApp ()
  : m_socket (0),
  m_sendEvent (EventId ())
{
  NS_LOG_FUNCTION (this);
}

VnfFirstApp::~VnfFirstApp ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
VnfFirstApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VnfFirstApp")
    .SetParent<Application> ()
    .AddConstructor<VnfFirstApp> ()

    .AddAttribute ("NextAddress", "The next VMF address.",
                   AddressValue (),
                   MakeAddressAccessor (&VnfFirstApp::m_nextAddress),
                   MakeAddressChecker ())
    .AddAttribute ("Port", "Local port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&VnfFirstApp::m_port),
                   MakeUintegerChecker<uint16_t> ())

    // These attributes must be configured for the desired traffic pattern.
    .AddAttribute ("PktInterval",
                   "A random variable for the packet inter-arrival time [s].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&VnfFirstApp::m_pktInterRng),
                   MakePointerChecker <RandomVariableStream> ())
    .AddAttribute ("PktSize",
                   "A random variable for the packet size [bytes].",
                   StringValue ("ns3::ConstantRandomVariable[Constant=100]"),
                   MakePointerAccessor (&VnfFirstApp::m_pktSizeRng),
                   MakePointerChecker <RandomVariableStream> ())
  ;
  return tid;
}

void
VnfFirstApp::SetNextAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_nextAddress = address;
}

void
VnfFirstApp::SetLocalTxPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_port = port;
}

void
VnfFirstApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  Application::DoDispose ();
}

void
VnfFirstApp::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the TX UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_socket = Socket::CreateSocket (GetNode (), udpFactory);
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_port));
  m_socket->Connect (InetSocketAddress::ConvertFrom (m_nextAddress));
  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());

  // Schedule the first packet transmission.
  m_sendEvent.Cancel ();
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &VnfFirstApp::SendPacket, this, newSize);
}

void
VnfFirstApp::StopApplication ()
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
}

void
VnfFirstApp::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  Ptr<Packet> packet = Create<Packet> (size);
  int bytes = m_socket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("First VNF TX packet with " << bytes << " bytes.");
    }
  else
    {
      NS_LOG_ERROR ("First VNF TX error.");
    }

  // Schedule the next packet transmission.
  Time sendTime = Seconds (std::abs (m_pktInterRng->GetValue ()));
  uint32_t newSize = m_pktSizeRng->GetInteger ();
  m_sendEvent = Simulator::Schedule (sendTime, &VnfFirstApp::SendPacket, this, newSize);
}

} // namespace ns3
