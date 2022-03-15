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

#ifndef SDN_NETWORK_H
#define SDN_NETWORK_H

#include <ns3/ofswitch13-module.h>
#include "sdn-controller.h"

namespace ns3 {

class VnfApp;
class VnfInfo;

/** The OpenFlow network. */
class SdnNetwork : public Object
{
public:
  SdnNetwork ();          //!< Default constructor.
  virtual ~SdnNetwork (); //!< Dummy destructor.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Enable PCAP traces files on the OpenFlow network.
   * \param enable True to enable the PCAP traces.
   */
  void EnablePcap (bool enable);

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Configure the SDN network topology. This includes nodes and links between
   * core/edge switches, server switches and host nodes.
   */
  void ConfigureTopology (void);

  /**
   * Create the VNFs and install copies of them in all switches/servers.
   */
  void ConfigureFunctions (void);

  /**
   * Configure the aplications for generating traffic in the network.
   */
  void ConfigureApplications (void);

  /**
   * Install a VNF copy on the network switch and server nodes.
   * \param switchDevice The OpenFlow network switch device
   * \param switchPortNo The port number on the network switch device
   * \param serverDevice The OpenFlow server switch device
   * \param serverPortNo The port number on the server switch device
   * \param switchToServerPortNo The uplink port number from switch to server
   * \param serverToSwitchPortNo The downlink port number from server to switch
   * \param vnfInfo The VNF information.
   * \param serverId The server ID (for several servers on the same switch)
   */
  void InstallVnfCopy (
    Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
    Ptr<Node> serverNode, Ptr<OFSwitch13Device> serverDevice,
    uint32_t switchToServerPortNo, uint32_t serverToSwitchPortNo,
    Ptr<VnfInfo> vnfInfo, int serverId);

private:
  Ptr<SdnController>            m_controllerApp;    //!< Controller app.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.
  NetDeviceContainer            m_portDevices;      //!< Switch port devices.
  NetDeviceContainer            m_hostDevices;      //!< Host devices.
  uint16_t                      m_numVnfs;          //!< Number of VNFs.

public:
  Ptr<Node>                     m_core1SwitchNode;
  Ptr<Node>                     m_edge1SwitchNode;
  Ptr<Node>                     m_edge2SwitchNode;
  Ptr<OFSwitch13Device>         m_core1SwitchDevice;
  Ptr<OFSwitch13Device>         m_edge1SwitchDevice;
  Ptr<OFSwitch13Device>         m_edge2SwitchDevice;

  Ptr<Node>                     m_core1Server1Node;
  Ptr<Node>                     m_core1Server2Node;
  Ptr<Node>                     m_edge1Server1Node;
  Ptr<Node>                     m_edge2Server1Node;
  Ptr<OFSwitch13Device>         m_core1Server1Device;
  Ptr<OFSwitch13Device>         m_core1Server2Device;
  Ptr<OFSwitch13Device>         m_edge1Server1Device;
  Ptr<OFSwitch13Device>         m_edge2Server1Device;

  Ptr<Node>                     m_core1HostNode;
  Ptr<Node>                     m_edge1HostNode;
  Ptr<Node>                     m_edge2HostNode;
  Ptr<NetDevice>                m_core1HostDevice;
  Ptr<NetDevice>                m_edge1HostDevice;
  Ptr<NetDevice>                m_edge2HostDevice;
  Ipv4Address                   m_core1HostAddress;
  Ipv4Address                   m_edge1HostAddress;
  Ipv4Address                   m_edge2HostAddress;

  Ptr<OFSwitch13Port>           m_core1ToHostPort;
  Ptr<OFSwitch13Port>           m_edge1ToHostPort;
  Ptr<OFSwitch13Port>           m_edge2ToHostPort;

  Ptr<OFSwitch13Port>           m_core1ToEdge1Port;
  Ptr<OFSwitch13Port>           m_edge1ToCore1Port;
  Ptr<OFSwitch13Port>           m_core1ToEdge2Port;
  Ptr<OFSwitch13Port>           m_edge2ToCore1Port;
  Ptr<OFSwitch13Port>           m_edge1ToEdge2Port;
  Ptr<OFSwitch13Port>           m_edge2ToEdge1Port;

  Ptr<OFSwitch13Port>           m_core1ToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToCore1DlinkPort;
  Ptr<OFSwitch13Port>           m_core1ToServer2DlinkPort;
  Ptr<OFSwitch13Port>           m_server2ToCore1DlinkPort;
  Ptr<OFSwitch13Port>           m_edge1ToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToEdge1DlinkPort;
  Ptr<OFSwitch13Port>           m_edge2ToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToEdge2DlinkPort;

  std::vector<Ptr<CsmaChannel>> m_core1Server1VnfLinks;
  std::vector<Ptr<CsmaChannel>> m_core1Server2VnfLinks;
  std::vector<Ptr<CsmaChannel>> m_edge1Server1VnfLinks;
  std::vector<Ptr<CsmaChannel>> m_edge2Server1VnfLinks;

  std::vector<Ptr<OFSwitch13Port>>  m_core1ToServer1VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_server1ToCore1VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_core1ToServer2VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_server2ToCore1VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_edge1ToServer1VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_server1ToEdge1VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_edge2ToServer1VnfPorts;
  std::vector<Ptr<OFSwitch13Port>>  m_server1ToEdge2VnfPorts;
};
} // namespace ns3
#endif /* SDN_NETWORK_H */
