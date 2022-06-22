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

#include <iomanip>
#include <iostream>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include "sdn-network.h"
#include "vnf-info.h"
#include "ns3/opengym-module.h"

using namespace ns3;

void EnableProgress (int);
void EnableLibLog  (bool);
void EnableVerbose (bool);
void ForceDefaults (void);

/*
Define observation space
*/
Ptr<OpenGymSpace> MyGetObservationSpace(void)
{
  uint32_t nodeNum = NodeList::GetNNodes ();
  float low = 0.0;
  float high = 100.0; // max queue lenght by default
  std::vector<uint32_t> shape = {nodeNum,};
  NS_LOG_UNCOND ("NodeNum " << nodeNum);
  std::string type = TypeNameGet<uint32_t> ();
  Ptr<OpenGymBoxSpace> space = CreateObject<OpenGymBoxSpace>(low,high,shape,type);
  NS_LOG_UNCOND ("MyGetObservationSpace: " << space);
  return space;
}

/*
Define action space
*/
Ptr<OpenGymSpace> MyGetActionSpace(void)
{
  uint32_t nodeNum = 5;

  Ptr<OpenGymDiscreteSpace> space = CreateObject<OpenGymDiscreteSpace> (nodeNum);
  NS_LOG_UNCOND ("MyGetActionSpace: " << space);
  return space;
}

/*
Define game over condition
*/
bool MyGetGameOver(void)
{

  bool isGameOver = false;
  bool test = false;
  static float stepCounter = 0.0;
  stepCounter += 1;
  if (stepCounter == 10 && test) {
      isGameOver = true;
  }
  NS_LOG_UNCOND ("MyGetGameOver: " << isGameOver);
  return isGameOver;
}

/*
Collect observations
*/
Ptr<OpenGymDataContainer> MyGetObservation(void)
{
  uint32_t nodeNum = NodeList::GetNNodes ();
  std::vector<uint32_t> shape = {nodeNum,};
  Ptr<OpenGymBoxContainer<uint32_t>> box = CreateObject<OpenGymBoxContainer<uint32_t>>(shape);

  for (uint32_t i=0; i<nodeNum; i++) 
    {
      Ptr<Node> node = NodeList::GetNode (i);
      int nodePackets = 0;
      for (uint32_t n=0; n<node->GetNDevices (); n++)
        {
          // we are using CsmaNetDevice
          Ptr<CsmaNetDevice> dev = DynamicCast <CsmaNetDevice> (node->GetDevice (n));
          if (dev)
            {
              nodePackets += dev->GetQueue ()->GetNPackets ();
            }
        }
      NS_LOG_UNCOND ("Node "<< i << ", Packets in queue " << nodePackets);
      box->AddValue(nodePackets);
    }
  return box;
}

/*
Define reward function
*/
float MyGetReward(void)
{
  static float reward = 0.0;
  reward += 1;
  return reward;
}

/*
Define extra info. Optional
*/
std::string MyGetExtraInfo(void)
{
  std::string myInfo = "testInfo";
  myInfo += "|123";
  NS_LOG_UNCOND("MyGetExtraInfo: " << myInfo);
  return myInfo;
}


/*
Execute received actions
*/
bool MyExecuteActions(Ptr<OpenGymDataContainer> action)
{
  Ptr<OpenGymDiscreteContainer> discrete = DynamicCast<OpenGymDiscreteContainer>(action);
  NS_LOG_UNCOND ("MyExecuteActions: " << action);
  NS_LOG_UNCOND ("Here you enforce the decision");

  return true;
}

void ScheduleNextStateRead(double envStepTime, Ptr<OpenGymInterface> openGym)
{
  std::cout << "----------------------" << std::endl;
  Simulator::Schedule (Seconds(envStepTime), &ScheduleNextStateRead, envStepTime, openGym);
  openGym->NotifyCurrentState();
}

int
main (int argc, char *argv[])
{
  // Command line arguments.
  int   progress = 10;
  int   simTime  = 60;
  bool  verbose  = false;
  bool  libLog   = false;
  bool  pcapLog  = false;
  bool  enableRl = false;
  double envStepTime = 20; //seconds, ns3gym env step time interval
  uint32_t openGymPort = 5557;

  // Parse the command line arguments and force default attributes.
  CommandLine cmd;
  cmd.AddValue ("LibLog",   "Enable ofsoftswitch13 logs.", libLog);
  cmd.AddValue ("Progress", "Simulation progress interval (sec).", progress);
  cmd.AddValue ("SimTime",  "Simulation time (sec)", simTime);
  cmd.AddValue ("Verbose",  "Enable verbose output.", verbose);
  cmd.AddValue ("Pcap",     "Enable PCAP output.", pcapLog);
  cmd.AddValue ("enableRl",     "Enable RL", enableRl);

  cmd.Parse (argc, argv);
  ForceDefaults ();

  // Enable verbose output, library log, and progress report for debug purposes.
  EnableLibLog (libLog);
  EnableProgress (progress);
  EnableVerbose (verbose);

  // OpenGym Env
  Ptr<OpenGymInterface> openGym = CreateObject<OpenGymInterface> (openGymPort);
  openGym->SetGetActionSpaceCb( MakeCallback (&MyGetActionSpace) );
  openGym->SetGetObservationSpaceCb( MakeCallback (&MyGetObservationSpace) );
  openGym->SetGetGameOverCb( MakeCallback (&MyGetGameOver) );
  openGym->SetGetObservationCb( MakeCallback (&MyGetObservation) );
  openGym->SetGetRewardCb( MakeCallback (&MyGetReward) );
  openGym->SetGetExtraInfoCb( MakeCallback (&MyGetExtraInfo) );
  openGym->SetExecuteActionsCb( MakeCallback (&MyExecuteActions) );

  if (enableRl)
    {
      Simulator::Schedule (Seconds(0.0), &ScheduleNextStateRead, envStepTime, openGym);
    }

  // ------------------------------------------------------------------------ //
  // Create the SDN network.
  Ptr<SdnNetwork> sdnNetwork = CreateObjectWithAttributes<SdnNetwork> (
    "NumberVnfs", UintegerValue (6), "NumberNodes", UintegerValue (3));
  sdnNetwork->EnablePcap (pcapLog);

  // Configure VNFs
  // VNFs 0 and 1: network service
  VnfInfo::GetPointer (0)->SetScalingFactors (0.3, 0.9);
  VnfInfo::GetPointer (1)->SetScalingFactors (0.3, 0.9);
  // VNFs 2 and 3: compression service
  VnfInfo::GetPointer (2)->SetScalingFactors (2.2, 0.7);
  VnfInfo::GetPointer (3)->SetScalingFactors (2.2, 0.7);
  // VNFs 4 and 5: expansion service
  // VnfInfo::GetPointer (4)->SetScalingFactors (1.4, 1.8);
  // VnfInfo::GetPointer (5)->SetScalingFactors (1.4, 1.8);
  VnfInfo::GetPointer (4)->SetScalingFactors (1, 1);
  VnfInfo::GetPointer (5)->SetScalingFactors (1, 1);

  // Create network traffic
  sdnNetwork->NewServiceTraffic (
    1, 2, {1, 3}, Seconds (5), Seconds (10),
    "ns3::ConstantRandomVariable[Constant=1250]",
    "ns3::ConstantRandomVariable[Constant=0.5]");
  sdnNetwork->NewServiceTraffic (
    2, 2, {4, 5}, Seconds (25), Seconds (40));

  // sdnNetwork->NewBackgroundTraffic (
  //   1, 0, Seconds (10), Seconds (25),
  //   "ns3::ConstantRandomVariable[Constant=1000]",
  //   "ns3::ConstantRandomVariable[Constant=0.1]");
  // sdnNetwork->NewBackgroundTraffic (
  //   0, 1, Seconds (10), Seconds (25),
  //   "ns3::ConstantRandomVariable[Constant=1000]",
  //   "ns3::ConstantRandomVariable[Constant=0.1]");
  // sdnNetwork->NewBackgroundTraffic (
  //   2, 0, Seconds (15), Seconds (30),
  //   "ns3::ConstantRandomVariable[Constant=512]",
  //   "ns3::ConstantRandomVariable[Constant=0.01]");
  // sdnNetwork->NewBackgroundTraffic (
  //   0, 2, Seconds (15), Seconds (30),
  //   "ns3::ConstantRandomVariable[Constant=512]",
  //   "ns3::ConstantRandomVariable[Constant=0.01]");


  // ------------------------------------------------------------------------ //

  // Populate routing tables.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Start the simulation.
  std::cout << "Simulating..." << std::endl;
  Simulator::Stop (Seconds (simTime) + MilliSeconds (100));
  Simulator::Run ();
  std::cout << "Done!" << std::endl;
  openGym->NotifySimulationEnd();
  Simulator::Destroy ();
  sdnNetwork->Dispose ();
  sdnNetwork = 0;

  return 0;
}

void
EnableLibLog (bool enable)
{
  if (enable)
    {
      ofs::EnableLibraryLog (true);
    }
}

void
EnableProgress (int interval)
{
  if (interval)
    {
      std::cout << "Simulation time: " << Simulator::Now ().As (Time::S) << std::endl;
      Simulator::Schedule (Seconds (interval), &EnableProgress, interval);
    }
}

void
EnableVerbose (bool enable)
{
  if (enable)
    {
      LogLevel logLevelWarn = static_cast<ns3::LogLevel> (
        LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN);
      NS_UNUSED (logLevelWarn);

      LogLevel logLevelWarnInfo = static_cast<ns3::LogLevel> (
        LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN | LOG_INFO);
      NS_UNUSED (logLevelWarnInfo);

      LogLevel logLevelWarnInfoDebug = static_cast<ns3::LogLevel> (
        LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_WARN | LOG_INFO | LOG_DEBUG);
      NS_UNUSED (logLevelWarnInfoDebug);

      LogLevel logLevelInfo = static_cast<ns3::LogLevel> (
        LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_INFO);
      NS_UNUSED (logLevelInfo);

      LogLevel logLevelAll = static_cast<ns3::LogLevel> (
        LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_LEVEL_ALL);
      NS_UNUSED (logLevelAll);

      LogComponentEnable ("SdnController",            logLevelWarnInfo);
      LogComponentEnable ("SdnNetwork",               logLevelWarnInfo);
      LogComponentEnable ("SourceApp",                logLevelWarnInfo);
      LogComponentEnable ("SinkApp",                  logLevelWarnInfo);
      LogComponentEnable ("VnfApp",                   logLevelWarnInfoDebug);

      // OFSwitch13 module components.
      LogComponentEnable ("OFSwitch13Controller",     logLevelWarn);
      LogComponentEnable ("OFSwitch13Device",         logLevelWarn);
      LogComponentEnable ("OFSwitch13Helper",         logLevelWarn);
      LogComponentEnable ("OFSwitch13Interface",      logLevelWarn);
      LogComponentEnable ("OFSwitch13Port",           logLevelWarn);
      LogComponentEnable ("OFSwitch13Queue",          logLevelWarn);
      LogComponentEnable ("OFSwitch13SocketHandler",  logLevelWarn);
    }
}

void ForceDefaults (void)
{
  //
  // Since we are using an external OpenFlow library that expects complete
  // network packets, we must enable checksum computations.
  //
  Config::SetGlobal ("ChecksumEnabled", BooleanValue (true));

  //
  // Whenever possible, use the full-duplex CSMA channel to improve throughput.
  // The code will automatically fall back to half-duplex mode for more than
  // two devices in the same channel.
  // This implementation is not available in default ns-3 code, and I got it
  // from https://codereview.appspot.com/187880044/
  //
  Config::SetDefault ("ns3::CsmaChannel::FullDuplex", BooleanValue (true));

  //
  // The minimum (default) value for TCP MSS is 536, and there's no dynamic MTU
  // discovery implemented yet in ns-3. To allow larger TCP packets, we defined
  // this value to 1400, based on 1500 bytes for Ethernet v2 MTU, and
  // considering 8 bytes for PPPoE header, 40 bytes for GTP/UDP/IP tunnel
  // headers, and 52 bytes for default TCP/IP headers. Don't use higher values
  // to avoid packet fragmentation.
  //
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));

  //
  // Increasing the default MTU for virtual network devices, which are used as
  // OpenFlow virtual port devices.
  //
  Config::SetDefault ("ns3::VirtualNetDevice::Mtu", UintegerValue (3000));

  //
  // OpenFlow switches with two flow tables in the pipeline.
  //
  Config::SetDefault ("ns3::OFSwitch13Device::PipelineTables", UintegerValue (2));

  //
  // OpenFlow channel with dedicated P2P connections.
  //
  Config::SetDefault ("ns3::OFSwitch13Helper::ChannelType",
                      EnumValue (OFSwitch13Helper::DEDICATEDP2P));
}
