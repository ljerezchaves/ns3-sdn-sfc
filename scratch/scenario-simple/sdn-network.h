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
   * Configure the SDN network topology with network, servers and host nodes.
   */
  void ConfigureTopology (void);

  /**
   * Create the VNFs and install copies of them in server nodes.
   */
  void ConfigureFunctions (void);

  /**
   * Configure the aplications for generating traffic in the network.
   */
  void ConfigureTraffic (void);

  void CreateTrafficFlow (
    uint32_t srcNodeId, uint32_t dstNodeId,
    std::vector<uint8_t> vnfList, Time startTime = Seconds (1));

private:
  Ptr<SdnController>            m_controllerApp;    //!< Controller app
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper
  CsmaHelper                    m_csmaHelper;       //!< Connection helper
  NetDeviceContainer            m_portDevices;      //!< Switch port devices
  uint16_t                      m_numVnfs;          //!< Number of VNFs
  uint16_t                      m_numNodes;         //!< Number of nodes

  NodeContainer                 m_networkNodes;     //!< Network nodes
  NodeContainer                 m_serverNodes;      //!< Server nodes
  NodeContainer                 m_hostNodes;        //!< Host nodes

  OFSwitch13DeviceContainer     m_networkSwitchDevs;//!< Network switch devices
  OFSwitch13DeviceContainer     m_serverSwitchDevs; //!< Server switch devices

  NetDeviceContainer            m_hostDevices;      //!< Host CSMA devices
  Ipv4InterfaceContainer        m_hostIfaces;       //!< Host IPv4 addresses

  /** Vector of switch ports */
  typedef std::vector<Ptr<OFSwitch13Port>> PortVector_t;

  /** Vector of CSMA channels */
  typedef std::vector<Ptr<CsmaChannel>> ChannelVector_t;

  /** Matrix of switch ports */
  typedef std::vector<std::vector<Ptr<OFSwitch13Port>>> PortVectorVector_t;

  /** Matrix of CSMA channels */
  typedef std::vector<std::vector<Ptr<CsmaChannel>>> ChannelVectorVector_t;

  /**
   * Vector of ports connecting each network switches to the host nodes
   * Index: [node id]
   */
  PortVector_t m_networkToHostPorts;

  /**
   * Vector of downlink ports connecting each server switch to the network switch
   * Index: [node id]
   */
  PortVector_t m_serverToNetworkDlinkPorts;

  /**
   * Matrix of uplink ports connecting each network switch to the server switch
   * There is one port for each VNF
   * Indexes: [node id][vnf id]
   */
  PortVectorVector_t m_networkToVnfUlinkPorts;

  /**
   * Matrix of CSMA channels connecting each network switch to the server switch
   * There is one channel for each VNF
   * Indexes: [node id][vnf id]
   */
  ChannelVectorVector_t m_networkToVnfUlinkChannels;

  /**
   * Matrix of switch ports connecting a pair of network switches
   * Indexes: [source node id][destination node id]
   */
  PortVectorVector_t m_networkToNetworkPorts;

  /**
   * Matrix of CSMA channels connecting a pair of network switches
   * Indexes: [source node id][destination node id]
   */
  ChannelVectorVector_t m_networkToNetworkChannels;
};
} // namespace ns3
#endif /* SDN_NETWORK_H */
