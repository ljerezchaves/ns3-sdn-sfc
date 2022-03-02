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
  uint32_t      GetVnfId      (void) const;
  Ipv4Address   GetIpAddr     (void) const;
  Mac48Address  GetMacAddr    (void) const;
  double        Get1stScaling (void) const;
  double        Get2ndScaling (void) const;
  void          Set2ndScaling (double value);
  void          Set1stScaling (double value);
  //\}

  /**
   * Create the pair of applications for this VNF.
   * \return The pair of applications
   */
  std::pair<Ptr<VnfApp>, Ptr<VnfApp>> CreateVnfApps (void);

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
  uint32_t        m_vnfId;            //!< VNF ID
  Ipv4Address     m_vnfIpAddress;     //!< VNF IPv4 address
  Mac48Address    m_vnfMacAddress;    //!< VNF MAC address
  double          m_1stScaling;       //!< 1st scaling factor (switch)
  double          m_2ndScaling;       //!< 2nd scaling factor (server)

  ObjectFactory   m_1stFactory;       //!< Factory for the first app (switch)
  ObjectFactory   m_2ndFactory;       //!< Factory for the second app (server)

  /** List of VNF copies */
  typedef std::vector<Ptr<VnfApp>> VnfAppList_t;
  VnfAppList_t    m_1stAppList;       //!< List of 1st apps (switches)
  VnfAppList_t    m_2ndAppList;       //!< List of 2nd apps (servers)

  /**
   * Register the VNF information in global map for further usage.
   * \param vnfInfo The VNF information.
   */
  static void RegisterVnfInfo (Ptr<VnfInfo> vnfInfo);

  /** Map saving VNF ID / VNF information. */
  typedef std::map<uint32_t, Ptr<VnfInfo>> VnfInfoMap_t;
  static VnfInfoMap_t m_vnfInfoById;  //!< Global VNF info map.
};

} // namespace ns3
#endif // VNF_INFO_H