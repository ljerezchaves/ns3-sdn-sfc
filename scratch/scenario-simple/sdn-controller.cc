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

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include "sdn-controller.h"
#include "sdn-network.h"

NS_LOG_COMPONENT_DEFINE ("SdnController");
NS_OBJECT_ENSURE_REGISTERED (SdnController);

// Initializing static members.
SdnController::IpMacMap_t SdnController::m_arpTable;

// OpenFlow flow-mod flags.
#define FLAGS_REMOVED_OVERLAP_RESET ((OFPFF_SEND_FLOW_REM | OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))
#define FLAGS_OVERLAP_RESET ((OFPFF_CHECK_OVERLAP | OFPFF_RESET_COUNTS))

SdnController::SdnController ()
  : m_network (0)
{
  NS_LOG_FUNCTION (this);
}

SdnController::~SdnController ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SdnController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SdnController")
    .SetParent<OFSwitch13Controller> ()
    .AddConstructor<SdnController> ()
  ;
  return tid;
}

void
SdnController::SetSdnNetwork (Ptr<SdnNetwork> network)
{
  NS_LOG_FUNCTION (this << network);

  m_network = network;
}

void
SdnController::NotifyHostAttach (
  Ptr<OFSwitch13Device> switchDev, uint32_t portNo, Ptr<NetDevice> hostDev)
{
  NS_LOG_FUNCTION (this << switchDev << portNo << hostDev);

  // Save addresses for further ARP resolution.
  Ipv4Address hostIpAddress = Ipv4AddressHelper::GetAddress (hostDev);
  Mac48Address hostMacAddress = Mac48Address::ConvertFrom (hostDev->GetAddress ());
  SaveArpEntry (hostIpAddress, hostMacAddress);

  // Foward IP packets addressed to the host connected to this port.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,prio=1024,table=0"
      << " eth_type="     << Ipv4L3Protocol::PROT_NUMBER
      << ",ip_dst="       << hostIpAddress
      << " apply:output=" << portNo;
  DpctlExecute (switchDev->GetDatapathId (), cmd.str ());
}

void
SdnController::NotifyVnfAttach (
  Ptr<OFSwitch13Device> switchDev, uint32_t portNo,
  Ipv4Address ipv4Address, Mac48Address macAddress)
{
  NS_LOG_FUNCTION (this << switchDev << portNo << ipv4Address << macAddress);

  // Save the IP and MAC addresses for further ARP resolution.
  SaveArpEntry (ipv4Address, macAddress);

  // Foward IP packets addressed to the VNF connected to this port.
  std::ostringstream cmd;
  cmd << "flow-mod cmd=add,prio=1024,table=0"
      << " eth_type="     << Ipv4L3Protocol::PROT_NUMBER
      << ",ip_dst="       << ipv4Address
      << " apply:output=" << portNo;
  DpctlExecute (switchDev->GetDatapathId (), cmd.str ());
}

void
SdnController::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_network = 0;
  m_arpTable.clear ();
  OFSwitch13Controller::DoDispose ();
}

ofl_err
SdnController::HandlePacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  char *msgStr = ofl_structs_match_to_string ((struct ofl_match_header*)msg->match, 0);
  NS_LOG_DEBUG ("Packet in match: " << msgStr);
  free (msgStr);

  if (msg->reason == OFPR_ACTION)
    {
      // Get Ethernet frame type
      uint16_t ethType;
      struct ofl_match_tlv *tlv;
      tlv = oxm_match_lookup (OXM_OF_ETH_TYPE, (struct ofl_match*)msg->match);
      memcpy (&ethType, tlv->value, OXM_LENGTH (OXM_OF_ETH_TYPE));

      if (ethType == ArpL3Protocol::PROT_NUMBER)
        {
          // ARP packet
          return HandleArpPacketIn (msg, swtch, xid);
        }
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);
  return 0;
}

void
SdnController::HandshakeSuccessful (Ptr<const RemoteSwitch> swtch)
{
  NS_LOG_FUNCTION (this << swtch);

  // Get the switch datapath ID
  uint64_t swDpId = swtch->GetDpId ();

  // For packet-in messages, send only the first 128 bytes to the controller
  DpctlExecute (swDpId, "set-config miss=128");

  // Send ARP requests to the controller
  DpctlExecute (swDpId, "flow-mod cmd=add,table=0,prio=20 "
                "eth_type=0x0806,arp_op=1 apply:output=ctrl");
}

ofl_err
SdnController::HandleArpPacketIn (
  struct ofl_msg_packet_in *msg, Ptr<const RemoteSwitch> swtch, uint32_t xid)
{
  NS_LOG_FUNCTION (this << swtch << xid);

  struct ofl_match_tlv *tlv;

  // Get ARP operation
  uint16_t arpOp;
  tlv = oxm_match_lookup (OXM_OF_ARP_OP, (struct ofl_match*)msg->match);
  memcpy (&arpOp, tlv->value, OXM_LENGTH (OXM_OF_ARP_OP));

  // Get input port
  uint32_t inPort;
  tlv = oxm_match_lookup (OXM_OF_IN_PORT, (struct ofl_match*)msg->match);
  memcpy (&inPort, tlv->value, OXM_LENGTH (OXM_OF_IN_PORT));

  // Get source and target IP address
  Ipv4Address srcIp, dstIp;
  srcIp = ExtractIpv4Address (OXM_OF_ARP_SPA, (struct ofl_match*)msg->match);
  dstIp = ExtractIpv4Address (OXM_OF_ARP_TPA, (struct ofl_match*)msg->match);

  // Get Source MAC address
  Mac48Address srcMac, dstMac;
  tlv = oxm_match_lookup (OXM_OF_ARP_SHA, (struct ofl_match*)msg->match);
  srcMac.CopyFrom (tlv->value);
  tlv = oxm_match_lookup (OXM_OF_ARP_THA, (struct ofl_match*)msg->match);
  dstMac.CopyFrom (tlv->value);

  // Check for ARP request
  if (arpOp == ArpHeader::ARP_TYPE_REQUEST)
    {
      uint8_t replyData[64];

      // Check for existing IP information
      Mac48Address replyMac = GetArpEntry (dstIp);
      Ptr<Packet> pkt = CreateArpReply (replyMac, dstIp, srcMac, srcIp);
      NS_ASSERT_MSG (pkt->GetSize () == 64, "Invalid packet size.");
      pkt->CopyData (replyData, 64);

      // Send the ARP replay back to the input port
      struct ofl_action_output *action =
        (struct ofl_action_output*)xmalloc (sizeof (struct ofl_action_output));
      action->header.type = OFPAT_OUTPUT;
      action->port = OFPP_IN_PORT;
      action->max_len = 0;

      // Send the ARP reply within an OpenFlow PacketOut message
      struct ofl_msg_packet_out reply;
      reply.header.type = OFPT_PACKET_OUT;
      reply.buffer_id = OFP_NO_BUFFER;
      reply.in_port = inPort;
      reply.data_length = 64;
      reply.data = &replyData[0];
      reply.actions_num = 1;
      reply.actions = (struct ofl_action_header**)&action;

      SendToSwitch (swtch, (struct ofl_msg_header*)&reply, xid);
      free (action);
    }

  // All handlers must free the message when everything is ok
  ofl_msg_free ((struct ofl_msg_header*)msg, 0);
  return 0;
}

Ipv4Address
SdnController::ExtractIpv4Address (uint32_t oxm_of, struct ofl_match* match)
{
  switch (oxm_of)
    {
    case OXM_OF_ARP_SPA:
    case OXM_OF_ARP_TPA:
    case OXM_OF_IPV4_DST:
    case OXM_OF_IPV4_SRC:
      {
        uint32_t ip;
        int size = OXM_LENGTH (oxm_of);
        struct ofl_match_tlv *tlv = oxm_match_lookup (oxm_of, match);
        memcpy (&ip, tlv->value, size);
        return Ipv4Address (ntohl (ip));
      }
    default:
      NS_ABORT_MSG ("Invalid IP field.");
    }
}

Ptr<Packet>
SdnController::CreateArpReply (Mac48Address srcMac, Ipv4Address srcIp,
                               Mac48Address dstMac, Ipv4Address dstIp)
{
  NS_LOG_FUNCTION (this << srcMac << srcIp << dstMac << dstIp);

  Ptr<Packet> packet = Create<Packet> ();

  // ARP header
  ArpHeader arp;
  arp.SetReply (srcMac, srcIp, dstMac, dstIp);
  packet->AddHeader (arp);

  // Ethernet header
  EthernetHeader eth (false);
  eth.SetSource (srcMac);
  eth.SetDestination (dstMac);
  if (packet->GetSize () < 46)
    {
      uint8_t buffer[46];
      memset (buffer, 0, 46);
      Ptr<Packet> padd = Create<Packet> (buffer, 46 - packet->GetSize ());
      packet->AddAtEnd (padd);
    }
  eth.SetLengthType (ArpL3Protocol::PROT_NUMBER);
  packet->AddHeader (eth);

  // Ethernet trailer
  EthernetTrailer trailer;
  if (Node::ChecksumEnabled ())
    {
      trailer.EnableFcs (true);
    }
  trailer.CalcFcs (packet);
  packet->AddTrailer (trailer);

  return packet;
}

void
SdnController::SaveArpEntry (Ipv4Address ipAddr, Mac48Address macAddr)
{
  std::pair<Ipv4Address, Mac48Address> entry (ipAddr, macAddr);
  std::pair <IpMacMap_t::iterator, bool> ret;
  ret = SdnController::m_arpTable.insert (entry);
  if (ret.second == true)
    {
      NS_LOG_INFO ("New ARP entry: " << ipAddr << " - " << macAddr);
      return;
    }
}

Mac48Address
SdnController::GetArpEntry (Ipv4Address ip)
{
  IpMacMap_t::iterator ret;
  ret = SdnController::m_arpTable.find (ip);
  if (ret != m_arpTable.end ())
    {
      NS_LOG_INFO ("Found ARP entry: " << ip << " - " << ret->second);
      return ret->second;
    }
  NS_ABORT_MSG ("No ARP information for this IP.");
}
