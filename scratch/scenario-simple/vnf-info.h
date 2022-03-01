/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Campinas (Unicamp)
 *
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
 *
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef VNF_INFO_H
#define VNF_INFO_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/ofswitch13-module.h>

namespace ns3 {

class VnfApp;

/**
 * Metadata associated to a VNF.
 */
class VnfInfo : public Object
{
public:
  /**
   * Complete constructor.
   * \param vnfId The VNF ID.
   */
  VnfInfo (uint32_t vnfId);
  virtual ~VnfInfo ();      //!< Dummy destructor, see DoDispose.

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * \name Private member accessors for VNF information.
   * \param value The value to set.
   * \return The requested information.
   */
  //\{
  uint32_t      GetVnfId          (void) const;
  Ipv4Address   GetServerIpAddr   (void) const;
  Ipv4Address   GetSwitchIpAddr   (void) const;
  Mac48Address  GetServerMacAddr  (void) const;
  Mac48Address  GetSwitchMacAddr  (void) const;
  double        GetServerScaling  (void) const;
  double        GetSwitchScaling  (void) const;
  int           GetActiveCopyIdx  (void) const;

  void          SetServerScaling  (double value);
  void          SetSwitchScaling  (double value);
  void          SetActiveCopyIdx  (int value);
  //\}

  /**
   * Create a VNF application
   * \return The created applivation
   */
  //\{
  Ptr<VnfApp> CreateServerApp (void);
  Ptr<VnfApp> CreateSwitchApp (void);
  //\}

  /**
   * Notify about a new copy of this VNF created in the network.
   * \param serverApp The VNF app on the server switch.
   * \param serverDevice The server switch node.
   * \param switchApp The VNF app on the network switch.
   * \param switchDevice The network switch node.
   */
  void NewVnfCopy (Ptr<VnfApp> serverApp, Ptr<OFSwitch13Device> serverDevice,
                   Ptr<VnfApp> switchApp, Ptr<OFSwitch13Device> switchDevice);

  /**
   * Get the VNF information from the global map for a specific VNF ID.
   * \param vnfId The VNF ID.
   * \return The VNF information.
   */
  static Ptr<VnfInfo> GetPointer (uint32_t vnfId);

protected:
  /** Destructor implementation. */
  virtual void DoDispose ();

private:
  uint32_t        m_vnfId;                //!< VNF ID

  Ipv4Address     m_switchIpAddress;      //!< VNF app IP on the switch node
  Ipv4Address     m_serverIpAddress;      //!< VNF app IP on the server node
  Mac48Address    m_switchMacAddress;     //!< VNF app MAC on the switch node
  Mac48Address    m_serverMacAddress;     //!< VNF app MAC on the server node
  double          m_switchScaling;        //!< Scaling factor at the switch
  double          m_serverScaling;        //!< Scaling factor at the server

  ObjectFactory   m_serverFactory;        //!< Factory for server apps
  ObjectFactory   m_switchFactory;        //!< Factory for switch apps

  /** List of VNF copies */
  typedef std::vector<Ptr<VnfApp>> VnfAppList_t;
  typedef std::vector<uint16_t> PortNoList_t;
  VnfAppList_t              m_switchAppList;  //!< List of apps on switches nodes
  VnfAppList_t              m_serverAppList;  //!< List of apps on servers nodes
  OFSwitch13DeviceContainer m_switchDevList;  //!< List of switch devices
  OFSwitch13DeviceContainer m_serverDevList;  //!< List of server devices
  PortNoList_t              m_switchPortList; //!< List of ports on switch nodes
  PortNoList_t              m_serverPortList; //!< List of ports on server nodes
  int                       m_activeCopyIdx;  //!< Index of the active copy

  /**
   * Register the VNF information in global map for further usage.
   * \param vnfInfo The VNF information.
   */
  static void RegisterVnfInfo (Ptr<VnfInfo> vnfInfo);

  /** Map saving VNF ID / VNF information. */
  typedef std::map<uint32_t, Ptr<VnfInfo> > VnfInfoMap_t;
  static VnfInfoMap_t m_vnfInfoById;  //!< Global VNF info map.
};

} // namespace ns3
#endif // VNF_INFO_H
