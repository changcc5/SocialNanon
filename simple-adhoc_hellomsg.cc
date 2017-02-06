/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

// 
// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000 
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect. 
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
// 
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

class AnonHeader : public Header 

{
public:

  AnonHeader ();
  virtual ~AnonHeader ();

  void SetData (uint16_t data);
  uint16_t GetData (void) const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;
private:
  uint16_t m_data;
};

AnonHeader::AnonHeader ()
{
  // we must provide a public default constructor, 
  // implicit or explicit, but never private.
}
AnonHeader::~AnonHeader ()
{
}

TypeId
AnonHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AnonHeader")
    .SetParent<Header> ()
    .AddConstructor<AnonHeader> ()
  ;
  return tid;
}
TypeId
AnonHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
AnonHeader::Print (std::ostream &os) const
{
  // This method is invoked by the packet printing
  // routines to print the content of my header.
  //os << "data=" << m_data << std::endl;
  os << "data=" << m_data;
}
uint32_t
AnonHeader::GetSerializedSize (void) const
{
  // we reserve 2 bytes for our header.
  return 2;
}
void
AnonHeader::Serialize (Buffer::Iterator start) const
{
  // we can serialize two bytes at the start of the buffer.
  // we write them in network byte order.
  start.WriteHtonU16 (m_data);
}
uint32_t
AnonHeader::Deserialize (Buffer::Iterator start)
{
  // we can deserialize two bytes from the start of the buffer.
  // we read them in network byte order and store them
  // in host byte order.
  m_data = start.ReadNtohU16 ();

  // we return the number of bytes effectively read.
  return 2;
}

void 
AnonHeader::SetData (uint16_t data)
{
  m_data = data;
}
uint16_t 
AnonHeader::GetData (void) const
{
  return m_data;
}



NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

void ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  uint32_t num_packet = 0;//##1
  while ( packet = socket->Recv ())
    {
      uint8_t *outBuf = new uint8_t [packet -> GetSize()];
      packet->CopyData (outBuf, packet -> GetSize());
      
      std::ostringstream convert;
      for (uint32_t a = 0; a < packet -> GetSize(); a++) {
          convert << outBuf[a];
      }

      std::string output = convert.str();
      NS_LOG_UNCOND (output);

      num_packet++;
      NS_LOG_UNCOND ("Received "<<num_packet<<" packet!");//##
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      Ptr<Packet> helloMsg = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello world!"), 12);
      //socket->Send (Create<Packet> (pktSize));##2
      socket->Send (helloMsg);//##3
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double rss = -80;  // -dBm
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 5;
  double interval = 1.0; // seconds
  bool verbose = false;
  uint32_t users = 25;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  NodeContainer c;
        
  c.Create (users);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) ); 
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
   wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel","MaxRange",DoubleValue (10));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  // Note that with FixedRssLossModel, the positions below are not 
  // used for received signal strength. 
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  //mobility.SetPositionAllocator (positionAlloc);

  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
  "X", StringValue ("10.0"),
  "Y", StringValue ("10.0"),
  "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-100, 100, -100, 100)),
                             "Distance", DoubleValue (1.0),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  mobility.Install (c);

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

//receiver ##
  //https://groups.google.com/forum/#!topic/ns-3-users/nSQB8-y1n1w
  //implement a class of receiver, not using socket only
  std::vector<Ptr<Socket> > receiveSink (users);
  for (uint32_t n = 0; n < users; n++) {
        receiveSink[n] = Socket::CreateSocket (c.Get(n), tid);
  }

  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);

  for (uint32_t n = 0; n < users; n++) {
        receiveSink[n]->Bind (local);
        receiveSink[n]->SetRecvCallback (MakeCallback (&ReceivePacket));      
  //SetRecvCallback (Callback<void, Ptr<Socket>>)##4
  }       

  Ptr<Socket> source = Socket::CreateSocket (c.Get (10), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Tracing
  //wifiPhy.EnablePcap ("simple-adhoc", devices);

  // Output what we are doing
  NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic, 
                                  source, packetSize, numPackets, interPacketInterval);

  Simulator::Stop (Seconds (10.0));
  AnimationInterface anim ("simple-adhoc.xml");
  //anim.SetConstantPosition (csmaNodes.Get(1), 6, 10);
  //anim.SetConstantPosition (csmaNodes.Get(2), 9, 10);
  //anim.SetConstantPosition (csmaNodes.Get(3), 12, 10);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
