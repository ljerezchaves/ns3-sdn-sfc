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

protected:
  /** Destructor implementation. */
  virtual void DoDispose (void);

  // Inherited from ObjectBase.
  virtual void NotifyConstructionCompleted (void);

  /**
   * Create the controller application and switch devices for the SDN network.
   */
  virtual void CreateTopology (void);

private:
  Ptr<SdnController>            m_controllerApp;    //!< Controller app.
  Ptr<Node>                     m_controllerNode;   //!< Controller node.

  CsmaHelper                    m_csmaHelper;       //!< Connection helper.
  Ptr<OFSwitch13InternalHelper> m_switchHelper;     //!< Switch helper.

  OFSwitch13DeviceContainer     m_networkDevices;   //!< Switch network devices.
  NodeContainer                 m_networkNodes;     //!< Switch network nodes.
  OFSwitch13DeviceContainer     m_serverDevices;    //!< Switch server devices.
  NodeContainer                 m_serverNodes;      //!< Switch server nodes.

  NodeContainer                 m_hostNodes;        //!< Host nodes.
  NetDeviceContainer            m_hostDevices;      //!< Host devices.

  Ipv4AddressHelper             m_hostAddHelper;    //!< Host address helper.

};

#endif /* SDN_NETWORK_H */
