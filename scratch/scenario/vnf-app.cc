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
  : m_rxSocket (0),
  m_txSocket (0),
  m_sendEvent (EventId ())
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

    .AddAttribute ("NextAddress", "The next VNF address.",
                   AddressValue (),
                   MakeAddressAccessor (&VnfApp::m_nextAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RxPort", "Local RX port.",
                   UintegerValue (10000),
                   MakeUintegerAccessor (&VnfApp::m_rxPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("TxPort", "Local TX port.",
                   UintegerValue (10001),
                   MakeUintegerAccessor (&VnfApp::m_txPort),
                   MakeUintegerChecker<uint16_t> ())

    .AddAttribute ("PktSizeScale",
                   "A scaling factor for the packet size.",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&VnfApp::m_pktSizeScale),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

void
VnfApp::SetNextAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);

  m_nextAddress = address;
}

void
VnfApp::SetLocalRxPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_rxPort = port;
}

void
VnfApp::SetLocalTxPort (uint16_t port)
{
  NS_LOG_FUNCTION (this << port);

  m_txPort = port;
}

void
VnfApp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_rxSocket = 0;
  m_txSocket = 0;
  Application::DoDispose ();
}

void
VnfApp::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  NS_LOG_INFO ("Opening the RX UDP socket.");
  TypeId udpFactory = TypeId::LookupByName ("ns3::UdpSocketFactory");
  m_rxSocket = Socket::CreateSocket (GetNode (), udpFactory);
  m_rxSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_rxPort));
  m_rxSocket->SetRecvCallback (MakeCallback (&VnfApp::ReadPacket, this));

  NS_LOG_INFO ("Opening the TX UDP socket.");
  m_txSocket = Socket::CreateSocket (GetNode (), udpFactory);
  m_txSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_txPort));
  m_txSocket->Connect (InetSocketAddress::ConvertFrom (m_nextAddress));
  m_txSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
}

void
VnfApp::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_rxSocket != 0)
    {
      m_rxSocket->Close ();
      m_rxSocket->Dispose ();
      m_rxSocket = 0;
    }
  if (m_txSocket != 0)
    {
      m_txSocket->Close ();
      m_txSocket->Dispose ();
      m_txSocket = 0;
    }
}

void
VnfApp::SendPacket (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);

  Ptr<Packet> packet = Create<Packet> (size);
  int bytes = m_txSocket->Send (packet);
  if (bytes == static_cast<int> (packet->GetSize ()))
    {
      NS_LOG_DEBUG ("VNF app transmitted a packet with " << bytes << " bytes.");
    }
  else
    {
      NS_LOG_ERROR ("Transmission error in VNF app.");
    }
}

void
VnfApp::ReadPacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet = socket->Recv ();
  uint32_t pktSize = packet->GetSize ();
  NS_LOG_DEBUG ("VNF app received a packet with " << packet->GetSize () << " bytes.");

  SendPacket (pktSize * m_pktSizeScale);
}

} // namespace ns3
