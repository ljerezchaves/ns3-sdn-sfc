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
   * \param switchDevice The OpenFlow network switch device.
   * \param switchPortNo The port number on the network switch device.
   * \param serverDevice The OpenFlow server switch device.
   * \param serverPortNo The port number on the server switch device.
   * \param switchToServerPortNo The uplink port number from switch to server.
   * \param serverToSwitchPortNo The downlink port number from server to switch.
   * \param vnfInfo The VNF information.
   */
  void InstallVnfCopy (
    Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
    Ptr<Node> serverNode, Ptr<OFSwitch13Device> serverDevice,
    uint32_t switchToServerPortNo, uint32_t serverToSwitchPortNo,
    Ptr<VnfInfo> vnfInfo);

private:
  Ptr<SdnController>            m_controllerApp;    //!< Controller app.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.
  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  NetDeviceContainer            m_portDevices;      //!< Switch port devices.
  uint16_t                      m_numVnfs;          //!< Number of VNFs.
  uint16_t                      m_numNodes;         //!< Number of nodes.

public:
  typedef std::vector<Ptr<OFSwitch13Port>> PortVector_t;  // vector of ports
  typedef std::vector<Ptr<CsmaChannel>> ChannelVector_t;  // vector of channels
  typedef std::vector<std::vector<Ptr<OFSwitch13Port>>> PortVectorVector_t; // matrix of ports
  typedef std::vector<std::vector<Ptr<CsmaChannel>>> ChannelVectorVector_t; // matrix of channels

  NodeContainer                 m_networkNodes;
  NodeContainer                 m_serverNodes;
  NodeContainer                 m_hostNodes;

  OFSwitch13DeviceContainer     m_networkSwitchDevs;
  OFSwitch13DeviceContainer     m_serverSwitchDevs;

  NetDeviceContainer            m_hostDevices;
  Ipv4InterfaceContainer        m_hostIfaces;

  PortVector_t                  m_networkToHostPorts;         // vector [node]
  PortVector_t                  m_serverToNetworkDlinkPorts;  // vector [node]

  PortVectorVector_t            m_networkToVnfUlinkPorts;     // matrix [node][vnf]
  ChannelVectorVector_t         m_networkToVnfUlinkChannels;  // matriz [node][vnf]

  PortVectorVector_t            m_networkToNetworkPorts;      // matrix [src node][dst node]
  ChannelVectorVector_t         m_networkToNetworkChannels;   // matrix [src node][dst node]
};
} // namespace ns3
#endif /* SDN_NETWORK_H */
