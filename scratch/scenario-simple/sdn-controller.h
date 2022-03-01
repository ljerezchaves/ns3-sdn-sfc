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

#ifndef SDN_CONTROLLER_H
#define SDN_CONTROLLER_H

#include <ns3/ofswitch13-module.h>

using namespace ns3;

class SdnNetwork;

/** The OpenFlow controller. */
class SdnController : public OFSwitch13Controller
{
public:
  SdnController ();          //!< Default constructor.
  virtual ~SdnController (); //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Save the SDN network object.
   * \param network The SDN network.
   */
  void SetSdnNetwork (Ptr<SdnNetwork> network);

  /**
   * Notify this controller of a new host connected to the OpenFlow network.
   * \param switchDev The OpenFlow switch device.
   * \param portNo The port number created at the OpenFlow switch.
   * \param hostDev The device created at the host node.
   */
  void NotifyHostAttach (Ptr<OFSwitch13Device> switchDev, uint32_t portNo, Ptr<NetDevice> hostDev);

  /**
   * Notify this controller of a new VNF connected to the OpenFlow network.
   * \param switchDev The OpenFlow switch device.
   * \param portNo The port number created at the OpenFlow switch.
   * \param ipv4Address The VNF IPv4 address.
   * \param macAddress The VNF MAC address.
   */
  void NotifyVnfAttach (Ptr<OFSwitch13Device> switchDev, uint32_t portNo,
                        Ipv4Address ipv4Address, Mac48Address macAddress);

  /**
   * Perform an ARP resolution
   * \param ip The Ipv4Address to search.
   * \return The MAC address for this ip.
   */
  static Mac48Address GetArpEntry (Ipv4Address ip);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from OFSwitch13Controller
  ofl_err HandlePacketIn (struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);

private:
  /**
   * Handle ARP request messages.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleArpPacketIn (struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);

  /**
   * Extract an IPv4 address from packet match.
   * \param oxm_of The OXM_IF_* IPv4 field.
   * \param match The ofl_match structure pointer.
   * \return The IPv4 address.
   */
  Ipv4Address ExtractIpv4Address (uint32_t oxm_of, struct ofl_match* match);

  /**
   * Create an ARP reply packet, encapsulated inside of an Ethernet frame.
   * \param srcMac Source MAC address.
   * \param srcIp Source IP address.
   * \param dstMac Destination MAC address.
   * \param dstIp Destination IP address.
   * \return The ns3 Ptr<Packet> with the ARP reply.
   */
  Ptr<Packet> CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp,
                              Mac48Address dstMac, Ipv4Address dstIp);

  /**
   * Save the pair IP / MAC address in ARP table.
   * \param ipAddr The IPv4 address.
   * \param macAddr The MAC address.
   */
  static void SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr);

  Ptr<SdnNetwork>     m_network;    //!< SDN network pointer.

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  static IpMacMap_t m_arpTable;     //!< ARP resolution table.
};

#endif /* SDN_CONTROLLER_H */
