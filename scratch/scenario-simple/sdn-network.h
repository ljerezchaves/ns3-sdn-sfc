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
   * Configure the SDN network topology.
   */
  void ConfigureTopology (void);

  /**
   * Configure the VNFs.
   */
  void ConfigureFunctions (void);

  /**
   * Configure the aplications for traffic in the network.
   */
  void ConfigureApplications (void);

  /**
   * Install a VNF application on the switch node, connecting it to the switch
   * device via a new VirtualNetDevice acting as an OpenFlow logical port.
   * \param switchNode The OpenFlow switch node.
   * \param switchDevice The OpenFlow switch device.
   * \param application The VNF application.
   * \param ipv4Address The VNF IPv4 address.
   * \param macAddress The VNF MAC address.
   */
  void InstallVnf (Ptr<Node> switchNode, Ptr<OFSwitch13Device> switchDevice,
                   Ptr<VnfApp> application, Ipv4Address ipv4Address,
                   Mac48Address macAddress);

private:
  Ptr<SdnController>            m_controllerApp;    //!< Controller app.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.
  NetDeviceContainer            m_portDevices;      //!< Switch port devices.
  NetDeviceContainer            m_hostDevices;      //!< Host devices.

public:
  Ptr<Node>                     m_switchNode;
  Ptr<Node>                     m_serverNode;
  Ptr<Node>                     m_host1Node;
  Ptr<Node>                     m_host2Node;

  Ipv4Address                   m_host1Address;
  Ipv4Address                   m_host2Address;

  Ptr<NetDevice>                m_host1Device;
  Ptr<NetDevice>                m_host2Device;

  Ptr<OFSwitch13Device>         m_switchDevice;
  Ptr<OFSwitch13Device>         m_serverDevice;

  Ptr<OFSwitch13Port>           m_switchToHost1Port;
  Ptr<OFSwitch13Port>           m_switchToHost2Port;
  Ptr<OFSwitch13Port>           m_switchToServerUlinkPort;
  Ptr<OFSwitch13Port>           m_switchToServerDlinkPort;
  Ptr<OFSwitch13Port>           m_serverToSwitchUlinkPort;
  Ptr<OFSwitch13Port>           m_serverToSwitchDlinkPort;
};

} // namespace ns3
#endif /* SDN_NETWORK_H */
