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

#ifndef SOURCE_APP_H
#define SOURCE_APP_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>

namespace ns3 {

/**
 * This application implements the traffic source for a VNF chain. We can
 * configure a custom traffic pattern by ajusting the PktInterval and PktSize
 * random variables of this class. Each packet created by this application
 * carries a SFC tag with a timestamp and a list of VNFs that this packet must
 * pass through.
 */
class SourceApp : public Application
{
public:
  SourceApp ();            //!< Default constructor.
  virtual ~SourceApp ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Set the local UDP port number.
   * \param port The port number.
   */
  void SetLocalUdpPort (uint16_t port);

  /**
   * Set the final IPv4 address.
   * This is the IPv4 on the host at the end of this traffic.
   * \param address The IP address.
   */
  void SetFinalIpAddress (Ipv4Address address);

  /**
   * Set the final UDP port number.
   * This is the UDP port number on the host at the end of this traffic.
   * \param port The port number.
   */
  void SetFinalUdpPort (uint16_t port);

  /**
   * Set the list of VNF IDs for intermediate traffic processing.
   * This list is empty by default, which means the traffic will flow
   * directly from this source application to the sink application.
   * \param vnfList The VFN ID list.
   */
  void SetVnfList (std::vector<uint8_t> vnfList);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

private:
  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * Handle a packet transmission.
   * \param size The packet size.
   */
  void SendPacket (uint32_t size);

  Ptr<Socket>                 m_socket;         //!< UDP socket.
  uint16_t                    m_localUdpPort;   //!< Local UDP port.
  uint16_t                    m_finalUdpPort;   //!< Final UDP port
  Ipv4Address                 m_finalIpAddress; //!< Final IPv4 address.
  std::vector<uint8_t>        m_vnfList;        //!< VNF list for this traffic.

  Ptr<RandomVariableStream>   m_pktInterRng;    //!< Packet inter-arrival time.
  Ptr<RandomVariableStream>   m_pktSizeRng;     //!< Packet size.
  EventId                     m_sendEvent;      //!< SendPacket event.
};

} // namespace ns3
#endif /* SOURCE_APP_H */
