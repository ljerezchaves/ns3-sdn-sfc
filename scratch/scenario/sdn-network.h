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

using namespace ns3;

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

private:
  Ptr<SdnController>            m_controllerApp;    //!< Controller app.
  Ptr<Node>                     m_controllerNode;   //!< Controller node.

  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.

  OFSwitch13DeviceContainer     m_networkDevices;   //!< Switch network devices.
  NodeContainer                 m_networkNodes;     //!< Switch network nodes.
  OFSwitch13DeviceContainer     m_serverDevices;    //!< Switch server devices.
  NodeContainer                 m_serverNodes;      //!< Switch server nodes.
  NetDeviceContainer            m_csmaPortDevices;  //!< Switch CSMA port devices.

  NodeContainer                 m_hostNodes;        //!< Host nodes.
  NetDeviceContainer            m_hostDevices;      //!< Host devices.
  Ipv4InterfaceContainer        m_hostIfaces;       //!< Host IP interfaces.
  Ipv4AddressHelper             m_hostAddrHelper;   //!< Host address helper.

public:
  Ptr<Node>                     m_core1SwitchNode;
  Ptr<Node>                     m_edge1SwitchNode;
  Ptr<Node>                     m_edge2SwitchNode;
  Ptr<Node>                     m_core1Server1Node;
  Ptr<Node>                     m_core1Server2Node;
  Ptr<Node>                     m_edge1Server1Node;
  Ptr<Node>                     m_edge2Server1Node;
  Ptr<Node>                     m_host1Node;
  Ptr<Node>                     m_host2Node;
  Ptr<Node>                     m_host3Node;

  Ipv4Address                   m_host1Address;
  Ipv4Address                   m_host2Address;
  Ipv4Address                   m_host3Address;

  Ptr<NetDevice>                m_host1Device;
  Ptr<NetDevice>                m_host2Device;
  Ptr<NetDevice>                m_host3Device;

  Ptr<OFSwitch13Device>         m_core1SwitchDevice;
  Ptr<OFSwitch13Device>         m_edge1SwitchDevice;
  Ptr<OFSwitch13Device>         m_edge2SwitchDevice;
  Ptr<OFSwitch13Device>         m_core1Server1Device;
  Ptr<OFSwitch13Device>         m_core1Server2Device;
  Ptr<OFSwitch13Device>         m_edge1Server1Device;
  Ptr<OFSwitch13Device>         m_edge2Server1Device;

  Ptr<OFSwitch13Port>           m_core1ToHostPort;
  Ptr<OFSwitch13Port>           m_edge1ToHostPort;
  Ptr<OFSwitch13Port>           m_edge2ToHostPort;
  Ptr<OFSwitch13Port>           m_core1ToEdge1APort;
  Ptr<OFSwitch13Port>           m_edge1ToCore1APort;
  Ptr<OFSwitch13Port>           m_core1ToEdge2APort;
  Ptr<OFSwitch13Port>           m_edge2ToCore1APort;
  Ptr<OFSwitch13Port>           m_core1ToEdge1BPort;
  Ptr<OFSwitch13Port>           m_edge1ToCore1BPort;
  Ptr<OFSwitch13Port>           m_core1ToEdge2BPort;
  Ptr<OFSwitch13Port>           m_edge2ToCore1BPort;
  Ptr<OFSwitch13Port>           m_edge1ToEdge2Port;
  Ptr<OFSwitch13Port>           m_edge2ToEdge1Port;
  Ptr<OFSwitch13Port>           m_core1ToServer1UlinkPort;
  Ptr<OFSwitch13Port>           m_core1ToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_core1ToServer2UlinkPort;
  Ptr<OFSwitch13Port>           m_core1ToServer2DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToCore1UlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToCore1DlinkPort;
  Ptr<OFSwitch13Port>           m_server2ToCore1UlinkPort;
  Ptr<OFSwitch13Port>           m_server2ToCore1DlinkPort;
  Ptr<OFSwitch13Port>           m_edge1ToServer1UlinkPort;
  Ptr<OFSwitch13Port>           m_edge1ToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_edge2ToServer1UlinkPort;
  Ptr<OFSwitch13Port>           m_edge2ToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToEdge1UlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToEdge1DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToEdge2UlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToEdge2DlinkPort;
};

#endif /* SDN_NETWORK_H */
