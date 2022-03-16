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

#ifndef VNF_APP_H
#define VNF_APP_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/virtual-net-device-module.h>

namespace ns3 {

/**
 * This application implements an intermediate VNF for packet processing. In
 * fact, this application does not with the packet except for resizing it based
 * on the ScalingFactor attribute. Each packet received by this
 * application must carry a SFC tag, from which we will get the next address in
 * the VNF chain to send the packet to. When the KeepAddress attribute is set to
 * true, this application doesn't change packet destination address. This is
 * usefull for implementing the first (fake) VNF application that we install in
 * the network switch.
 */
class VnfApp : public Application
{
public:
  VnfApp ();            //!< Default constructor.
  virtual ~VnfApp ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Set the virtual net device and configure the send callback. Setting this
   * virtual device is mandatory so this application can directly communicate
   * with the OpenFlow switch.
   * \param device The virtual device.
   */
  void SetVirtualDevice (Ptr<VirtualNetDevice> device);

  /**
   * Method to be assigned to the send callback of the VirtualNetDevice
   * implementing the OpenFlow logical port. It is called when the OpenFlow
   * switch sends a packet out over the logical port. The logical port callbacks
   * here, and we must process the packet to send it back to the switch.
   * \param inPacket The packet received from the logical port.
   * \param srcMac Ethernet source address.
   * \param dstMac Ethernet destination address.
   * \param protocolNo The type of payload contained in this packet.
   * \return Whether the operation succeeded.
   */
  bool ProcessPacket (Ptr<Packet> inPacket, const Address& source,
                      const Address& dest, uint16_t protocolNo);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

  /**
   * Remove and check the IPv4 and UDP headers from the incoming packet.
   * \param packet The incoming packet
   */
  void RemoveHeaders (Ptr<Packet> packet);

  /**
   * Insert the UDP, IPv4 and Ethernet headers into the output packet.
   * \param packet The output packet.
   * \param srcIp The source IP address.
   * \param dstIp The destination IP address.
   * \param srcPort The source UDP port.
   * \param dstPort The destination UDP port.
   * \param srcMac The source MAC address.
   * \param dstMac The destination MAC address.
   */
  void InsertHeaders (Ptr<Packet> packet, Ipv4Address srcIp, Ipv4Address dstIp,
  uint16_t srcPort, uint16_t dstPort, Mac48Address srcMac, Mac48Address dstMac);

private:
  uint32_t              m_vnfId;            //!< VNF ID.
  Ipv4Address           m_ipv4Address;      //!< Local IPv4 address.
  uint16_t              m_udpPort;          //!< Local UDP port.
  bool                  m_keepAddress;      //!< Keep VNF address.

  double                m_scalingFactor;    //!< Packet size scaling factor.
  EventId               m_sendEvent;        //!< SendPacket event.

  Ptr<VirtualNetDevice> m_logicalPort;      //!< OpenFlow logical port device.
};

} // namespace ns3
#endif /* VNF_APP_H */
