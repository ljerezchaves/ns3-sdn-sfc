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
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device-module.h>

namespace ns3 {

/**
 * This application implements an intermediate VNF.
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
   * Set the destination address.
   * \param address The address.
   */
  void SetTargetAddress (Address address);

  /**
   * Set the local address.
   * \param address The address.
   */
  void SetLocalAddress (Address address);

  /**
   * Set the local port number.
   * \param port The port number.
   */
  void SetLocalPort (uint16_t port);

  /**
   * Set the virtual net device and configure the send callback.
   * \param device The virtual device.
   */
  void SetVirtualDevice (Ptr<VirtualNetDevice> device);

  /**
   * Method to be assigned to the send callback of the VirtualNetDevice
   * implementing the OpenFlow logical port. It is called when the OpenFlow
   * switch sends a packet out over the logical port. The logical port
   * callbacks here, and we must process the packet to send it back.
   * \param packet The packet received from the logical port.
   * \param source Ethernet source address.
   * \param dest Ethernet destination address.
   * \param protocolNo The type of payload contained in this packet.
   * \return Whether the operation succeeded.
   */
  bool ProcessPacket (Ptr<Packet> packet, const Address& source,
                      const Address& dest, uint16_t protocolNo);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

private:
  uint16_t              m_port;           //!< Local port.
  Address               m_targetAddress;  //!< Destination address.
  Address               m_ipv4Address;   //!< Local address.

  double                m_pktSizeScale;   //!< Packet size scaling factor.
  EventId               m_sendEvent;      //!< SendPacket event.

  Ptr<VirtualNetDevice> m_logicalPort;    //!< OpenFlow logical port device.
};

} // namespace ns3
#endif /* VNF_APP_H */
