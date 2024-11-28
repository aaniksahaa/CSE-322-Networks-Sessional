/*
 * Copyright (c) 2011 University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Justin Rohrer <rohrej@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

/*
 * This example program allows one to run ns-3 DSDV, AODV, or OLSR under
 * a typical random waypoint mobility model.
 *
 * By default, the simulation runs for 200 simulated seconds, of which
 * the first 50 are used for start-up time.  The number of nodes is 50.
 * Nodes move according to RandomWaypointMobilityModel with a speed of
 * 20 m/s and no pause time within a 300x1500 m region.  The WiFi is
 * in ad hoc mode with a 2 Mb/s rate (802.11b) and a Friis loss model.
 * The transmit power is set to 7.5 dBm.
 *
 * It is possible to change the mobility and density of the network by
 * directly modifying the speed and the number of nodes.  It is also
 * possible to change the characteristics of the network by changing
 * the transmit power (as power increases, the impact of mobility
 * decreases and the effective density increases).
 *
 * By default, OLSR is used, but specifying a value of 2 for the protocol
 * will cause AODV to be used, and specifying a value of 3 will cause
 * DSDV to be used.
 *
 * By default, there are 10 source/sink data pairs sending UDP data
 * at an application rate of 2.048 Kb/s each.    This is typically done
 * at a rate of 4 64-byte packets per second.  Application data is
 * started at a random time between 50 and 51 seconds and continues
 * to the end of the simulation.
 *
 * The program outputs a few items:
 * - packet receptions are notified to stdout such as:
 *   <timestamp> <node-id> received one packet from <src-address>
 * - each second, the data reception statistics are tabulated and output
 *   to a comma-separated value (csv) file
 * - some tracing and flow monitor configuration that used to work is
 *   left commented inline in the program
 */

#include "ns3/aodv-module.h"
#include "ns3/raodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-module.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE("aodv-analysis");

/**
 * Routing experiment class.
 *
 * It handles the creation and run of an experiment.
 */
class RoutingExperiment
{
  public:
    RoutingExperiment();
    /**
     * Run the experiment.
     */
    void Run();

    /**
     * Handles the command-line parameters.
     * \param argc The argument count.
     * \param argv The argument vector.
     */
    void CommandSetup(int argc, char** argv);

  private:
    // parameters

    std::string outputNamePrefix;

    std::string protocolName;                    //!< Protocol name.

    // for configuring which variant of RAODV are we running...
    int variant;

    // Number of nodes
    int nodeCount;

    // packets per second
    int packetsPerSecond;

    // speed of nodes
    int nodeSpeed;

    // the time through which flows run
    double flowRunningTime;

    /**
     * Setup the receiving socket in a Sink Node.
     * \param addr The address of the node.
     * \param node The node pointer.
     * \return the socket.
     */
    Ptr<Socket> SetupPacketReceive(Ipv4Address addr, Ptr<Node> node);
    /**
     * Receive a packet.
     * \param socket The receiving socket.
     */
    void ReceivePacket(Ptr<Socket> socket);

    void TransmitPacketCallback(Ptr<const Packet> packet);

    /**
     * Compute the throughput.
     */
    void CheckThroughput();

    uint32_t port{9};            //!< Receiving port number.
    uint32_t bytesTotal{0};      //!< Total received bytes.
    uint32_t packetsTransmitted{0}; //!< Total received packets.
    uint32_t packetsReceived{0}; //!< Total received packets.

    // these are not used
    double m_txp{7.5};                                     //!< Tx power.
    int m_nSinks{10};                                      //!< Number of sink nodes.

    bool pcap{false};
    bool printRoutes{false};
    bool m_traceMobility{false};                           //!< Enable mobility tracing.
    bool m_flowMonitor{true};                             //!< Enable FlowMonitor.
};

// default parameters
RoutingExperiment::RoutingExperiment():
    outputNamePrefix("routing-protocol-analysis"),
    protocolName("AODV"),
    variant(0),
    nodeCount(20),
    packetsPerSecond(100),
    nodeSpeed(5),
    flowRunningTime(20.0)
{
    
}

static inline std::string
PrintReceivedPacket(Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
    std::ostringstream oss;

    oss << Simulator::Now().GetSeconds() << " " << socket->GetNode()->GetId();

    if (InetSocketAddress::IsMatchingType(senderAddress))
    {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddress);
        oss << " received one packet from " << addr.GetIpv4();
    }
    else
    {
        oss << " received one packet!";
    }
    return oss.str();
}

void
RoutingExperiment::ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address senderAddress;
    while ((packet = socket->RecvFrom(senderAddress)))
    {
        bytesTotal += packet->GetSize();
        packetsReceived += 1;
        NS_LOG_UNCOND(PrintReceivedPacket(socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput()
{
    double kbs = (bytesTotal * 8.0) / 1000;
    bytesTotal = 0;

    std::ofstream out(outputNamePrefix + ".csv", std::ios::app);

    out << (Simulator::Now()).GetSeconds() << "," << kbs << "," << packetsReceived << ","
        << packetsTransmitted << " " << std::endl;

    out.close();
    packetsReceived = 0;
    packetsTransmitted = 0;
    Simulator::Schedule(Seconds(1.0), &RoutingExperiment::CheckThroughput, this);
}

void 
RoutingExperiment::TransmitPacketCallback(Ptr<const Packet> packet)
{
    packetsTransmitted++;
}


Ptr<Socket>
RoutingExperiment::SetupPacketReceive(Ipv4Address addr, Ptr<Node> node)
{
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(node, tid);
    InetSocketAddress local = InetSocketAddress(addr, port);
    sink->Bind(local);
    sink->SetRecvCallback(MakeCallback(&RoutingExperiment::ReceivePacket, this));

    return sink;
}

void
RoutingExperiment::CommandSetup(int argc, char** argv)
{
    CommandLine cmd(__FILE__);

    std::string comment;
    
    cmd.AddValue("comment", "Suitable comment as filename prefix for late parsing", comment);
    cmd.AddValue("protocolName", "Routing protocol name(AODV or RAODV)", protocolName);
    cmd.AddValue("variant", "variant number in case of RAODV, ignored for AODV", variant);
    cmd.AddValue("nodeCount", "The number of nodes", nodeCount);
    cmd.AddValue("packetsPerSecond", "The number of packets per second", packetsPerSecond);
    cmd.AddValue("nodeSpeed", "The speed of the mobile nodes", nodeSpeed);
    cmd.AddValue("flowRunningTime", "The time through which flows run", flowRunningTime);

    cmd.Parse(argc, argv);

    std::ostringstream oss;

    oss<<"\nRunning Parameters:\n";
    oss<<"comment = "<<comment<<"\n";
    oss<<"protocolName = "<<protocolName<<"\n";
    oss<<"variant = "<<variant<<"\n";
    oss<<"nodeCount = "<<nodeCount<<"\n";
    oss<<"packetsPerSecond = "<<packetsPerSecond<<"\n";
    oss<<"nodeSpeed = "<<nodeSpeed<<"\n";
    oss<<"flowRunningTime = "<<flowRunningTime<<"\n";
    oss<<"\n";

    NS_LOG_UNCOND(oss.str());

    std::vector<std::string> allowedProtocols{"AODV", "RAODV"};

    if (std::find(std::begin(allowedProtocols), std::end(allowedProtocols), protocolName) ==
        std::end(allowedProtocols))
    {
        NS_FATAL_ERROR("No such protocol:" << protocolName);
    }

    std::stringstream ss;
    ss << "_protocolName-" << protocolName << "_variant-" << variant << "_nodeCount-" << nodeCount << "_packetsPerSecond-" << packetsPerSecond << "_nodeSpeed-" << nodeSpeed << "_flowRunningTime-" << flowRunningTime << "_comment-";
    std::string params = ss.str();

    // std::stringstream ss1;
    // ss1 << packetsPerSecond;
    // std::string sPacketsPerSecond = ss1.str();

    // std::stringstream ss2;
    // ss2 << nodeSpeed;
    // std::string sNodeSpeed = ss2.str();

    std::string dir{"./scratch/results/"};
    std::string basename{"experiment"};
    outputNamePrefix = dir + basename + params + comment;
    // outputNamePrefix = dir + outputNamePrefix + "_nodeCount-" + sNodeCount + "_packetsPerSecond-" + sPacketsPerSecond + "_nodeSpeed-" + sNodeSpeed;
}

int
main(int argc, char* argv[])
{
    RoutingExperiment experiment;
    experiment.CommandSetup(argc, argv);
    experiment.Run();

    return 0;
}

void
RoutingExperiment::Run()
{
    Packet::EnablePrinting();

    // blank out the last output file and write the column headers
    std::ofstream out(outputNamePrefix + ".csv");
    out << "SimulationSecond,"
        << "ReceivedKb,"
        << "PacketsReceived,"
        << "PacketsTransmitted," << std::endl;
    out.close();

    double TotalTime = 100.0 + flowRunningTime;

    int packetSize = 64; 

    int rateValue = packetSize*8*packetsPerSecond;

    // add "bps" suffix
    std::ostringstream oss;
    oss << rateValue << "bps"; 
    std::string rate = oss.str();

    // std::string rate("2048bps");
    std::string phyMode("DsssRate11Mbps");

    int nodePause = 0;  // in s

    Config::SetDefault("ns3::OnOffApplication::PacketSize", StringValue(std::to_string(packetSize)));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue(rate));

    // Set Non-unicastMode rate to unicast mode
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer adhocNodes;
    adhocNodes.Create(nodeCount);

    // setting up wifi phy and channel using helpers
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));

    wifiPhy.Set("TxPowerStart", DoubleValue(m_txp));
    wifiPhy.Set("TxPowerEnd", DoubleValue(m_txp));

    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer adhocDevices = wifi.Install(wifiPhy, wifiMac, adhocNodes);

    MobilityHelper mobilityAdhoc;
    int64_t streamIndex = 0; // used to get consistent mobility across scenarios

    ObjectFactory pos;
    pos.SetTypeId("ns3::RandomRectanglePositionAllocator");
    pos.Set("X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
    pos.Set("Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create()->GetObject<PositionAllocator>();
    streamIndex += taPositionAlloc->AssignStreams(streamIndex);

    std::stringstream ssSpeed;
    ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
    std::stringstream ssPause;
    ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
    mobilityAdhoc.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                                   "Speed",
                                   StringValue(ssSpeed.str()),
                                   "Pause",
                                   StringValue(ssPause.str()),
                                   "PositionAllocator",
                                   PointerValue(taPositionAlloc));
    mobilityAdhoc.SetPositionAllocator(taPositionAlloc);
    mobilityAdhoc.Install(adhocNodes);
    streamIndex += mobilityAdhoc.AssignStreams(adhocNodes, streamIndex);


    AodvHelper aodv;
    RAodvHelper raodv;

    Ipv4ListRoutingHelper list;
    InternetStackHelper internet;

    if (protocolName == "AODV")
    {
        list.Add(aodv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else if (protocolName == "RAODV")
    {
        raodv.Set("Variant", UintegerValue(variant));
        list.Add(raodv, 100);
        internet.SetRoutingHelper(list);
        internet.Install(adhocNodes);
    }
    else
    {
        NS_FATAL_ERROR("No such protocol:" << protocolName);
    }

    NS_LOG_INFO("assigning ip address");

    Ipv4AddressHelper addressAdhoc;
    addressAdhoc.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer adhocInterfaces;
    adhocInterfaces = addressAdhoc.Assign(adhocDevices);

    OnOffHelper onoff1("ns3::UdpSocketFactory", Address());
    onoff1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    onoff1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));

    for (int i = 0; i < nodeCount; i++)
    {
        int flowSrc = nodeCount - 1 - i;
        int flowDst = i;

        Ptr<Socket> sink = SetupPacketReceive(adhocInterfaces.GetAddress(flowDst), adhocNodes.Get(flowDst));

        AddressValue remoteAddress(InetSocketAddress(adhocInterfaces.GetAddress(flowDst), port));
        onoff1.SetAttribute("Remote", remoteAddress);

        // Ptr<Socket> sourceSocket = Socket::CreateSocket(adhocNodes.Get(flowSrc), UdpSocketFactory::GetTypeId());
        // sourceSocket->SetSendCallback(MakeCallback(&RoutingExperiment::TransmitPacketCallback, this));


        Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable>();
        ApplicationContainer temp = onoff1.Install(adhocNodes.Get(flowSrc));

        // Attach a trace callback to monitor transmitted packets
        Ptr<OnOffApplication> onoffApp = StaticCast<OnOffApplication>(temp.Get(0));
        onoffApp->TraceConnectWithoutContext("Tx", MakeCallback(&RoutingExperiment::TransmitPacketCallback, this));

        temp.Start(Seconds(var->GetValue(100.0, 101.0)));
        temp.Stop(Seconds(TotalTime));
    }

    std::stringstream ss;
    ss << nodeCount;
    std::string nodes = ss.str();

    std::stringstream ss2;
    ss2 << nodeSpeed;
    std::string sNodeSpeed = ss2.str();

    std::stringstream ss3;
    ss3 << nodePause;
    std::string sNodePause = ss3.str();

    std::stringstream ss4;
    ss4 << rate;
    std::string sRate = ss4.str();

    // NS_LOG_INFO("Configure Tracing.");
    // outputNamePrefix = outputNamePrefix + "_" + protocolName +"_" + nodes + "nodes_" + sNodeSpeed + "speed_" +
    // sNodePause + "pause_" + sRate + "rate";

    // AsciiTraceHelper ascii;
    // Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream(outputNamePrefix + ".tr");
    // wifiPhy.EnableAsciiAll(osw);
    // AsciiTraceHelper ascii;
    // MobilityHelper::EnableAsciiAll(ascii.CreateFileStream(outputNamePrefix + ".mob"));

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon;
    if (m_flowMonitor)
    {
        flowmon = flowmonHelper.InstallAll();
    }

    NS_LOG_INFO("Run Simulation.");

    CheckThroughput();

    Simulator::Stop(Seconds(TotalTime));
    Simulator::Run();

    if (m_flowMonitor)
    {
        flowmon->SerializeToXmlFile(outputNamePrefix + ".flowmon", false, false);
    }

    Simulator::Destroy();
}
