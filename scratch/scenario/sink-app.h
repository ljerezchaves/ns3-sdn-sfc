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

#ifndef SINK_APP_H
#define SINK_APP_H

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>

namespace ns3 {

/**
 * This application implements the traffic sink for a VNF chain.
 */
class SinkApp : public Application
{
public:
  SinkApp ();            //!< Default constructor.
  virtual ~SinkApp ();   //!< Dummy destructor, see DoDispose.

  /**
   * Get the type ID.
   * \return the object TypeId.
   */
  static TypeId GetTypeId (void);

  /**
   * Set the local port number.
   * \param port The port number.
   */
  void SetLocalPort (uint16_t port);

protected:
  /** Destructor implementation */
  virtual void DoDispose (void);

private:
  // Inherited from Application.
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * Handle a packet reception.
   * \param socket Socket with data available to be read.
   */
  void ReadPacket (Ptr<Socket> socket);

  Ptr<Socket>                 m_socket;         //!< Local socket.
  uint16_t                    m_port;           //!< Local port.
};

} // namespace ns3
#endif /* SINK_APP_H */
