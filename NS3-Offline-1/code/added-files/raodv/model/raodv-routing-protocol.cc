/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      https://web.archive.org/web/20100527072022/http://core.it.uu.se/core/index.php/RAODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 * 
 *          Anik Saha <aaniksahaa.2001@gmail.com>
 */
#define NS_LOG_APPEND_CONTEXT                                                                      \
    if (m_ipv4)                                                                                    \
    {                                                                                              \
        std::clog << "[node " << m_ipv4->GetObject<Node>()->GetId() << "] ";                       \
    }



#define ANSI_RED       "\x1b[31m"
#define ANSI_GREEN     "\x1b[32m"
#define ANSI_YELLOW    "\x1b[33m"
#define ANSI_BLUE      "\x1b[34m"
#define ANSI_MAGENTA   "\x1b[35m"
#define ANSI_CYAN      "\x1b[36m"
#define ANSI_WHITE     "\x1b[37m"
#define ANSI_BLACK     "\x1b[30m"
#define ANSI_RESET   "\x1b[0m"

// #define LOG_UNCOND_RED(x) NS_LOG_INFO(ANSI_RED << x << ANSI_RESET)
// #define LOG_UNCOND_GREEN(x) NS_LOG_INFO(ANSI_GREEN << x << ANSI_RESET)
// #define LOG_UNCOND_YELLOW(x) NS_LOG_INFO(ANSI_YELLOW << x << ANSI_RESET)
// #define LOG_UNCOND_BLUE(x) NS_LOG_INFO(ANSI_BLUE << x << ANSI_RESET)

#define LOG_UNCOND_RED(x) NS_LOG_UNCOND(ANSI_RED << x << ANSI_RESET)
#define LOG_UNCOND_GREEN(x) NS_LOG_UNCOND(ANSI_GREEN << x << ANSI_RESET)
#define LOG_UNCOND_YELLOW(x) NS_LOG_UNCOND(ANSI_YELLOW << x << ANSI_RESET)
#define LOG_UNCOND_BLUE(x) NS_LOG_UNCOND(ANSI_BLUE << x << ANSI_RESET)
#define LOG_UNCOND_MAGENTA(x) NS_LOG_UNCOND(ANSI_MAGENTA << x << ANSI_RESET)
#define LOG_UNCOND_CYAN(x) NS_LOG_UNCOND(ANSI_CYAN << x << ANSI_RESET)


#include "raodv-routing-protocol.h"

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-header.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>
#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RAodvRoutingProtocol");

namespace raodv
{
NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);

/// UDP Port for RAODV control traffic
const uint32_t RoutingProtocol::RAODV_PORT = 654;

/**
 * \ingroup raodv
 * \brief Tag used by RAODV implementation
 */
class DeferredRouteOutputTag : public Tag
{
  public:
    /**
     * \brief Constructor
     * \param o the output interface
     */
    DeferredRouteOutputTag(int32_t o = -1)
        : Tag(),
          m_oif(o)
    {
    }

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::raodv::DeferredRouteOutputTag")
                                .SetParent<Tag>()
                                .SetGroupName("RAodv")
                                .AddConstructor<DeferredRouteOutputTag>();
        return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
        return GetTypeId();
    }

    /**
     * \brief Get the output interface
     * \return the output interface
     */
    int32_t GetInterface() const
    {
        return m_oif;
    }

    /**
     * \brief Set the output interface
     * \param oif the output interface
     */
    void SetInterface(int32_t oif)
    {
        m_oif = oif;
    }

    uint32_t GetSerializedSize() const override
    {
        return sizeof(int32_t);
    }

    void Serialize(TagBuffer i) const override
    {
        i.WriteU32(m_oif);
    }

    void Deserialize(TagBuffer i) override
    {
        m_oif = i.ReadU32();
    }

    void Print(std::ostream& os) const override
    {
        os << "DeferredRouteOutputTag: output interface = " << m_oif;
    }

  private:
    /// Positive if output device is fixed in RouteOutput
    int32_t m_oif;
};

NS_OBJECT_ENSURE_REGISTERED(DeferredRouteOutputTag);

// here we just set some default parameters
// like ttl values, rate limit etc

//-----------------------------------------------------------------------------
RoutingProtocol::RoutingProtocol()
    : m_variant(0),
      m_rreqRetries(2),
      m_ttlStart(1),
      m_ttlIncrement(2),
      m_ttlThreshold(7),
      m_timeoutBuffer(2),
      m_rreqRateLimit(10),
      m_rerrRateLimit(10),
      m_activeRouteTimeout(Seconds(3)),
      m_netDiameter(35),
      m_nodeTraversalTime(MilliSeconds(40)),
      m_netTraversalTime(Time((2 * m_netDiameter) * m_nodeTraversalTime)),
      m_pathDiscoveryTime(Time(2 * m_netTraversalTime)),
      m_myRouteTimeout(Time(2 * std::max(m_pathDiscoveryTime, m_activeRouteTimeout))),
      m_helloInterval(Seconds(1)),
      m_allowedHelloLoss(2),
      m_deletePeriod(Time(5 * std::max(m_activeRouteTimeout, m_helloInterval))),
      m_nextHopWait(m_nodeTraversalTime + MilliSeconds(10)),
      m_blackListTimeout(Time(m_rreqRetries * m_netTraversalTime)),
      m_maxQueueLen(64),
      m_maxQueueTime(Seconds(30)),
      m_destinationOnly(true),
      m_gratuitousReply(false),
      m_enableHello(false),
      m_routingTable(m_deletePeriod),
      m_queue(m_maxQueueLen, m_maxQueueTime),
      m_requestId(0),

      // ANIK-NS3-OFFLINE-1
      m_revRequestId(0),

      m_seqNo(0),
      m_rreqIdCache(m_pathDiscoveryTime),

      // ANIK-NS3-OFFLINE-1
      m_revRreqIdCache(m_pathDiscoveryTime),

      m_dpd(m_pathDiscoveryTime),
      m_nb(m_helloInterval),
      m_rreqCount(0),
      m_rerrCount(0),
      m_htimer(Timer::CANCEL_ON_DESTROY),
      m_rreqRateLimitTimer(Timer::CANCEL_ON_DESTROY),
      m_rerrRateLimitTimer(Timer::CANCEL_ON_DESTROY),
      m_lastBcastTime(Seconds(0))
{
    m_nb.SetCallback(MakeCallback(&RoutingProtocol::SendRerrWhenBreaksLinkToNextHop, this));
}

TypeId
RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::raodv::RoutingProtocol")
            .SetParent<Ipv4RoutingProtocol>()
            .SetGroupName("RAodv")
            .AddConstructor<RoutingProtocol>()
            // ANIK-NS3-OFFLINE-1
            .AddAttribute("Variant",
                          "Variant of the algorithm - 0 means original, 1,2 means modifications",
                          UintegerValue(0),
                          MakeUintegerAccessor(&RoutingProtocol::m_variant),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("HelloInterval",
                          "HELLO messages emission interval.",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&RoutingProtocol::m_helloInterval),
                          MakeTimeChecker())
            .AddAttribute("TtlStart",
                          "Initial TTL value for RREQ.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&RoutingProtocol::m_ttlStart),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("TtlIncrement",
                          "TTL increment for each attempt using the expanding ring search for RREQ "
                          "dissemination.",
                          UintegerValue(2),
                          MakeUintegerAccessor(&RoutingProtocol::m_ttlIncrement),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("TtlThreshold",
                          "Maximum TTL value for expanding ring search, TTL = NetDiameter is used "
                          "beyond this value.",
                          UintegerValue(7),
                          MakeUintegerAccessor(&RoutingProtocol::m_ttlThreshold),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("TimeoutBuffer",
                          "Provide a buffer for the timeout.",
                          UintegerValue(2),
                          MakeUintegerAccessor(&RoutingProtocol::m_timeoutBuffer),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("RreqRetries",
                          "Maximum number of retransmissions of RREQ to discover a route",
                          UintegerValue(2),
                          MakeUintegerAccessor(&RoutingProtocol::m_rreqRetries),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("RreqRateLimit",
                          "Maximum number of RREQ per second.",
                          UintegerValue(10),
                          MakeUintegerAccessor(&RoutingProtocol::m_rreqRateLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("RerrRateLimit",
                          "Maximum number of RERR per second.",
                          UintegerValue(10),
                          MakeUintegerAccessor(&RoutingProtocol::m_rerrRateLimit),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("NodeTraversalTime",
                          "Conservative estimate of the average one hop traversal time for packets "
                          "and should include "
                          "queuing delays, interrupt processing times and transfer times.",
                          TimeValue(MilliSeconds(40)),
                          MakeTimeAccessor(&RoutingProtocol::m_nodeTraversalTime),
                          MakeTimeChecker())
            .AddAttribute(
                "NextHopWait",
                "Period of our waiting for the neighbour's RREP_ACK = 10 ms + NodeTraversalTime",
                TimeValue(MilliSeconds(50)),
                MakeTimeAccessor(&RoutingProtocol::m_nextHopWait),
                MakeTimeChecker())
            .AddAttribute("ActiveRouteTimeout",
                          "Period of time during which the route is considered to be valid",
                          TimeValue(Seconds(3)),
                          MakeTimeAccessor(&RoutingProtocol::m_activeRouteTimeout),
                          MakeTimeChecker())
            .AddAttribute("MyRouteTimeout",
                          "Value of lifetime field in RREP generating by this node = 2 * "
                          "max(ActiveRouteTimeout, PathDiscoveryTime)",
                          TimeValue(Seconds(11.2)),
                          MakeTimeAccessor(&RoutingProtocol::m_myRouteTimeout),
                          MakeTimeChecker())
            .AddAttribute("BlackListTimeout",
                          "Time for which the node is put into the blacklist = RreqRetries * "
                          "NetTraversalTime",
                          TimeValue(Seconds(5.6)),
                          MakeTimeAccessor(&RoutingProtocol::m_blackListTimeout),
                          MakeTimeChecker())
            .AddAttribute("DeletePeriod",
                          "DeletePeriod is intended to provide an upper bound on the time for "
                          "which an upstream node A "
                          "can have a neighbor B as an active next hop for destination D, while B "
                          "has invalidated the route to D."
                          " = 5 * max (HelloInterval, ActiveRouteTimeout)",
                          TimeValue(Seconds(15)),
                          MakeTimeAccessor(&RoutingProtocol::m_deletePeriod),
                          MakeTimeChecker())
            .AddAttribute("NetDiameter",
                          "Net diameter measures the maximum possible number of hops between two "
                          "nodes in the network",
                          UintegerValue(35),
                          MakeUintegerAccessor(&RoutingProtocol::m_netDiameter),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "NetTraversalTime",
                "Estimate of the average net traversal time = 2 * NodeTraversalTime * NetDiameter",
                TimeValue(Seconds(2.8)),
                MakeTimeAccessor(&RoutingProtocol::m_netTraversalTime),
                MakeTimeChecker())
            .AddAttribute(
                "PathDiscoveryTime",
                "Estimate of maximum time needed to find route in network = 2 * NetTraversalTime",
                TimeValue(Seconds(5.6)),
                MakeTimeAccessor(&RoutingProtocol::m_pathDiscoveryTime),
                MakeTimeChecker())
            .AddAttribute("MaxQueueLen",
                          "Maximum number of packets that we allow a routing protocol to buffer.",
                          UintegerValue(64),
                          MakeUintegerAccessor(&RoutingProtocol::SetMaxQueueLen,
                                               &RoutingProtocol::GetMaxQueueLen),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("MaxQueueTime",
                          "Maximum time packets can be queued (in seconds)",
                          TimeValue(Seconds(30)),
                          MakeTimeAccessor(&RoutingProtocol::SetMaxQueueTime,
                                           &RoutingProtocol::GetMaxQueueTime),
                          MakeTimeChecker())
            .AddAttribute("AllowedHelloLoss",
                          "Number of hello messages which may be loss for valid link.",
                          UintegerValue(2),
                          MakeUintegerAccessor(&RoutingProtocol::m_allowedHelloLoss),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("GratuitousReply",
                          "Indicates whether a gratuitous RREP should be unicast to the node "
                          "originated route discovery.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RoutingProtocol::SetGratuitousReplyFlag,
                                              &RoutingProtocol::GetGratuitousReplyFlag),
                          MakeBooleanChecker())
            .AddAttribute("DestinationOnly",
                          "Indicates only the destination may respond to this RREQ.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RoutingProtocol::SetDestinationOnlyFlag,
                                              &RoutingProtocol::GetDestinationOnlyFlag),
                          MakeBooleanChecker())
            .AddAttribute("EnableHello",
                          "Indicates whether a hello messages enable.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RoutingProtocol::SetHelloEnable,
                                              &RoutingProtocol::GetHelloEnable),
                          MakeBooleanChecker())
            .AddAttribute("EnableBroadcast",
                          "Indicates whether a broadcast data packets forwarding enable.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&RoutingProtocol::SetBroadcastEnable,
                                              &RoutingProtocol::GetBroadcastEnable),
                          MakeBooleanChecker())
            .AddAttribute("UniformRv",
                          "Access to the underlying UniformRandomVariable",
                          StringValue("ns3::UniformRandomVariable"),
                          MakePointerAccessor(&RoutingProtocol::m_uniformRandomVariable),
                          MakePointerChecker<UniformRandomVariable>());
    return tid;
}

void
RoutingProtocol::SetMaxQueueLen(uint32_t len)
{
    m_maxQueueLen = len;
    m_queue.SetMaxQueueLen(len);
}

void
RoutingProtocol::SetMaxQueueTime(Time t)
{
    m_maxQueueTime = t;
    m_queue.SetQueueTimeout(t);
}

RoutingProtocol::~RoutingProtocol()
{
}

void
RoutingProtocol::DoDispose()
{
    m_ipv4 = nullptr;
    for (auto iter = m_socketAddresses.begin(); iter != m_socketAddresses.end(); iter++)
    {
        iter->first->Close();
    }
    m_socketAddresses.clear();
    for (auto iter = m_socketSubnetBroadcastAddresses.begin();
         iter != m_socketSubnetBroadcastAddresses.end();
         iter++)
    {
        iter->first->Close();
    }
    m_socketSubnetBroadcastAddresses.clear();
    Ipv4RoutingProtocol::DoDispose();
}

void
RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    *stream->GetStream() << "Node: " << m_ipv4->GetObject<Node>()->GetId()
                         << "; Time: " << Now().As(unit)
                         << ", Local time: " << m_ipv4->GetObject<Node>()->GetLocalTime().As(unit)
                         << ", RAODV Routing table" << std::endl;

    m_routingTable.Print(stream, unit);
    *stream->GetStream() << std::endl;
}

int64_t
RoutingProtocol::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uniformRandomVariable->SetStream(stream);
    return 1;
}

void
RoutingProtocol::Start()
{
    NS_LOG_FUNCTION(this);
    if (m_enableHello)
    {
        m_nb.ScheduleTimer();
    }
    m_rreqRateLimitTimer.SetFunction(&RoutingProtocol::RreqRateLimitTimerExpire, this);
    m_rreqRateLimitTimer.Schedule(Seconds(1));

    m_rerrRateLimitTimer.SetFunction(&RoutingProtocol::RerrRateLimitTimerExpire, this);
    m_rerrRateLimitTimer.Schedule(Seconds(1));
}


// this is the function where a route to a destination is requested
// if a valid route already exists, that is returned
// otherwise, deferred until full packet formed
// when done, RREQ will be sent to find a valid route and only then the route will be updated
// this RREQ sending is done in the deffered function
Ptr<Ipv4Route>
RoutingProtocol::RouteOutput(Ptr<Packet> p,
                             const Ipv4Header& header,
                             Ptr<NetDevice> oif,
                             Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << header << (oif ? oif->GetIfIndex() : 0));
    if (!p)
    {
        NS_LOG_DEBUG("Packet is == 0");
        return LoopbackRoute(header, oif); // later
    }
    if (m_socketAddresses.empty())
    {
        sockerr = Socket::ERROR_NOROUTETOHOST;
        NS_LOG_LOGIC("No raodv interfaces");
        Ptr<Ipv4Route> route;
        return route;
    }
    sockerr = Socket::ERROR_NOTERROR;
    Ptr<Ipv4Route> route;
    Ipv4Address dst = header.GetDestination();
    RoutingTableEntry rt;
    if (m_routingTable.LookupValidRoute(dst, rt))
    {
        route = rt.GetRoute();
        NS_ASSERT(route);
        NS_LOG_DEBUG("Exist route to " << route->GetDestination() << " from interface "
                                       << route->GetSource());
        if (oif && route->GetOutputDevice() != oif)
        {
            NS_LOG_DEBUG("Output device doesn't match. Dropped.");
            sockerr = Socket::ERROR_NOROUTETOHOST;
            return Ptr<Ipv4Route>();
        }
        UpdateRouteLifeTime(dst, m_activeRouteTimeout);
        UpdateRouteLifeTime(route->GetGateway(), m_activeRouteTimeout);
        return route;
    }

    // Valid route not found, in this case we return loopback.
    // Actual route request will be deferred until packet will be fully formed,
    // routed to loopback, received from loopback and passed to RouteInput (see below)
    uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice(oif) : -1);
    DeferredRouteOutputTag tag(iif);
    NS_LOG_DEBUG("Valid Route not found");
    if (!p->PeekPacketTag(tag))
    {
        p->AddPacketTag(tag);
    }
    return LoopbackRoute(header, oif);
}

// here we actually send the RREQ packet
// to find a valid route to the destination
void
RoutingProtocol::DeferredRouteOutput(Ptr<const Packet> p,
                                     const Ipv4Header& header,
                                     UnicastForwardCallback ucb,
                                     ErrorCallback ecb)
{
    NS_LOG_FUNCTION(this << p << header);
    NS_ASSERT(p && p != Ptr<Packet>());

    QueueEntry newEntry(p, header, ucb, ecb);
    bool result = m_queue.Enqueue(newEntry);
    if (result)
    {
        NS_LOG_LOGIC("Add packet " << p->GetUid() << " to queue. Protocol "
                                   << (uint16_t)header.GetProtocol());
        RoutingTableEntry rt;
        bool result = m_routingTable.LookupRoute(header.GetDestination(), rt);
        if (!result || ((rt.GetFlag() != IN_SEARCH) && result))
        {
            NS_LOG_LOGIC("Send new RREQ for outbound packet to " << header.GetDestination());
            SendRequest(header.GetDestination());
        }
    }
}

bool
RoutingProtocol::RouteInput(Ptr<const Packet> p,
                            const Ipv4Header& header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback& ucb,
                            const MulticastForwardCallback& mcb,
                            const LocalDeliverCallback& lcb,
                            const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p->GetUid() << header.GetDestination() << idev->GetAddress());
    if (m_socketAddresses.empty())
    {
        NS_LOG_LOGIC("No raodv interfaces");
        return false;
    }
    NS_ASSERT(m_ipv4);
    NS_ASSERT(p);
    // Check if input device supports IP
    NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
    int32_t iif = m_ipv4->GetInterfaceForDevice(idev);

    Ipv4Address dst = header.GetDestination();
    Ipv4Address origin = header.GetSource();

    // Deferred route request
    if (idev == m_lo)
    {
        DeferredRouteOutputTag tag;
        if (p->PeekPacketTag(tag))
        {
            DeferredRouteOutput(p, header, ucb, ecb);
            return true;
        }
    }

    // Duplicate of own packet
    if (IsMyOwnAddress(origin))
    {
        return true;
    }

    // RAODV is not a multicast routing protocol
    if (dst.IsMulticast())
    {
        return false;
    }

    // Broadcast local delivery/forwarding
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ipv4InterfaceAddress iface = j->second;
        if (m_ipv4->GetInterfaceForAddress(iface.GetLocal()) == iif)
        {
            if (dst == iface.GetBroadcast() || dst.IsBroadcast())
            {
                if (m_dpd.IsDuplicate(p, header))
                {
                    NS_LOG_DEBUG("Duplicated packet " << p->GetUid() << " from " << origin
                                                      << ". Drop.");
                    return true;
                }
                UpdateRouteLifeTime(origin, m_activeRouteTimeout);
                Ptr<Packet> packet = p->Copy();
                if (!lcb.IsNull())
                {
                    NS_LOG_LOGIC("Broadcast local delivery to " << iface.GetLocal());
                    lcb(p, header, iif);
                    // Fall through to additional processing
                }
                else
                {
                    NS_LOG_ERROR("Unable to deliver packet locally due to null callback "
                                 << p->GetUid() << " from " << origin);
                    ecb(p, header, Socket::ERROR_NOROUTETOHOST);
                }
                if (!m_enableBroadcast)
                {
                    return true;
                }
                if (header.GetProtocol() == UdpL4Protocol::PROT_NUMBER)
                {
                    UdpHeader udpHeader;
                    p->PeekHeader(udpHeader);
                    if (udpHeader.GetDestinationPort() == RAODV_PORT)
                    {
                        // RAODV packets sent in broadcast are already managed
                        return true;
                    }
                }
                if (header.GetTtl() > 1)
                {
                    NS_LOG_LOGIC("Forward broadcast. TTL " << (uint16_t)header.GetTtl());
                    RoutingTableEntry toBroadcast;
                    if (m_routingTable.LookupRoute(dst, toBroadcast))
                    {
                        Ptr<Ipv4Route> route = toBroadcast.GetRoute();
                        ucb(route, packet, header);
                    }
                    else
                    {
                        NS_LOG_DEBUG("No route to forward broadcast. Drop packet " << p->GetUid());
                    }
                }
                else
                {
                    NS_LOG_DEBUG("TTL exceeded. Drop packet " << p->GetUid());
                }
                return true;
            }
        }
    }

    // Unicast local delivery
    if (m_ipv4->IsDestinationAddress(dst, iif))
    {
        UpdateRouteLifeTime(origin, m_activeRouteTimeout);
        RoutingTableEntry toOrigin;
        if (m_routingTable.LookupValidRoute(origin, toOrigin))
        {
            UpdateRouteLifeTime(toOrigin.GetNextHop(), m_activeRouteTimeout);
            m_nb.Update(toOrigin.GetNextHop(), m_activeRouteTimeout);
        }
        if (!lcb.IsNull())
        {
            NS_LOG_LOGIC("Unicast local delivery to " << dst);
            lcb(p, header, iif);
        }
        else
        {
            NS_LOG_ERROR("Unable to deliver packet locally due to null callback "
                         << p->GetUid() << " from " << origin);
            ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        }
        return true;
    }

    // Check if input device supports IP forwarding
    if (!m_ipv4->IsForwarding(iif))
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        return true;
    }

    // Forwarding
    return Forwarding(p, header, ucb, ecb);
}

bool
RoutingProtocol::Forwarding(Ptr<const Packet> p,
                            const Ipv4Header& header,
                            UnicastForwardCallback ucb,
                            ErrorCallback ecb)
{
    NS_LOG_FUNCTION(this);
    Ipv4Address dst = header.GetDestination();
    Ipv4Address origin = header.GetSource();
    m_routingTable.Purge();
    RoutingTableEntry toDst;
    if (m_routingTable.LookupRoute(dst, toDst))
    {
        if (toDst.GetFlag() == VALID)
        {
            Ptr<Ipv4Route> route = toDst.GetRoute();
            NS_LOG_LOGIC(route->GetSource() << " forwarding to " << dst << " from " << origin
                                            << " packet " << p->GetUid());

            /*
             *  Each time a route is used to forward a data packet, its Active Route
             *  Lifetime field of the source, destination and the next hop on the
             *  path to the destination is updated to be no less than the current
             *  time plus ActiveRouteTimeout.
             */
            UpdateRouteLifeTime(origin, m_activeRouteTimeout);
            UpdateRouteLifeTime(dst, m_activeRouteTimeout);
            UpdateRouteLifeTime(route->GetGateway(), m_activeRouteTimeout);
            /*
             *  Since the route between each originator and destination pair is expected to be
             * symmetric, the Active Route Lifetime for the previous hop, along the reverse path
             * back to the IP source, is also updated to be no less than the current time plus
             * ActiveRouteTimeout
             */
            RoutingTableEntry toOrigin;
            m_routingTable.LookupRoute(origin, toOrigin);
            UpdateRouteLifeTime(toOrigin.GetNextHop(), m_activeRouteTimeout);

            m_nb.Update(route->GetGateway(), m_activeRouteTimeout);
            m_nb.Update(toOrigin.GetNextHop(), m_activeRouteTimeout);

            ucb(route, p, header);
            return true;
        }
        else
        {
            if (toDst.GetValidSeqNo())
            {
                SendRerrWhenNoRouteToForward(dst, toDst.GetSeqNo(), origin);
                NS_LOG_DEBUG("Drop packet " << p->GetUid() << " because no route to forward it.");
                return false;
            }
        }
    }
    NS_LOG_LOGIC("route not found to " << dst << ". Send RERR message.");
    NS_LOG_DEBUG("Drop packet " << p->GetUid() << " because no route to forward it.");
    SendRerrWhenNoRouteToForward(dst, 0, origin);
    return false;
}

void
RoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_ASSERT(ipv4);
    NS_ASSERT(!m_ipv4);

    m_ipv4 = ipv4;

    // Create lo route. It is asserted that the only one interface up for now is loopback
    NS_ASSERT(m_ipv4->GetNInterfaces() == 1 &&
              m_ipv4->GetAddress(0, 0).GetLocal() == Ipv4Address("127.0.0.1"));
    m_lo = m_ipv4->GetNetDevice(0);
    NS_ASSERT(m_lo);
    // Remember lo route
    RoutingTableEntry rt(
        /*dev=*/m_lo,
        /*dst=*/Ipv4Address::GetLoopback(),
        /*vSeqNo=*/true,
        /*seqNo=*/0,
        /*iface=*/Ipv4InterfaceAddress(Ipv4Address::GetLoopback(), Ipv4Mask("255.0.0.0")),
        /*hops=*/1,
        /*nextHop=*/Ipv4Address::GetLoopback(),
        /*lifetime=*/Simulator::GetMaximumSimulationTime());
    m_routingTable.AddRoute(rt);

    Simulator::ScheduleNow(&RoutingProtocol::Start, this);
}

void
RoutingProtocol::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << m_ipv4->GetAddress(i, 0).GetLocal());
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    if (l3->GetNAddresses(i) > 1)
    {
        NS_LOG_WARN("RAODV does not work with more then one address per each interface.");
    }
    Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
    if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
    {
        return;
    }

    // Create a socket to listen only on this interface
    Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket);
    socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvRAodv, this));
    socket->BindToNetDevice(l3->GetNetDevice(i));
    socket->Bind(InetSocketAddress(iface.GetLocal(), RAODV_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    m_socketAddresses.insert(std::make_pair(socket, iface));

    // create also a subnet broadcast socket
    socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket);
    socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvRAodv, this));
    socket->BindToNetDevice(l3->GetNetDevice(i));
    socket->Bind(InetSocketAddress(iface.GetBroadcast(), RAODV_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetIpRecvTtl(true);
    m_socketSubnetBroadcastAddresses.insert(std::make_pair(socket, iface));

    // Add local broadcast record to the routing table
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
    RoutingTableEntry rt(/*dev=*/dev,
                         /*dst=*/iface.GetBroadcast(),
                         /*vSeqNo=*/true,
                         /*seqNo=*/0,
                         /*iface=*/iface,
                         /*hops=*/1,
                         /*nextHop=*/iface.GetBroadcast(),
                         /*lifetime=*/Simulator::GetMaximumSimulationTime());
    m_routingTable.AddRoute(rt);

    if (l3->GetInterface(i)->GetArpCache())
    {
        m_nb.AddArpCache(l3->GetInterface(i)->GetArpCache());
    }

    // Allow neighbor manager use this interface for layer 2 feedback if possible
    Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice>();
    if (!wifi)
    {
        return;
    }
    Ptr<WifiMac> mac = wifi->GetMac();
    if (!mac)
    {
        return;
    }

    mac->TraceConnectWithoutContext("DroppedMpdu",
                                    MakeCallback(&RoutingProtocol::NotifyTxError, this));
}

void
RoutingProtocol::NotifyTxError(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    m_nb.GetTxErrorCallback()(mpdu->GetHeader());
}

void
RoutingProtocol::NotifyInterfaceDown(uint32_t i)
{
    NS_LOG_FUNCTION(this << m_ipv4->GetAddress(i, 0).GetLocal());

    // Disable layer 2 link state monitoring (if possible)
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    Ptr<NetDevice> dev = l3->GetNetDevice(i);
    Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice>();
    if (wifi)
    {
        Ptr<WifiMac> mac = wifi->GetMac()->GetObject<AdhocWifiMac>();
        if (mac)
        {
            mac->TraceDisconnectWithoutContext("DroppedMpdu",
                                               MakeCallback(&RoutingProtocol::NotifyTxError, this));
            m_nb.DelArpCache(l3->GetInterface(i)->GetArpCache());
        }
    }

    // Close socket
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(m_ipv4->GetAddress(i, 0));
    NS_ASSERT(socket);
    socket->Close();
    m_socketAddresses.erase(socket);

    // Close socket
    socket = FindSubnetBroadcastSocketWithInterfaceAddress(m_ipv4->GetAddress(i, 0));
    NS_ASSERT(socket);
    socket->Close();
    m_socketSubnetBroadcastAddresses.erase(socket);

    if (m_socketAddresses.empty())
    {
        NS_LOG_LOGIC("No raodv interfaces");
        m_htimer.Cancel();
        m_nb.Clear();
        m_routingTable.Clear();
        return;
    }
    m_routingTable.DeleteAllRoutesFromInterface(m_ipv4->GetAddress(i, 0));
}

void
RoutingProtocol::NotifyAddAddress(uint32_t i, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << " interface " << i << " address " << address);
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    if (!l3->IsUp(i))
    {
        return;
    }
    if (l3->GetNAddresses(i) == 1)
    {
        Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
        Ptr<Socket> socket = FindSocketWithInterfaceAddress(iface);
        if (!socket)
        {
            if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
            {
                return;
            }
            // Create a socket to listen only on this interface
            Ptr<Socket> socket =
                Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvRAodv, this));
            socket->BindToNetDevice(l3->GetNetDevice(i));
            socket->Bind(InetSocketAddress(iface.GetLocal(), RAODV_PORT));
            socket->SetAllowBroadcast(true);
            m_socketAddresses.insert(std::make_pair(socket, iface));

            // create also a subnet directed broadcast socket
            socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvRAodv, this));
            socket->BindToNetDevice(l3->GetNetDevice(i));
            socket->Bind(InetSocketAddress(iface.GetBroadcast(), RAODV_PORT));
            socket->SetAllowBroadcast(true);
            socket->SetIpRecvTtl(true);
            m_socketSubnetBroadcastAddresses.insert(std::make_pair(socket, iface));

            // Add local broadcast record to the routing table
            Ptr<NetDevice> dev =
                m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
            RoutingTableEntry rt(/*dev=*/dev,
                                 /*dst=*/iface.GetBroadcast(),
                                 /*vSeqNo=*/true,
                                 /*seqNo=*/0,
                                 /*iface=*/iface,
                                 /*hops=*/1,
                                 /*nextHop=*/iface.GetBroadcast(),
                                 /*lifetime=*/Simulator::GetMaximumSimulationTime());
            m_routingTable.AddRoute(rt);
        }
    }
    else
    {
        NS_LOG_LOGIC("RAODV does not work with more then one address per each interface. Ignore "
                     "added address");
    }
}

void
RoutingProtocol::NotifyRemoveAddress(uint32_t i, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(address);
    if (socket)
    {
        m_routingTable.DeleteAllRoutesFromInterface(address);
        socket->Close();
        m_socketAddresses.erase(socket);

        Ptr<Socket> unicastSocket = FindSubnetBroadcastSocketWithInterfaceAddress(address);
        if (unicastSocket)
        {
            unicastSocket->Close();
            m_socketAddresses.erase(unicastSocket);
        }

        Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
        if (l3->GetNAddresses(i))
        {
            Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
            // Create a socket to listen only on this interface
            Ptr<Socket> socket =
                Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvRAodv, this));
            // Bind to any IP address so that broadcasts can be received
            socket->BindToNetDevice(l3->GetNetDevice(i));
            socket->Bind(InetSocketAddress(iface.GetLocal(), RAODV_PORT));
            socket->SetAllowBroadcast(true);
            socket->SetIpRecvTtl(true);
            m_socketAddresses.insert(std::make_pair(socket, iface));

            // create also a unicast socket
            socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvRAodv, this));
            socket->BindToNetDevice(l3->GetNetDevice(i));
            socket->Bind(InetSocketAddress(iface.GetBroadcast(), RAODV_PORT));
            socket->SetAllowBroadcast(true);
            socket->SetIpRecvTtl(true);
            m_socketSubnetBroadcastAddresses.insert(std::make_pair(socket, iface));

            // Add local broadcast record to the routing table
            Ptr<NetDevice> dev =
                m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
            RoutingTableEntry rt(/*dev=*/dev,
                                 /*dst=*/iface.GetBroadcast(),
                                 /*vSeqNo=*/true,
                                 /*seqNo=*/0,
                                 /*iface=*/iface,
                                 /*hops=*/1,
                                 /*nextHop=*/iface.GetBroadcast(),
                                 /*lifetime=*/Simulator::GetMaximumSimulationTime());
            m_routingTable.AddRoute(rt);
        }
        if (m_socketAddresses.empty())
        {
            NS_LOG_LOGIC("No raodv interfaces");
            m_htimer.Cancel();
            m_nb.Clear();
            m_routingTable.Clear();
            return;
        }
    }
    else
    {
        NS_LOG_LOGIC("Remove address not participating in RAODV operation");
    }
}

// just a linear search
bool
RoutingProtocol::IsMyOwnAddress(Ipv4Address src)
{
    NS_LOG_FUNCTION(this << src);
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ipv4InterfaceAddress iface = j->second;
        if (src == iface.GetLocal())
        {
            return true;
        }
    }
    return false;
}

Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute(const Ipv4Header& hdr, Ptr<NetDevice> oif) const
{
    NS_LOG_FUNCTION(this << hdr);
    NS_ASSERT(m_lo);
    Ptr<Ipv4Route> rt = Create<Ipv4Route>();
    rt->SetDestination(hdr.GetDestination());
    //
    // Source address selection here is tricky.  The loopback route is
    // returned when RAODV does not have a route; this causes the packet
    // to be looped back and handled (cached) in RouteInput() method
    // while a route is found. However, connection-oriented protocols
    // like TCP need to create an endpoint four-tuple (src, src port,
    // dst, dst port) and create a pseudo-header for checksumming.  So,
    // RAODV needs to guess correctly what the eventual source address
    // will be.
    //
    // For single interface, single address nodes, this is not a problem.
    // When there are possibly multiple outgoing interfaces, the policy
    // implemented here is to pick the first available RAODV interface.
    // If RouteOutput() caller specified an outgoing interface, that
    // further constrains the selection of source address
    //
    auto j = m_socketAddresses.begin();
    if (oif)
    {
        // Iterate to find an address on the oif device
        for (j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
        {
            Ipv4Address addr = j->second.GetLocal();
            int32_t interface = m_ipv4->GetInterfaceForAddress(addr);
            if (oif == m_ipv4->GetNetDevice(static_cast<uint32_t>(interface)))
            {
                rt->SetSource(addr);
                break;
            }
        }
    }
    else
    {
        rt->SetSource(j->second.GetLocal());
    }
    NS_ASSERT_MSG(rt->GetSource() != Ipv4Address(), "Valid RAODV source address not found");
    rt->SetGateway(Ipv4Address("127.0.0.1"));
    rt->SetOutputDevice(m_lo);
    return rt;
}

// first we check rate limit, if exceeded, we schedule this sending for later
// then, we incrementally build an RReqHeader
// first set the dst
// if there already exists a valid route in the routing table
// we set the ttl as such and cap it with the diameter of network
// then, set dstseqno, unknownseqno, hop, flag, lifetime of the rt
// here rt means Routing table entry
// we set its flag to IN_SEARCH, meaning we are now searching for a better version of this route
// then we finally update this new rt in the routing table
// and if no route exists in the routing table, we build a new rt similarly
// and add that route to the routing table
// then, we again start building our RReqHeader object setting necessary flags
// then build a packet with ttltag, req_header and appropriate typeHeader
// and broadcast it to every available interface
// note that, the hopcount of rreqHeader is not set here and therefore it is by default 0 when set
// and it keeps increasing by one, whenever it touches another node
// this increment is done in RecvRequest
void
RoutingProtocol::SendRequest(Ipv4Address dst)
{
    NS_LOG_FUNCTION(this << dst);
    // A node SHOULD NOT originate more than RREQ_RATELIMIT RREQ messages per second.
    if (m_rreqCount == m_rreqRateLimit)
    {
        Simulator::Schedule(m_rreqRateLimitTimer.GetDelayLeft() + MicroSeconds(100),
                            &RoutingProtocol::SendRequest,
                            this,
                            dst);
        return;
    }
    else
    {
        m_rreqCount++;
    }
    // Create RREQ header
    RreqHeader rreqHeader;
    rreqHeader.SetDst(dst);

    RoutingTableEntry rt;
    // Updating the Hop field in Routing Table to manage the expanding ring search
    // capping by m_netDiameter
    uint16_t ttl = m_ttlStart;
    if (m_routingTable.LookupRoute(dst, rt))
    {
        if (rt.GetFlag() != IN_SEARCH)
        {
            ttl = std::min<uint16_t>(rt.GetHop() + m_ttlIncrement, m_netDiameter);
        }
        else
        {
            ttl = rt.GetHop() + m_ttlIncrement;
            if (ttl > m_ttlThreshold)
            {
                ttl = m_netDiameter;
            }
        }
        if (ttl == m_netDiameter)
        {
            rt.IncrementRreqCnt();
        }
        if (rt.GetValidSeqNo())
        {
            rreqHeader.SetDstSeqno(rt.GetSeqNo());
        }
        else
        {
            rreqHeader.SetUnknownSeqno(true);
        }
        rt.SetHop(ttl);
        rt.SetFlag(IN_SEARCH);
        rt.SetLifeTime(m_pathDiscoveryTime);
        m_routingTable.Update(rt);
    }
    else
    {
        // dev, iface, nexthop are not yet known, so just default or null kept
        rreqHeader.SetUnknownSeqno(true);
        Ptr<NetDevice> dev = nullptr;
        RoutingTableEntry newEntry(/*dev=*/dev,
                                   /*dst=*/dst,
                                   /*vSeqNo=*/false,
                                   /*seqNo=*/0,
                                   /*iface=*/Ipv4InterfaceAddress(),
                                   /*hops=*/ttl,
                                   /*nextHop=*/Ipv4Address(),
                                   /*lifetime=*/m_pathDiscoveryTime);
        // Check if TtlStart == NetDiameter
        if (ttl == m_netDiameter)
        {
            newEntry.IncrementRreqCnt();
        }
        newEntry.SetFlag(IN_SEARCH);
        m_routingTable.AddRoute(newEntry);
    }

    if (m_gratuitousReply)
    {
        rreqHeader.SetGratuitousRrep(true);
    }
    if (m_destinationOnly)
    {
        rreqHeader.SetDestinationOnly(true);
    }

    m_seqNo++;
    rreqHeader.SetOriginSeqno(m_seqNo);
    m_requestId++;
    rreqHeader.SetId(m_requestId);

    // Send RREQ as subnet directed broadcast from each interface used by raodv
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;

        rreqHeader.SetOrigin(iface.GetLocal());
        m_rreqIdCache.IsDuplicate(iface.GetLocal(), m_requestId);

        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag tag;
        tag.SetTtl(ttl);
        packet->AddPacketTag(tag);
        packet->AddHeader(rreqHeader);
        TypeHeader tHeader(AODVTYPE_RREQ);
        packet->AddHeader(tHeader);
        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }
        NS_LOG_DEBUG("Send RREQ with id " << rreqHeader.GetId() << " to socket");
        m_lastBcastTime = Simulator::Now();
        Simulator::Schedule(Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10))),
                            &RoutingProtocol::SendTo,
                            this,
                            socket,
                            packet,
                            destination);
    }
    ScheduleRreqRetry(dst);
}

void
RoutingProtocol::SendTo(Ptr<Socket> socket, Ptr<Packet> packet, Ipv4Address destination)
{
    socket->SendTo(packet, 0, InetSocketAddress(destination, RAODV_PORT));
}

void
RoutingProtocol::ScheduleRreqRetry(Ipv4Address dst)
{
    NS_LOG_FUNCTION(this << dst);
    if (m_addressReqTimer.find(dst) == m_addressReqTimer.end())
    {
        Timer timer(Timer::CANCEL_ON_DESTROY);
        m_addressReqTimer[dst] = timer;
    }
    m_addressReqTimer[dst].SetFunction(&RoutingProtocol::RouteRequestTimerExpire, this);
    m_addressReqTimer[dst].Cancel();
    m_addressReqTimer[dst].SetArguments(dst);
    RoutingTableEntry rt;
    m_routingTable.LookupRoute(dst, rt);
    Time retry;
    if (rt.GetHop() < m_netDiameter)
    {
        retry = 2 * m_nodeTraversalTime * (rt.GetHop() + m_timeoutBuffer);
    }
    else
    {
        NS_ABORT_MSG_UNLESS(rt.GetRreqCnt() > 0, "Unexpected value for GetRreqCount ()");
        uint16_t backoffFactor = rt.GetRreqCnt() - 1;
        NS_LOG_LOGIC("Applying binary exponential backoff factor " << backoffFactor);
        retry = m_netTraversalTime * (1 << backoffFactor);
    }
    m_addressReqTimer[dst].Schedule(retry);
    NS_LOG_LOGIC("Scheduled RREQ retry in " << retry.As(Time::S));
}

// in this function, we handle what will happen when different types of packets will be received
// find we determine the sender and receiver
// then we check the packet validity
// then case by case we call functions based on header type
// for raodv, we will actually need to handle a new type of header named AODVTYPE_REV_RREQ
// so we will need a new function for receiving that type of packet
// upon receiving that type of packet, we will actually braodcast it if intermediate
// and if it reached the src node, then we will actually send packets from queue, much like we do when we
// receive RREP at the src node
// so that new function will be kindof a mixture of both RecvRequest and RecvReply
// since it will be broadcast, we will also need to check it against cache, probably
//  so keep a separate cache for rrreq just like we have for rreq
void
RoutingProtocol::RecvRAodv(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Address sourceAddress;
    Ptr<Packet> packet = socket->RecvFrom(sourceAddress);
    InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom(sourceAddress);
    // upto this point, we extracted the ipv4 address of the sender
    Ipv4Address sender = inetSourceAddr.GetIpv4();
    Ipv4Address receiver;

    // now we determine which receiver socket the packet came from
    if (m_socketAddresses.find(socket) != m_socketAddresses.end())
    {
        receiver = m_socketAddresses[socket].GetLocal();
    }
    else if (m_socketSubnetBroadcastAddresses.find(socket) !=
             m_socketSubnetBroadcastAddresses.end())
    {
        receiver = m_socketSubnetBroadcastAddresses[socket].GetLocal();
    }
    else
    {
        NS_ASSERT_MSG(false, "Received a packet from an unknown socket");
    }
    NS_LOG_DEBUG("RAODV node " << this << " received a RAODV packet from " << sender << " to "
                              << receiver);

    UpdateRouteToNeighbor(sender, receiver);
    TypeHeader tHeader(AODVTYPE_RREQ);
    packet->RemoveHeader(tHeader);
    // checks header validity first
    if (!tHeader.IsValid())
    {
        NS_LOG_DEBUG("RAODV message " << packet->GetUid() << " with unknown type received: "
                                     << tHeader.Get() << ". Drop");
        return; // drop
    }
    // sets which function to call based on the header type
    switch (tHeader.Get())
    {
        case AODVTYPE_RREQ: {
            RecvRequest(packet, receiver, sender);
            break;
        }
        case AODVTYPE_RREP: {
            RecvReply(packet, receiver, sender);
            break;
        }
        case AODVTYPE_RERR: {
            RecvError(packet, sender);
            break;
        }
        case AODVTYPE_RREP_ACK: {
            RecvReplyAck(sender);
            break;
        }
        case AODVTYPE_REV_RREQ: {
            RecvRevRequest(packet, receiver, sender);
            break;
        }
    }
}

// just updating the route to addr in the routing table
// and set the new lifetime
bool
RoutingProtocol::UpdateRouteLifeTime(Ipv4Address addr, Time lifetime)
{
    NS_LOG_FUNCTION(this << addr << lifetime);
    RoutingTableEntry rt;
    if (m_routingTable.LookupRoute(addr, rt))
    {
        if (rt.GetFlag() == VALID)
        {
            NS_LOG_DEBUG("Updating VALID route");
            rt.SetRreqCnt(0);
            rt.SetLifeTime(std::max(lifetime, rt.GetLifeTime()));
            m_routingTable.Update(rt);
            return true;
        }
    }
    return false;
}

// this function is called we receive a packet from a source
// then we can surely infer that this node is a direct neighbour of mine
// so just add a rt entry with hop = 1 and next hop = that node
void
RoutingProtocol::UpdateRouteToNeighbor(Ipv4Address sender, Ipv4Address receiver)
{
    NS_LOG_FUNCTION(this << "sender " << sender << " receiver " << receiver);
    RoutingTableEntry toNeighbor;
    if (!m_routingTable.LookupRoute(sender, toNeighbor))
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry newEntry(
            /*dev=*/dev,
            /*dst=*/sender,
            /*vSeqNo=*/false,
            /*seqNo=*/0,
            /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
            /*hops=*/1,
            /*nextHop=*/sender,
            /*lifetime=*/m_activeRouteTimeout);
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        if (toNeighbor.GetValidSeqNo() && (toNeighbor.GetHop() == 1) &&
            (toNeighbor.GetOutputDevice() == dev))
        {
            toNeighbor.SetLifeTime(std::max(m_activeRouteTimeout, toNeighbor.GetLifeTime()));
        }
        else
        {
            RoutingTableEntry newEntry(
                /*dev=*/dev,
                /*dst=*/sender,
                /*vSeqNo=*/false,
                /*seqNo=*/0,
                /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
                /*hops=*/1,
                /*nextHop=*/sender,
                /*lifetime=*/std::max(m_activeRouteTimeout, toNeighbor.GetLifeTime()));
            m_routingTable.Update(newEntry);
        }
    }
}

// first check against blacklist
// and then also extract the id and check against cache for duplicate
// note that, the IsDuplicate checking function also does the side job of updating the cache
// then we increment the hopCount by 1 and set the hopCount
// now we check whether there is a route to the origin from me in the routing table
// if no, create a new entry
// if yes, update its fields
// also, add entry to the neighbor from which i got this, with hop = 1
// and if already exists, update the fields
// finally check whether the destination of the rreqheader is me myself
// if yes, SendReply, this will basically send a unicast RREP
// and if it not myself, then create a new packet with rreq header and braodcast it
// #TODO but in modified raodv, in this place, we should rather call anotehr function
// for broadcasting a rev_rreq 
// also, to stop the intermediate sending reply, we may just set the destOnly flag to true
void
RoutingProtocol::RecvRequest(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src)
{
    NS_LOG_FUNCTION(this);
    RreqHeader rreqHeader;
    p->RemoveHeader(rreqHeader);

    // A node ignores all RREQs received from any node in its blacklist
    RoutingTableEntry toPrev;
    if (m_routingTable.LookupRoute(src, toPrev))
    {
        if (toPrev.IsUnidirectional())
        {
            NS_LOG_DEBUG("Ignoring RREQ from node in blacklist");
            return;
        }
    }

    uint32_t id = rreqHeader.GetId();
    Ipv4Address origin = rreqHeader.GetOrigin();

    /*
     *  Node checks to determine whether it has received a RREQ with the same Originator IP Address
     * and RREQ ID. If such a RREQ has been received, the node silently discards the newly received
     * RREQ.
     */
    if (m_rreqIdCache.IsDuplicate(origin, id))
    {
        // ANIK-NS3-OFFLINE-1
        // VARIANT 2
        // In my proposed variant 2
        // I intentionally receive duplicate RREQ packets at the actual destination
        // and also send RREP messsages for them
        // this, along with, RREP from the first time the destination receives it
        // actually simulates multiple simulatneous unicast instead of braodcast in original RAODV
        // Intuition: 
        // Broadcasting seems like a from-scratch-search from the reverse direction
        // but since we have already found the destination, maybe more than once,
        // utilizing all those distinct unicast paths seems to be more efficient 
        // than direct broadcasting
        if(m_variant == 2){
            if (IsMyOwnAddress(rreqHeader.GetDst()))
            {
                LOG_UNCOND_CYAN("Doing intentional Multicast");

                RoutingTableEntry toOrigin;
                m_routingTable.LookupRoute(origin, toOrigin);

                LOG_UNCOND_YELLOW("Attention! Destination reached. Sending RREP message.");
                NS_LOG_DEBUG("Send reply since I am the destination");
                SendReply(rreqHeader, toOrigin);
            }
        } else{
            NS_LOG_DEBUG("Ignoring RREQ due to duplicate");
        }
        return;
    }

    // Increment RREQ hop count
    uint8_t hop = rreqHeader.GetHopCount() + 1;
    rreqHeader.SetHopCount(hop);

    /*
     *  When the reverse route is created or updated, the following actions on the route are also
     * carried out:
     *  1. the Originator Sequence Number from the RREQ is compared to the corresponding destination
     * sequence number in the route table entry and copied if greater than the existing value there
     *  2. the valid sequence number field is set to true;
     *  3. the next hop in the routing table becomes the node from which the  RREQ was received
     *  4. the hop count is copied from the Hop Count in the RREQ message;
     *  5. the Lifetime is set to be the maximum of (ExistingLifetime, MinimalLifetime), where
     *     MinimalLifetime = current time + 2*NetTraversalTime - 2*HopCount*NodeTraversalTime
     */
    RoutingTableEntry toOrigin;
    if (!m_routingTable.LookupRoute(origin, toOrigin))
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry newEntry(
            /*dev=*/dev,
            /*dst=*/origin,
            /*vSeqNo=*/true,
            /*seqNo=*/rreqHeader.GetOriginSeqno(),
            /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
            /*hops=*/hop,
            /*nextHop=*/src,
            /*lifetime=*/Time(2 * m_netTraversalTime - 2 * hop * m_nodeTraversalTime));
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        if (toOrigin.GetValidSeqNo())
        {
            if (int32_t(rreqHeader.GetOriginSeqno()) - int32_t(toOrigin.GetSeqNo()) > 0)
            {
                toOrigin.SetSeqNo(rreqHeader.GetOriginSeqno());
            }
        }
        else
        {
            toOrigin.SetSeqNo(rreqHeader.GetOriginSeqno());
        }
        toOrigin.SetValidSeqNo(true);
        toOrigin.SetNextHop(src);
        toOrigin.SetOutputDevice(m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver)));
        toOrigin.SetInterface(m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0));
        toOrigin.SetHop(hop);
        toOrigin.SetLifeTime(std::max(Time(2 * m_netTraversalTime - 2 * hop * m_nodeTraversalTime),
                                      toOrigin.GetLifeTime()));
        m_routingTable.Update(toOrigin);
        // m_nb.Update (src, Time (AllowedHelloLoss * HelloInterval));
    }

    RoutingTableEntry toNeighbor;
    if (!m_routingTable.LookupRoute(src, toNeighbor))
    {
        NS_LOG_DEBUG("Neighbor:" << src << " not found in routing table. Creating an entry");
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry newEntry(dev,
                                   src,
                                   false,
                                   rreqHeader.GetOriginSeqno(),
                                   m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
                                   1,
                                   src,
                                   m_activeRouteTimeout);
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        toNeighbor.SetLifeTime(m_activeRouteTimeout);
        toNeighbor.SetValidSeqNo(false);
        toNeighbor.SetSeqNo(rreqHeader.GetOriginSeqno());
        toNeighbor.SetFlag(VALID);
        toNeighbor.SetOutputDevice(m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver)));
        toNeighbor.SetInterface(m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0));
        toNeighbor.SetHop(1);
        toNeighbor.SetNextHop(src);
        m_routingTable.Update(toNeighbor);
    }
    m_nb.Update(src, Time(m_allowedHelloLoss * m_helloInterval));

    NS_LOG_LOGIC(receiver << " receive RREQ with hop count "
                          << static_cast<uint32_t>(rreqHeader.GetHopCount()) << " ID "
                          << rreqHeader.GetId() << " to destination " << rreqHeader.GetDst());

    // ANIK-NS3-OFFLINE-1
    // note that, this is place of main difference between AODV and RAODV
    // there is room to modify what is done here
    // A node generates a REV_RREQ (or, RREP for variants) if either:
    if (IsMyOwnAddress(rreqHeader.GetDst()))
    {
        m_routingTable.LookupRoute(origin, toOrigin);

        // ANIK-NS3-OFFLINE-1
        // VARIANT 1
        // in variant 1, I send both unicast and broadcast message
        // note that, unicast, here is not trivially included in the broadcast
        // because, while broadcasting, we are just blindly flooding the packet,
        // without taking help from routing table
        // however, for the one unicast route, we already had found it as shortest path
        // so there is some probability that this shortest path has not broken
        // in the meantime
        // Intuition:
        // Fully ignoring the previously found shortest unicast path seems to be wasteful
        // Since, we are already broadcasting, it does not take much more to also unicast 
        // like AODV
        // So, this variant is a plain union of AODV and RAODV
        if(m_variant == 1){
            LOG_UNCOND_YELLOW("Attention! Destination reached. Sending RREP message.");
            NS_LOG_DEBUG("Send reply since I am the destination");
            SendReply(rreqHeader, toOrigin);

            LOG_UNCOND_YELLOW("Attention! Destination reached. Sending REV_RREQ message.");
            NS_LOG_DEBUG("Send REV-RREQ since I am the destination");
            SendRevRequest(rreqHeader, toOrigin);
        }
        // variant 2 is multicast
        // so, here upon first reception
        // it just sends RREP as it used to do in normal AODV
        else if(m_variant == 2){
            LOG_UNCOND_YELLOW("Attention! Destination reached. Sending RREP message.");
            NS_LOG_DEBUG("Send reply since I am the destination");
            SendReply(rreqHeader, toOrigin);
        }
        // this is the original RAODV
        // here it sends only REV_RREQ 
        else{
            LOG_UNCOND_YELLOW("Attention! Destination reached. Sending REV_RREQ message.");
            NS_LOG_DEBUG("Send REV-RREQ since I am the destination");
            SendRevRequest(rreqHeader, toOrigin);
        }

        return;
    }

    /*
     * 
     * 
     */
    
    /*
     * (ii) or it has an active route to the destination, the destination sequence number in the
     * node's existing route table entry for the destination is valid and greater than or equal to
     * the Destination Sequence Number of the RREQ, and the "destination only" flag is NOT set.
     */
    RoutingTableEntry toDst;
    Ipv4Address dst = rreqHeader.GetDst();
    if (m_routingTable.LookupRoute(dst, toDst))
    {
        /*
         * Drop RREQ, This node RREP will make a loop.
         */
        if (toDst.GetNextHop() == src)
        {
            NS_LOG_DEBUG("Drop RREQ from " << src << ", dest next hop " << toDst.GetNextHop());
            return;
        }
        /*
         * The Destination Sequence number for the requested destination is set to the maximum of
         * the corresponding value received in the RREQ message, and the destination sequence value
         * currently maintained by the node for the requested destination. However, the forwarding
         * node MUST NOT modify its maintained value for the destination sequence number, even if
         * the value received in the incoming RREQ is larger than the value currently maintained by
         * the forwarding node.
         */
        if ((rreqHeader.GetUnknownSeqno() ||
             (int32_t(toDst.GetSeqNo()) - int32_t(rreqHeader.GetDstSeqno()) >= 0)) &&
            toDst.GetValidSeqNo())
        {
            // if (!rreqHeader.GetDestinationOnly() && toDst.GetFlag() == VALID)
            // {
            //     m_routingTable.LookupRoute(origin, toOrigin);
            //     SendReplyByIntermediateNode(toDst, toOrigin, rreqHeader.GetGratuitousRrep());
            //     return;
            // }
            rreqHeader.SetDstSeqno(toDst.GetSeqNo());
            rreqHeader.SetUnknownSeqno(false);
        }
    }

    SocketIpTtlTag tag;
    p->RemovePacketTag(tag);
    if (tag.GetTtl() < 2)
    {
        NS_LOG_DEBUG("TTL exceeded. Drop RREQ origin " << src << " destination " << dst);
        return;
    }

    // please note that, while re-broadcasting, we do not create a rreqHeader again
    // rather we send the header that is present now at this method
    // after performing the modifications along the above lines
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag ttl;
        ttl.SetTtl(tag.GetTtl() - 1);
        packet->AddPacketTag(ttl);
        packet->AddHeader(rreqHeader);
        TypeHeader tHeader(AODVTYPE_RREQ);
        packet->AddHeader(tHeader);
        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }
        m_lastBcastTime = Simulator::Now();
        Simulator::Schedule(Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10))),
                            &RoutingProtocol::SendTo,
                            this,
                            socket,
                            packet,
                            destination);
    }
}

// instead of this function we will have a new function for SendRevRequest
// that will essentially broadcast a packet with Rev_rreq header
// note that the src and destination actually stay the same for both
// that means, also in the case of RREP, the dst remains the dst of the RREQ
void
RoutingProtocol::SendReply(const RreqHeader& rreqHeader, const RoutingTableEntry& toOrigin)
{
    NS_LOG_FUNCTION(this << toOrigin.GetDestination());
    /*
     * Destination node MUST increment its own sequence number by one if the sequence number in the
     * RREQ packet is equal to that incremented value. Otherwise, the destination does not change
     * its sequence number before generating the  RREP message.
     */
    if (!rreqHeader.GetUnknownSeqno() && (rreqHeader.GetDstSeqno() == m_seqNo + 1))
    {
        m_seqNo++;
    }
    RrepHeader rrepHeader(/*prefixSize=*/0,
                          /*hopCount=*/0,
                          /*dst=*/rreqHeader.GetDst(),
                          /*dstSeqNo=*/m_seqNo,
                          /*origin=*/toOrigin.GetDestination(),
                          /*lifetime=*/m_myRouteTimeout);
    Ptr<Packet> packet = Create<Packet>();
    SocketIpTtlTag tag;
    tag.SetTtl(toOrigin.GetHop());
    packet->AddPacketTag(tag);
    packet->AddHeader(rrepHeader);
    TypeHeader tHeader(AODVTYPE_RREP);
    packet->AddHeader(tHeader);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
    NS_ASSERT(socket);
    socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), RAODV_PORT));
}


// ANIK-NS3-OFFLINE-1
// We need to send a REV_RREQ header containing packet from the destination end
// when the RREQ packet reaches destination
// There we write a new function for sending RevRequest just like sending Request
// Just like SendRequest() braodcasts RREQ, this function broadcasts REV_RREQ message
// Note that, in this function, we have two important changes
// First, we do not introduce any rate limit
// Rate limiting here does not really mean totally discarding the sending
// It just means delaying
// However, doing this retrying both at the sender and receiver end does not make sense
// it would rather introduce further congestion
// so we intentionally skip the rate limiting and retrying functionality
void
RoutingProtocol::SendRevRequest(const RreqHeader& rreqHeader, const RoutingTableEntry& toOrigin)
{
    NS_LOG_FUNCTION(this << toOrigin.GetDestination());
    /*
     * Destination node MUST increment its own sequence number by one if the sequence number in the
     * RREQ packet is equal to that incremented value. Otherwise, the destination does not change
     * its sequence number before generating the REV_RREQ message.
     */
    // this m_seqNo is then used as dstSeqNo
    if (!rreqHeader.GetUnknownSeqno() && (rreqHeader.GetDstSeqno() == m_seqNo + 1))
    {
        m_seqNo++;
    }

    // since it is a braodcast message keeping the lifetime high
    uint32_t ttl = m_netDiameter;

    // building the revRreqHeader
    // by setting appropriate fields
    // we are not setting the hop,
    // so, the initial hop will be 0
    RevRreqHeader revRreqHeader;
    revRreqHeader.SetDst(rreqHeader.GetDst());
    revRreqHeader.SetDstSeqno(m_seqNo);
    
    // as we will now broadcast
    // so the origin should be set dynamically
    // based on the particular interface we will be broadcasting from
    // so the following line is wrong
    // revRreqHeader.SetOrigin(toOrigin.GetDestination());

    // revRreqHeader.SetLifeTime(ttl);
    revRreqHeader.SetLifeTime(m_myRouteTimeout);

    // please note that, 
    // in case of RREP message, there was no ID
    // But here we are again broadcasting just like we did in case of 
    // RREQ
    // thus, in REV_RREQ, we will again need a broadcast ID 
    m_revRequestId++;
    revRreqHeader.SetId(m_revRequestId);

    // if (m_gratuitousReply)
    // {
    //     rreqHeader.SetGratuitousRrep(true);
    // }
    if (m_destinationOnly)
    {
        revRreqHeader.SetDestinationOnly(true);
    }

    // Broadcast REV-RREQ
    // this block is just like that in SendRequest() method
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;

        // setting the origin for this interface, as we are broadcasting
        // revRreqHeader.SetOrigin(iface.GetLocal());
        revRreqHeader.SetOrigin(rreqHeader.GetOrigin());

        // just like in RREQ, we set this in our IdCcahe
        m_revRreqIdCache.IsDuplicate(rreqHeader.GetOrigin(), m_revRequestId);

        // build the packet by adding tag and header
        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag tag;
        tag.SetTtl(ttl);
        packet->AddPacketTag(tag);
        packet->AddHeader(revRreqHeader);
        TypeHeader tHeader(AODVTYPE_REV_RREQ);
        packet->AddHeader(tHeader);

        // BROADCAST
        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }

        Simulator::Schedule(Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10))),
                          &RoutingProtocol::SendTo,
                          this,
                          socket,
                          packet,
                          destination);
    }
    // There should be no retry timer here
    // unlike the case in SendRequest
    // actually there it was mandatory to find a path sooner or later
    // so we had been retrying
    // but here, if we add a retry timer again, this will be illogical
    // too many packets will flood the network unnecessarily
    // so i remove the retry timer
}




// we will not actually need anything like this for the original RAODV probably
// as per the paper, REV_RREQ message is only replied from the actual destination
// not any intermediate node...
// still keeping this method to implement hybrid variants
void
RoutingProtocol::SendReplyByIntermediateNode(RoutingTableEntry& toDst,
                                             RoutingTableEntry& toOrigin,
                                             bool gratRep)
{
    NS_LOG_FUNCTION(this);
    RrepHeader rrepHeader(/*prefixSize=*/0,
                          /*hopCount=*/toDst.GetHop(),
                          /*dst=*/toDst.GetDestination(),
                          /*dstSeqNo=*/toDst.GetSeqNo(),
                          /*origin=*/toOrigin.GetDestination(),
                          /*lifetime=*/toDst.GetLifeTime());
    /* If the node we received a RREQ for is a neighbor we are
     * probably facing a unidirectional link... Better request a RREP-ack
     */
    if (toDst.GetHop() == 1)
    {
        rrepHeader.SetAckRequired(true);
        RoutingTableEntry toNextHop;
        m_routingTable.LookupRoute(toOrigin.GetNextHop(), toNextHop);
        toNextHop.m_ackTimer.SetFunction(&RoutingProtocol::AckTimerExpire, this);
        toNextHop.m_ackTimer.SetArguments(toNextHop.GetDestination(), m_blackListTimeout);
        toNextHop.m_ackTimer.SetDelay(m_nextHopWait);
    }
    toDst.InsertPrecursor(toOrigin.GetNextHop());
    toOrigin.InsertPrecursor(toDst.GetNextHop());
    m_routingTable.Update(toDst);
    m_routingTable.Update(toOrigin);

    Ptr<Packet> packet = Create<Packet>();
    SocketIpTtlTag tag;
    tag.SetTtl(toOrigin.GetHop());
    packet->AddPacketTag(tag);
    packet->AddHeader(rrepHeader);
    TypeHeader tHeader(AODVTYPE_RREP);
    packet->AddHeader(tHeader);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
    NS_ASSERT(socket);
    socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), RAODV_PORT));

    // Generating gratuitous RREPs
    if (gratRep)
    {
        RrepHeader gratRepHeader(/*prefixSize=*/0,
                                 /*hopCount=*/toOrigin.GetHop(),
                                 /*dst=*/toOrigin.GetDestination(),
                                 /*dstSeqNo=*/toOrigin.GetSeqNo(),
                                 /*origin=*/toDst.GetDestination(),
                                 /*lifetime=*/toOrigin.GetLifeTime());
        Ptr<Packet> packetToDst = Create<Packet>();
        SocketIpTtlTag gratTag;
        gratTag.SetTtl(toDst.GetHop());
        packetToDst->AddPacketTag(gratTag);
        packetToDst->AddHeader(gratRepHeader);
        TypeHeader type(AODVTYPE_RREP);
        packetToDst->AddHeader(type);
        Ptr<Socket> socket = FindSocketWithInterfaceAddress(toDst.GetInterface());
        NS_ASSERT(socket);
        NS_LOG_LOGIC("Send gratuitous RREP " << packet->GetUid());
        socket->SendTo(packetToDst, 0, InetSocketAddress(toDst.GetNextHop(), RAODV_PORT));
    }
}

void
RoutingProtocol::SendReplyAck(Ipv4Address neighbor)
{
    NS_LOG_FUNCTION(this << " to " << neighbor);
    RrepAckHeader h;
    TypeHeader typeHeader(AODVTYPE_RREP_ACK);
    Ptr<Packet> packet = Create<Packet>();
    SocketIpTtlTag tag;
    tag.SetTtl(1);
    packet->AddPacketTag(tag);
    packet->AddHeader(h);
    packet->AddHeader(typeHeader);
    RoutingTableEntry toNeighbor;
    m_routingTable.LookupRoute(neighbor, toNeighbor);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(toNeighbor.GetInterface());
    NS_ASSERT(socket);
    socket->SendTo(packet, 0, InetSocketAddress(neighbor, RAODV_PORT));
}

// Receives the RREP packet
// increments hop count
// and it sets a rt entry for the dst as destination
// then check whether i am the origin, that means, whether the unicast was towards me
// if yes, then i will do two important things
// i scheduled a timer for retrying, those are being canceled etc
// and also update the route table entry and sendPacketsFromQueue
// and if i am not the origin, then i first update precursors of 4 rt entries
// and then create a new RREP type packet and unicast it again
// in our case, we will actually have a new RecvRevRequest method
// that will operate on REV_RREQ
// and it will do similar if it itself is the origin, like sending packets from queue etc
// but it is not the origin
// then it will need to again braodcast a new REV_RREQ
// just like we did in SendRequest
void
RoutingProtocol::RecvReply(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
    NS_LOG_FUNCTION(this << " src " << sender);
    RrepHeader rrepHeader;
    p->RemoveHeader(rrepHeader);
    Ipv4Address dst = rrepHeader.GetDst();
    NS_LOG_LOGIC("RREP destination " << dst << " RREP origin " << rrepHeader.GetOrigin());

    uint8_t hop = rrepHeader.GetHopCount() + 1;
    rrepHeader.SetHopCount(hop);

    // If RREP is Hello message
    if (dst == rrepHeader.GetOrigin())
    {
        ProcessHello(rrepHeader, receiver);
        return;
    }

    /*
     * If the route table entry to the destination is created or updated, then the following actions
     * occur:
     * -  the route is marked as active,
     * -  the destination sequence number is marked as valid,
     * -  the next hop in the route entry is assigned to be the node from which the RREP is
     * received, which is indicated by the source IP address field in the IP header,
     * -  the hop count is set to the value of the hop count from RREP message + 1
     * -  the expiry time is set to the current time plus the value of the Lifetime in the RREP
     * message,
     * -  and the destination sequence number is the Destination Sequence Number in the RREP
     * message.
     */
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
    RoutingTableEntry newEntry(
        /*dev=*/dev,
        /*dst=*/dst,
        /*vSeqNo=*/true,
        /*seqNo=*/rrepHeader.GetDstSeqno(),
        /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
        /*hops=*/hop,
        /*nextHop=*/sender,
        /*lifetime=*/rrepHeader.GetLifeTime());
    RoutingTableEntry toDst;
    if (m_routingTable.LookupRoute(dst, toDst))
    {
        // The existing entry is updated only in the following circumstances:
        if (
            // (i) the sequence number in the routing table is marked as invalid in route table
            // entry.
            (!toDst.GetValidSeqNo()) ||

            // (ii) the Destination Sequence Number in the RREP is greater than the node's copy of
            // the destination sequence number and the known value is valid,
            ((int32_t(rrepHeader.GetDstSeqno()) - int32_t(toDst.GetSeqNo())) > 0) ||

            // (iii) the sequence numbers are the same, but the route is marked as inactive.
            (rrepHeader.GetDstSeqno() == toDst.GetSeqNo() && toDst.GetFlag() != VALID) ||

            // (iv) the sequence numbers are the same, and the New Hop Count is smaller than the
            // hop count in route table entry.
            (rrepHeader.GetDstSeqno() == toDst.GetSeqNo() && hop < toDst.GetHop()))
        {
            m_routingTable.Update(newEntry);
        }
    }
    else
    {
        // The forward route for this destination is created if it does not already exist.
        NS_LOG_LOGIC("add new route");
        m_routingTable.AddRoute(newEntry);
    }
    // Acknowledge receipt of the RREP by sending a RREP-ACK message back
    if (rrepHeader.GetAckRequired())
    {
        SendReplyAck(sender);
        rrepHeader.SetAckRequired(false);
    }
    NS_LOG_LOGIC("receiver " << receiver << " origin " << rrepHeader.GetOrigin());
    if (IsMyOwnAddress(rrepHeader.GetOrigin()))
    {
        LOG_UNCOND_MAGENTA("Origin received RREP packet!");
        if (toDst.GetFlag() == IN_SEARCH)
        {
            m_routingTable.Update(newEntry);
            m_addressReqTimer[dst].Cancel();
            m_addressReqTimer.erase(dst);
        }
        m_routingTable.LookupRoute(dst, toDst);
        SendPacketFromQueue(dst, toDst.GetRoute());
        return;
    }

    RoutingTableEntry toOrigin;
    if (!m_routingTable.LookupRoute(rrepHeader.GetOrigin(), toOrigin) ||
        toOrigin.GetFlag() == IN_SEARCH)
    {
        return; // Impossible! drop.
    }
    toOrigin.SetLifeTime(std::max(m_activeRouteTimeout, toOrigin.GetLifeTime()));
    m_routingTable.Update(toOrigin);

    // Update information about precursors
    if (m_routingTable.LookupValidRoute(rrepHeader.GetDst(), toDst))
    {
        toDst.InsertPrecursor(toOrigin.GetNextHop());
        m_routingTable.Update(toDst);

        RoutingTableEntry toNextHopToDst;
        m_routingTable.LookupRoute(toDst.GetNextHop(), toNextHopToDst);
        toNextHopToDst.InsertPrecursor(toOrigin.GetNextHop());
        m_routingTable.Update(toNextHopToDst);

        toOrigin.InsertPrecursor(toDst.GetNextHop());
        m_routingTable.Update(toOrigin);

        RoutingTableEntry toNextHopToOrigin;
        m_routingTable.LookupRoute(toOrigin.GetNextHop(), toNextHopToOrigin);
        toNextHopToOrigin.InsertPrecursor(toDst.GetNextHop());
        m_routingTable.Update(toNextHopToOrigin);
    }
    SocketIpTtlTag tag;
    p->RemovePacketTag(tag);
    if (tag.GetTtl() < 2)
    {
        NS_LOG_DEBUG("TTL exceeded. Drop RREP destination " << dst << " origin "
                                                            << rrepHeader.GetOrigin());
        return;
    }

    Ptr<Packet> packet = Create<Packet>();
    SocketIpTtlTag ttl;
    ttl.SetTtl(tag.GetTtl() - 1);
    packet->AddPacketTag(ttl);
    packet->AddHeader(rrepHeader);
    TypeHeader tHeader(AODVTYPE_RREP);
    packet->AddHeader(tHeader);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
    NS_ASSERT(socket);
    socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), RAODV_PORT));
}



// ANIK-NS3-OFFLINE-1
// note that, in this function, we omit the precursor settings
// beacuse we are now broadcasting, intermediate nodes do not necessarily have a route
// to the origin
// thus setting precursors does not make sense in this case
void
RoutingProtocol::RecvRevRequest(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
    LOG_UNCOND_GREEN("Receiving Rev Request. So RAODV is working!");
    NS_LOG_FUNCTION(this << " src " << sender);

    RevRreqHeader revRreqHeader;
    p->RemoveHeader(revRreqHeader);

    bool isDuplicate = false, isMeOrigin = false;

    // check two things, whether this is a duplicate braodcast REV_RREQ
    // and whether I am the destination myself
    // these booleans will be used later to decide what to do
    if (m_revRreqIdCache.IsDuplicate(revRreqHeader.GetOrigin(), revRreqHeader.GetId()))
    {
        isDuplicate = true;
    }
    if (IsMyOwnAddress(revRreqHeader.GetOrigin())){
        isMeOrigin = true;
    }

    if(isDuplicate && (!isMeOrigin)){
        NS_LOG_DEBUG("Ignoring REV_RREQ due to duplicate and I am not the origin, so just drop!");
        return;
    }

    // Check for duplicates
    // but should not drop right now
    // if (m_revRreqIdCache.IsDuplicate(revRreqHeader.GetOrigin(), revRreqHeader.GetId())) {
    //     return; 
    // }

    Ipv4Address dst = revRreqHeader.GetDst();

    // Update hop count
    uint8_t hop = revRreqHeader.GetHopCount() + 1;
    revRreqHeader.SetHopCount(hop);

    // for the intermediate node, update the route to the dst
    // just like it was done while receiving reply
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
    RoutingTableEntry newEntry(
        /*dev=*/dev,
        /*dst=*/dst,
        /*vSeqNo=*/true,
        /*seqNo=*/revRreqHeader.GetDstSeqno(),
        /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
        /*hops=*/hop,
        /*nextHop=*/sender,
        /*lifetime=*/revRreqHeader.GetLifeTime());

    RoutingTableEntry toDst;
    if (m_routingTable.LookupRoute(dst, toDst)) {

        // The existing entry is updated only in the following circumstances:
        if (
            // (i) the sequence number in the routing table is marked as invalid in route table
            // entry.
            (!toDst.GetValidSeqNo()) ||

            // (ii) the Destination Sequence Number in the RREP is greater than the node's copy of
            // the destination sequence number and the known value is valid,
            ((int32_t(revRreqHeader.GetDstSeqno()) - int32_t(toDst.GetSeqNo())) > 0) ||

            // (iii) the sequence numbers are the same, but the route is marked as inactive.
            (revRreqHeader.GetDstSeqno() == toDst.GetSeqNo() && toDst.GetFlag() != VALID) ||

            // (iv) the sequence numbers are the same, and the New Hop Count is smaller than the
            // hop count in route table entry.
            (revRreqHeader.GetDstSeqno() == toDst.GetSeqNo() && hop < toDst.GetHop()))
        {
            newEntry.SetLifeTime(std::max(revRreqHeader.GetLifeTime(), toDst.GetLifeTime()));
            m_routingTable.Update(newEntry);
        }

    } else {
        // The forward route for this destination is created if it does not already exist.
        NS_LOG_LOGIC("add new route");
        m_routingTable.AddRoute(newEntry);
    }

    // If I am the original source
    if (isMeOrigin) {
        // If this is first REV-RREQ received
        if (!isDuplicate) {
            LOG_UNCOND_YELLOW("Attention! First REV_RREQ received by origin. Now it will send packets in Queue.");
            if (toDst.GetFlag() == IN_SEARCH && (m_addressReqTimer.find(revRreqHeader.GetOrigin()) != m_addressReqTimer.end())) {
                m_routingTable.Update(newEntry);
                m_addressReqTimer[dst].Cancel();
                m_addressReqTimer.erase(dst);
            }
            m_routingTable.LookupRoute(dst, toDst);
            SendPacketFromQueue(dst, toDst.GetRoute());
            return;
        } else {
            // #TODO
            // Store as alternative route
            // m_alternativeRoutes.push_back(toDst);
        }
        return;
    }

    // Otherwise, rebroadcast REV-RREQ
    // please note that, while re-broadcasting, we do not create a revRreqHeader again
    // rather we send the header that is present now at this method
    // after performing the modifications along the above lines
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j) {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;

        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag tag;
        tag.SetTtl(m_netDiameter - hop);
        packet->AddPacketTag(tag);
        packet->AddHeader(revRreqHeader);
        TypeHeader tHeader(AODVTYPE_REV_RREQ);
        packet->AddHeader(tHeader);

        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }

        // #CONF
        // m_lastBcastTime = Simulator::Now();

        Simulator::Schedule(Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10))),
                          &RoutingProtocol::SendTo,
                          this,
                          socket,
                          packet,
                          destination);
    }
}



void
RoutingProtocol::RecvReplyAck(Ipv4Address neighbor)
{
    NS_LOG_FUNCTION(this);
    RoutingTableEntry rt;
    if (m_routingTable.LookupRoute(neighbor, rt))
    {
        rt.m_ackTimer.Cancel();
        rt.SetFlag(VALID);
        m_routingTable.Update(rt);
    }
}

void
RoutingProtocol::ProcessHello(const RrepHeader& rrepHeader, Ipv4Address receiver)
{
    NS_LOG_FUNCTION(this << "from " << rrepHeader.GetDst());
    /*
     *  Whenever a node receives a Hello message from a neighbor, the node
     * SHOULD make sure that it has an active route to the neighbor, and
     * create one if necessary.
     */
    RoutingTableEntry toNeighbor;
    if (!m_routingTable.LookupRoute(rrepHeader.GetDst(), toNeighbor))
    {
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
        RoutingTableEntry newEntry(
            /*dev=*/dev,
            /*dst=*/rrepHeader.GetDst(),
            /*vSeqNo=*/true,
            /*seqNo=*/rrepHeader.GetDstSeqno(),
            /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
            /*hops=*/1,
            /*nextHop=*/rrepHeader.GetDst(),
            /*lifetime=*/rrepHeader.GetLifeTime());
        m_routingTable.AddRoute(newEntry);
    }
    else
    {
        toNeighbor.SetLifeTime(
            std::max(Time(m_allowedHelloLoss * m_helloInterval), toNeighbor.GetLifeTime()));
        toNeighbor.SetSeqNo(rrepHeader.GetDstSeqno());
        toNeighbor.SetValidSeqNo(true);
        toNeighbor.SetFlag(VALID);
        toNeighbor.SetOutputDevice(m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver)));
        toNeighbor.SetInterface(m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0));
        toNeighbor.SetHop(1);
        toNeighbor.SetNextHop(rrepHeader.GetDst());
        m_routingTable.Update(toNeighbor);
    }
    if (m_enableHello)
    {
        m_nb.Update(rrepHeader.GetDst(), Time(m_allowedHelloLoss * m_helloInterval));
    }
}

// here we are receiving the error messages and 
// recursively forwarding them again
// as appropriate
void
RoutingProtocol::RecvError(Ptr<Packet> p, Ipv4Address src)
{
    NS_LOG_FUNCTION(this << " from " << src);
    RerrHeader rerrHeader;
    p->RemoveHeader(rerrHeader);
    std::map<Ipv4Address, uint32_t> dstWithNextHopSrc;
    std::map<Ipv4Address, uint32_t> unreachable;
    m_routingTable.GetListOfDestinationWithNextHop(src, dstWithNextHopSrc);
    std::pair<Ipv4Address, uint32_t> un;
    while (rerrHeader.RemoveUnDestination(un))
    {
        for (auto i = dstWithNextHopSrc.begin(); i != dstWithNextHopSrc.end(); ++i)
        {
            if (i->first == un.first)
            {
                unreachable.insert(un);
            }
        }
    }

    std::vector<Ipv4Address> precursors;
    for (auto i = unreachable.begin(); i != unreachable.end();)
    {
        if (!rerrHeader.AddUnDestination(i->first, i->second))
        {
            TypeHeader typeHeader(AODVTYPE_RERR);
            Ptr<Packet> packet = Create<Packet>();
            SocketIpTtlTag tag;
            tag.SetTtl(1);
            packet->AddPacketTag(tag);
            packet->AddHeader(rerrHeader);
            packet->AddHeader(typeHeader);
            SendRerrMessage(packet, precursors);
            rerrHeader.Clear();
        }
        else
        {
            RoutingTableEntry toDst;
            m_routingTable.LookupRoute(i->first, toDst);
            toDst.GetPrecursors(precursors);
            ++i;
        }
    }
    if (rerrHeader.GetDestCount() != 0)
    {
        TypeHeader typeHeader(AODVTYPE_RERR);
        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag tag;
        tag.SetTtl(1);
        packet->AddPacketTag(tag);
        packet->AddHeader(rerrHeader);
        packet->AddHeader(typeHeader);
        SendRerrMessage(packet, precursors);
    }
    m_routingTable.InvalidateRoutesWithDst(unreachable);
}

void
RoutingProtocol::RouteRequestTimerExpire(Ipv4Address dst)
{
    NS_LOG_LOGIC(this);
    RoutingTableEntry toDst;
    if (m_routingTable.LookupValidRoute(dst, toDst))
    {
        SendPacketFromQueue(dst, toDst.GetRoute());
        NS_LOG_LOGIC("route to " << dst << " found");
        return;
    }
    /*
     *  If a route discovery has been attempted RreqRetries times at the maximum TTL without
     *  receiving any RREP, all data packets destined for the corresponding destination SHOULD be
     *  dropped from the buffer and a Destination Unreachable message SHOULD be delivered to the
     * application.
     */
    if (toDst.GetRreqCnt() == m_rreqRetries)
    {
        NS_LOG_LOGIC("route discovery to " << dst << " has been attempted RreqRetries ("
                                           << m_rreqRetries << ") times with ttl "
                                           << m_netDiameter);
        m_addressReqTimer.erase(dst);
        m_routingTable.DeleteRoute(dst);
        NS_LOG_DEBUG("Route not found. Drop all packets with dst " << dst);
        m_queue.DropPacketWithDst(dst);
        return;
    }

    if (toDst.GetFlag() == IN_SEARCH)
    {
        NS_LOG_LOGIC("Resend RREQ to " << dst << " previous ttl " << toDst.GetHop());
        SendRequest(dst);
    }
    else
    {
        NS_LOG_DEBUG("Route down. Stop search. Drop packet with destination " << dst);
        m_addressReqTimer.erase(dst);
        m_routingTable.DeleteRoute(dst);
        m_queue.DropPacketWithDst(dst);
    }
}

void
RoutingProtocol::HelloTimerExpire()
{
    NS_LOG_FUNCTION(this);
    Time offset = Time(Seconds(0));
    if (m_lastBcastTime > Time(Seconds(0)))
    {
        offset = Simulator::Now() - m_lastBcastTime;
        NS_LOG_DEBUG("Hello deferred due to last bcast at:" << m_lastBcastTime);
    }
    else
    {
        SendHello();
    }
    m_htimer.Cancel();
    Time diff = m_helloInterval - offset;
    m_htimer.Schedule(std::max(Time(Seconds(0)), diff));
    m_lastBcastTime = Time(Seconds(0));
}

void
RoutingProtocol::RreqRateLimitTimerExpire()
{
    NS_LOG_FUNCTION(this);
    m_rreqCount = 0;
    m_rreqRateLimitTimer.Schedule(Seconds(1));
}

void
RoutingProtocol::RerrRateLimitTimerExpire()
{
    NS_LOG_FUNCTION(this);
    m_rerrCount = 0;
    m_rerrRateLimitTimer.Schedule(Seconds(1));
}

void
RoutingProtocol::AckTimerExpire(Ipv4Address neighbor, Time blacklistTimeout)
{
    NS_LOG_FUNCTION(this);
    m_routingTable.MarkLinkAsUnidirectional(neighbor, blacklistTimeout);
}

void
RoutingProtocol::SendHello()
{
    NS_LOG_FUNCTION(this);
    /* Broadcast a RREP with TTL = 1 with the RREP message fields set as follows:
     *   Destination IP Address         The node's IP address.
     *   Destination Sequence Number    The node's latest sequence number.
     *   Hop Count                      0
     *   Lifetime                       AllowedHelloLoss * HelloInterval
     */
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        RrepHeader helloHeader(/*prefixSize=*/0,
                               /*hopCount=*/0,
                               /*dst=*/iface.GetLocal(),
                               /*dstSeqNo=*/m_seqNo,
                               /*origin=*/iface.GetLocal(),
                               /*lifetime=*/Time(m_allowedHelloLoss * m_helloInterval));
        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag tag;
        tag.SetTtl(1);
        packet->AddPacketTag(tag);
        packet->AddHeader(helloHeader);
        TypeHeader tHeader(AODVTYPE_RREP);
        packet->AddHeader(tHeader);
        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }
        Time jitter = Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10)));
        Simulator::Schedule(jitter, &RoutingProtocol::SendTo, this, socket, packet, destination);
    }
}

// this function is called when we find a valid route
// now our job is to send all the waiting packets that have that destination
// we pass the dst and the route
// and it finds all those packets and sends them
// by unicast
void
RoutingProtocol::SendPacketFromQueue(Ipv4Address dst, Ptr<Ipv4Route> route)
{
    NS_LOG_FUNCTION(this);
    QueueEntry queueEntry;
    while (m_queue.Dequeue(dst, queueEntry))
    {
        DeferredRouteOutputTag tag;
        Ptr<Packet> p = ConstCast<Packet>(queueEntry.GetPacket());
        if (p->RemovePacketTag(tag) && tag.GetInterface() != -1 &&
            tag.GetInterface() != m_ipv4->GetInterfaceForDevice(route->GetOutputDevice()))
        {
            NS_LOG_DEBUG("Output device doesn't match. Dropped.");
            return;
        }
        UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback();
        Ipv4Header header = queueEntry.GetIpv4Header();
        header.SetSource(route->GetSource());
        header.SetTtl(header.GetTtl() +
                      1); // compensate extra TTL decrement by fake loopback routing
        ucb(route, p, header);
    }
}

// send a Rerr message to every precursor, that means those who depend upon this route
// so, find them and notify them recursively
void
RoutingProtocol::SendRerrWhenBreaksLinkToNextHop(Ipv4Address nextHop)
{
    NS_LOG_FUNCTION(this << nextHop);
    RerrHeader rerrHeader;
    std::vector<Ipv4Address> precursors;
    std::map<Ipv4Address, uint32_t> unreachable;

    RoutingTableEntry toNextHop;
    if (!m_routingTable.LookupRoute(nextHop, toNextHop))
    {
        return;
    }
    toNextHop.GetPrecursors(precursors);
    rerrHeader.AddUnDestination(nextHop, toNextHop.GetSeqNo());
    m_routingTable.GetListOfDestinationWithNextHop(nextHop, unreachable);
    for (auto i = unreachable.begin(); i != unreachable.end();)
    {
        if (!rerrHeader.AddUnDestination(i->first, i->second))
        {
            NS_LOG_LOGIC("Send RERR message with maximum size.");
            TypeHeader typeHeader(AODVTYPE_RERR);
            Ptr<Packet> packet = Create<Packet>();
            SocketIpTtlTag tag;
            tag.SetTtl(1);
            packet->AddPacketTag(tag);
            packet->AddHeader(rerrHeader);
            packet->AddHeader(typeHeader);
            SendRerrMessage(packet, precursors);
            rerrHeader.Clear();
        }
        else
        {
            RoutingTableEntry toDst;
            m_routingTable.LookupRoute(i->first, toDst);
            toDst.GetPrecursors(precursors);
            ++i;
        }
    }
    if (rerrHeader.GetDestCount() != 0)
    {
        TypeHeader typeHeader(AODVTYPE_RERR);
        Ptr<Packet> packet = Create<Packet>();
        SocketIpTtlTag tag;
        tag.SetTtl(1);
        packet->AddPacketTag(tag);
        packet->AddHeader(rerrHeader);
        packet->AddHeader(typeHeader);
        SendRerrMessage(packet, precursors);
    }
    unreachable.insert(std::make_pair(nextHop, toNextHop.GetSeqNo()));
    m_routingTable.InvalidateRoutesWithDst(unreachable);
}

void
RoutingProtocol::SendRerrWhenNoRouteToForward(Ipv4Address dst,
                                              uint32_t dstSeqNo,
                                              Ipv4Address origin)
{
    NS_LOG_FUNCTION(this);
    // A node SHOULD NOT originate more than RERR_RATELIMIT RERR messages per second.
    if (m_rerrCount == m_rerrRateLimit)
    {
        // Just make sure that the RerrRateLimit timer is running and will expire
        NS_ASSERT(m_rerrRateLimitTimer.IsRunning());
        // discard the packet and return
        NS_LOG_LOGIC("RerrRateLimit reached at "
                     << Simulator::Now().As(Time::S) << " with timer delay left "
                     << m_rerrRateLimitTimer.GetDelayLeft().As(Time::S) << "; suppressing RERR");
        return;
    }
    RerrHeader rerrHeader;
    rerrHeader.AddUnDestination(dst, dstSeqNo);
    RoutingTableEntry toOrigin;
    Ptr<Packet> packet = Create<Packet>();
    SocketIpTtlTag tag;
    tag.SetTtl(1);
    packet->AddPacketTag(tag);
    packet->AddHeader(rerrHeader);
    packet->AddHeader(TypeHeader(AODVTYPE_RERR));
    if (m_routingTable.LookupValidRoute(origin, toOrigin))
    {
        Ptr<Socket> socket = FindSocketWithInterfaceAddress(toOrigin.GetInterface());
        NS_ASSERT(socket);
        NS_LOG_LOGIC("Unicast RERR to the source of the data transmission");
        socket->SendTo(packet, 0, InetSocketAddress(toOrigin.GetNextHop(), RAODV_PORT));
    }
    else
    {
        for (auto i = m_socketAddresses.begin(); i != m_socketAddresses.end(); ++i)
        {
            Ptr<Socket> socket = i->first;
            Ipv4InterfaceAddress iface = i->second;
            NS_ASSERT(socket);
            NS_LOG_LOGIC("Broadcast RERR message from interface " << iface.GetLocal());
            // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
            Ipv4Address destination;
            if (iface.GetMask() == Ipv4Mask::GetOnes())
            {
                destination = Ipv4Address("255.255.255.255");
            }
            else
            {
                destination = iface.GetBroadcast();
            }
            socket->SendTo(packet->Copy(), 0, InetSocketAddress(destination, RAODV_PORT));
        }
    }
}

void
RoutingProtocol::SendRerrMessage(Ptr<Packet> packet, std::vector<Ipv4Address> precursors)
{
    NS_LOG_FUNCTION(this);

    if (precursors.empty())
    {
        NS_LOG_LOGIC("No precursors");
        return;
    }
    // A node SHOULD NOT originate more than RERR_RATELIMIT RERR messages per second.
    if (m_rerrCount == m_rerrRateLimit)
    {
        // Just make sure that the RerrRateLimit timer is running and will expire
        NS_ASSERT(m_rerrRateLimitTimer.IsRunning());
        // discard the packet and return
        NS_LOG_LOGIC("RerrRateLimit reached at "
                     << Simulator::Now().As(Time::S) << " with timer delay left "
                     << m_rerrRateLimitTimer.GetDelayLeft().As(Time::S) << "; suppressing RERR");
        return;
    }
    // If there is only one precursor, RERR SHOULD be unicast toward that precursor
    if (precursors.size() == 1)
    {
        RoutingTableEntry toPrecursor;
        if (m_routingTable.LookupValidRoute(precursors.front(), toPrecursor))
        {
            Ptr<Socket> socket = FindSocketWithInterfaceAddress(toPrecursor.GetInterface());
            NS_ASSERT(socket);
            NS_LOG_LOGIC("one precursor => unicast RERR to "
                         << toPrecursor.GetDestination() << " from "
                         << toPrecursor.GetInterface().GetLocal());
            Simulator::Schedule(Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10))),
                                &RoutingProtocol::SendTo,
                                this,
                                socket,
                                packet,
                                precursors.front());
            m_rerrCount++;
        }
        return;
    }

    //  Should only transmit RERR on those interfaces which have precursor nodes for the broken
    //  route
    std::vector<Ipv4InterfaceAddress> ifaces;
    RoutingTableEntry toPrecursor;
    for (auto i = precursors.begin(); i != precursors.end(); ++i)
    {
        if (m_routingTable.LookupValidRoute(*i, toPrecursor) &&
            std::find(ifaces.begin(), ifaces.end(), toPrecursor.GetInterface()) == ifaces.end())
        {
            ifaces.push_back(toPrecursor.GetInterface());
        }
    }

    for (auto i = ifaces.begin(); i != ifaces.end(); ++i)
    {
        Ptr<Socket> socket = FindSocketWithInterfaceAddress(*i);
        NS_ASSERT(socket);
        NS_LOG_LOGIC("Broadcast RERR message from interface " << i->GetLocal());
        // std::cout << "Broadcast RERR message from interface " << i->GetLocal () << std::endl;
        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ptr<Packet> p = packet->Copy();
        Ipv4Address destination;
        if (i->GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = i->GetBroadcast();
        }
        Simulator::Schedule(Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 10))),
                            &RoutingProtocol::SendTo,
                            this,
                            socket,
                            p,
                            destination);
    }
}

Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const
{
    NS_LOG_FUNCTION(this << addr);
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        if (iface == addr)
        {
            return socket;
        }
    }
    Ptr<Socket> socket;
    return socket;
}

Ptr<Socket>
RoutingProtocol::FindSubnetBroadcastSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const
{
    NS_LOG_FUNCTION(this << addr);
    for (auto j = m_socketSubnetBroadcastAddresses.begin();
         j != m_socketSubnetBroadcastAddresses.end();
         ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        if (iface == addr)
        {
            return socket;
        }
    }
    Ptr<Socket> socket;
    return socket;
}

void
RoutingProtocol::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    uint32_t startTime;
    if (m_enableHello)
    {
        m_htimer.SetFunction(&RoutingProtocol::HelloTimerExpire, this);
        startTime = m_uniformRandomVariable->GetInteger(0, 100);
        NS_LOG_DEBUG("Starting at time " << startTime << "ms");
        m_htimer.Schedule(MilliSeconds(startTime));
    }
    Ipv4RoutingProtocol::DoInitialize();
}

} // namespace raodv
} // namespace ns3
