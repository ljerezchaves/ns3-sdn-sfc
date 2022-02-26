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

using namespace ns3;

/** The OpenFlow controller. */
class SdnController : public OFSwitch13Controller
{
public:
  SdnController ();          //!< Default constructor.
  virtual ~SdnController (); //!< Dummy destructor.

  /** Destructor implementation */
  virtual void DoDispose ();

  /**
   * Register this type.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

protected:
  // Inherited from OFSwitch13Controller
  ofl_err HandlePacketIn (struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid);
  void HandshakeSuccessful (Ptr<const RemoteSwitch> swtch);

private:
};

#endif /* SDN_CONTROLLER_H */
