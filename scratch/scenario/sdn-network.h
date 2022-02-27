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
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.
  NetDeviceContainer            m_csmaPortDevices;  //!< Switch port devices.
  NetDeviceContainer            m_hostDevices;      //!< Host devices.

public:
  Ptr<Node>                     m_coreSwitchNode;
  Ptr<Node>                     m_edge1SwitchNode;
  Ptr<Node>                     m_edge2SwitchNode;
  Ptr<Node>                     m_coreServer1Node;
  Ptr<Node>                     m_coreServer2Node;
  Ptr<Node>                     m_edge1ServerNode;
  Ptr<Node>                     m_edge2ServerNode;
  Ptr<Node>                     m_coreHostNode;
  Ptr<Node>                     m_edge1HostNode;
  Ptr<Node>                     m_edge2HostNode;

  Ipv4Address                   m_coreHostAddress;
  Ipv4Address                   m_edge1HostAddress;
  Ipv4Address                   m_edge2HostAddress;

  Ptr<NetDevice>                m_coreHostDevice;
  Ptr<NetDevice>                m_edge1HostDevice;
  Ptr<NetDevice>                m_edge2HostDevice;

  Ptr<OFSwitch13Device>         m_coreSwitchDevice;
  Ptr<OFSwitch13Device>         m_edge1SwitchDevice;
  Ptr<OFSwitch13Device>         m_edge2SwitchDevice;
  Ptr<OFSwitch13Device>         m_coreServer1Device;
  Ptr<OFSwitch13Device>         m_coreServer2Device;
  Ptr<OFSwitch13Device>         m_edge1ServerDevice;
  Ptr<OFSwitch13Device>         m_edge2ServerDevice;

  Ptr<OFSwitch13Port>           m_coreToHostPort;
  Ptr<OFSwitch13Port>           m_edge1ToHostPort;
  Ptr<OFSwitch13Port>           m_edge2ToHostPort;
  Ptr<OFSwitch13Port>           m_coreToEdge1APort;
  Ptr<OFSwitch13Port>           m_edge1ToCoreAPort;
  Ptr<OFSwitch13Port>           m_coreToEdge2APort;
  Ptr<OFSwitch13Port>           m_edge2ToCoreAPort;
  Ptr<OFSwitch13Port>           m_coreToEdge1BPort;
  Ptr<OFSwitch13Port>           m_edge1ToCoreBPort;
  Ptr<OFSwitch13Port>           m_coreToEdge2BPort;
  Ptr<OFSwitch13Port>           m_edge2ToCoreBPort;
  Ptr<OFSwitch13Port>           m_edge1ToEdge2Port;
  Ptr<OFSwitch13Port>           m_edge2ToEdge1Port;
  Ptr<OFSwitch13Port>           m_coreToServer1UlinkPort;
  Ptr<OFSwitch13Port>           m_coreToServer1DlinkPort;
  Ptr<OFSwitch13Port>           m_coreToServer2UlinkPort;
  Ptr<OFSwitch13Port>           m_coreToServer2DlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToCoreUlinkPort;
  Ptr<OFSwitch13Port>           m_server1ToCoreDlinkPort;
  Ptr<OFSwitch13Port>           m_server2ToCoreUlinkPort;
  Ptr<OFSwitch13Port>           m_server2ToCoreDlinkPort;
  Ptr<OFSwitch13Port>           m_edge1ToServerUlinkPort;
  Ptr<OFSwitch13Port>           m_edge1ToServerDlinkPort;
  Ptr<OFSwitch13Port>           m_edge2ToServerUlinkPort;
  Ptr<OFSwitch13Port>           m_edge2ToServerDlinkPort;
  Ptr<OFSwitch13Port>           m_serverToEdge1UlinkPort;
  Ptr<OFSwitch13Port>           m_serverToEdge1DlinkPort;
  Ptr<OFSwitch13Port>           m_serverToEdge2UlinkPort;
  Ptr<OFSwitch13Port>           m_serverToEdge2DlinkPort;
};

#endif /* SDN_NETWORK_H */
