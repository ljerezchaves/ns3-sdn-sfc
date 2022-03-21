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

namespace ns3 {

class SdnNetwork;
class VnfInfo;

/** The OpenFlow controller. */
class SdnController : public OFSwitch13Controller
{
public:
  /**
   * Complete constructor.
   * \param sdnNetwork The SDN network.
   */
  SdnController (Ptr<SdnNetwork> sdnNetwork);
  virtual ~SdnController (); //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Notify this controller of a host connected to the OpenFlow network.
   * \param switchDevice The OpenFlow network switch device.
   * \param switchPortNo The port number on the network switch device
   * \param hostDevice The device created at the host node.
   */
  void NotifyHostAttach (
    Ptr<OFSwitch13Device> switchDevice, uint32_t switchPortNo,
    Ptr<NetDevice> hostDevice);

  /**
   * Notify this controller of a VNF connected to the OpenFlow network.
   * \param switchDevice The OpenFlow network switch device.
   * \param switchPortNo The port number on the network switch device.
   * \param serverDevice The OpenFlow server switch device.
   * \param serverPortNo The port number on the server switch device.
   * \param switchToServerPortNo The uplink port number from switch to server.
   * \param serverToSwitchPortNo The downlink port number from server to switch.
   * \param vnfInfo The VNF information.
   */
  void NotifyVnfAttach (
    Ptr<OFSwitch13Device> switchDevice, uint32_t switchPortNo,
    Ptr<OFSwitch13Device> serverDevice, uint32_t serverPortNo,
    uint32_t switchToServerPortNo, uint32_t serverToSwitchPortNo,
    Ptr<VnfInfo> vnfInfo);

  /**
   * Notify this controller about a new service traffic flow in the network.
   * \param srcAddress The source socket address (traffic ID).
   * \param dstAddress The destination socket address.
   * \param srcHostId The source host node ID.
   * \param dstHostId The destination host node ID.
   * \param vnfList The list of VNF IDs for this traffic.
   * \param startTime The application start time.
   * \param stopTime The application stop time.
   */
  void NotifyNewServiceTraffic (
    InetSocketAddress srcAddress, InetSocketAddress dstAddress,
    uint32_t srcHostId, uint32_t dstHostId, std::vector<uint8_t> vnfList,
    Time startTime, Time stopTime);

  /**
   * Notify this controller about a new background traffic flow in the network.
   * \param srcAddress The source socket address (traffic ID).
   * \param dstAddress The destination socket address.
   * \param srcHostId The source host node ID.
   * \param dstHostId The destination host node ID.
   * \param startTime The application start time.
   * \param stopTime The application stop time.
   */
  void NotifyNewBackgroundTraffic (
    InetSocketAddress srcAddress, InetSocketAddress dstAddress,
    uint32_t srcHostId, uint32_t dstHostId,
    Time startTime, Time stopTime);

  /**
   * Activate the VNF on a given server for a specific traffic.
   * \param vnfId The VNF ID
   * \param serverId The server ID
   * \param srcAddress The source socket address (traffic ID)
   */
  void SetUpVnf (uint8_t vnfId, uint32_t serverId, InetSocketAddress srcAddress);

  /**
   * Move the active VNF from one server to the other for a specific traffic.
   * \param vnfId The VNF ID
   * \param srcServerId The source server ID
   * \param dstServerId The destination server ID
   * \param srcAddress The source socket address (traffic ID)
   */
  void MoveVnf (uint8_t vnfId, uint32_t srcServerId, uint32_t dstServerId,
                InetSocketAddress srcAddress);

  /**
   * Route network traffic from source to destination switches,
   * considering source and destination addresses.
   * \param srcAddress The source socket address.
   * \param dstAddress The destination socket address.
   * \param srcNodeId The source network switch.
   * \param dstNodeId The destination network switch.
   */
  void RouteTraffic (InetSocketAddress srcAddress, InetSocketAddress dstAddress,
                     uint32_t srcNodeId, uint32_t dstNodeId);

  /**
   * Perform an ARP resolution
   * \param ip The Ipv4Address to search.
   * \return The MAC address for this ip.
   */
  static Mac48Address GetArpEntry (Ipv4Address ip);

  /**
   * Save the pair IP / MAC address in ARP table.
   * \param ipAddr The IPv4 address.
   * \param macAddr The MAC address.
   */
  static void SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr);

protected:
  /** Destructor implementation */
  virtual void DoDispose ();

  // Inherited from OFSwitch13Controller
  ofl_err HandlePacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);

private:
  /**
   * Handle ARP request messages.
   * \param msg The packet-in message.
   * \param swtch The switch information.
   * \param xid Transaction id.
   * \return 0 if everything's ok, otherwise an error number.
   */
  ofl_err HandleArpPacketIn (
    struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);

  /**
   * Extract an IPv4 address from packet match.
   * \param oxm_of The OXM_IF_* IPv4 field.
   * \param match The ofl_match structure pointer.
   * \return The IPv4 address.
   */
  Ipv4Address ExtractIpv4Address (
    uint32_t oxm_of, struct ofl_match* match);

  /**
   * Create an ARP reply packet, encapsulated inside of an Ethernet frame.
   * \param srcMac Source MAC address.
   * \param srcIp Source IP address.
   * \param dstMac Destination MAC address.
   * \param dstIp Destination IP address.
   * \return The ns3 Ptr<Packet> with the ARP reply.
   */
  Ptr<Packet> CreateArpReply (
    Mac48Address srcMac, Ipv4Address srcIp,
    Mac48Address dstMac, Ipv4Address dstIp);

  Ptr<SdnNetwork>   m_network;      //!< SDN network pointer.

  /** Map saving <IPv4 address / MAC address> */
  typedef std::map<Ipv4Address, Mac48Address> IpMacMap_t;
  static IpMacMap_t m_arpTable;     //!< ARP resolution table.
};

} // namespace ns3
#endif /* SDN_CONTROLLER_H */
