/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>,
 *          Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 *          Greeshma Umapathi
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

#include "tcp-stochasticlogwestwood-plus.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/random-variable-stream.h"


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




NS_LOG_COMPONENT_DEFINE("TcpStochasticLogWestwoodPlus");

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TcpStochasticLogWestwoodPlus);

TypeId
TcpStochasticLogWestwoodPlus::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TcpStochasticLogWestwoodPlus")
            .SetParent<TcpNewReno>()
            .SetGroupName("Internet")
            .AddConstructor<TcpStochasticLogWestwoodPlus>()
            .AddAttribute(
                "FilterType",
                "Use this to choose no filter or Tustin's approximation filter",
                EnumValue(TcpStochasticLogWestwoodPlus::TUSTIN),
                MakeEnumAccessor<FilterType>(&TcpStochasticLogWestwoodPlus::m_fType),
                MakeEnumChecker(TcpStochasticLogWestwoodPlus::NONE, "None", TcpStochasticLogWestwoodPlus::TUSTIN, "Tustin"))
            .AddTraceSource("EstimatedBW",
                            "The estimated bandwidth",
                            MakeTraceSourceAccessor(&TcpStochasticLogWestwoodPlus::m_currentBW),
                            "ns3::TracedValueCallback::DataRate");
    return tid;
}

TcpStochasticLogWestwoodPlus::TcpStochasticLogWestwoodPlus()
    : TcpNewReno(),
      m_currentBW(0),
      m_lastSampleBW(0),
      m_lastBW(0),
      m_ackedSegments(0),
      m_IsCount(false),
      m_lastAck(0),
      m_lastMaxCwnd(0)
{
    NS_LOG_FUNCTION(this);

    double cov = 0.25;
    double mean = 4.0;

    double std = mean * cov;
    double variance = std*std;
    double bound = mean / 2;

    Ptr<NormalRandomVariable> x = CreateObject<NormalRandomVariable> ();
    x->SetAttribute ("Mean", DoubleValue (mean));
    x->SetAttribute ("Variance", DoubleValue (variance));
    x->SetAttribute ("Bound", DoubleValue (bound));

    // Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    // x->SetAttribute ("Min", DoubleValue (mean-bound));
    // x->SetAttribute ("Max", DoubleValue (mean+bound));

    m_alpha = x->GetValue ();

    LOG_UNCOND_MAGENTA("Set alpha = " + std::to_string(m_alpha));
}

TcpStochasticLogWestwoodPlus::TcpStochasticLogWestwoodPlus(const TcpStochasticLogWestwoodPlus& sock)
    : TcpNewReno(sock),
      m_currentBW(sock.m_currentBW),
      m_lastSampleBW(sock.m_lastSampleBW),
      m_lastBW(sock.m_lastBW),
      m_fType(sock.m_fType),
      m_IsCount(sock.m_IsCount),
      m_lastMaxCwnd(sock.m_lastMaxCwnd),
      m_alpha(sock.m_alpha)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_LOGIC("Invoked the copy constructor");
}

TcpStochasticLogWestwoodPlus::~TcpStochasticLogWestwoodPlus()
{
}

/**
 * When new packets are acked,
 * we restart the BW estimation process
 */
void
TcpStochasticLogWestwoodPlus::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << packetsAcked << rtt);

    if (rtt.IsZero())
    {
        NS_LOG_WARN("RTT measured is zero!");
        return;
    }

    m_ackedSegments += packetsAcked;

    if (!(rtt.IsZero() || m_IsCount))
    {
        m_IsCount = true;
        m_bwEstimateEvent.Cancel();
        m_bwEstimateEvent = Simulator::Schedule(rtt, &TcpStochasticLogWestwoodPlus::EstimateBW, this, rtt, tcb);
    }
}

/**
 * Here, we estimate the BW
 * first we find the currentSampleBW, this is just as simple as
 * the BW for the last ackedSegments, (total size of last ackedSegments) / RTT
 * The estimation is a weighted moving average
 * newBW = 0.9*(lastBW) + 0.1*(average of (lastSampleBW, currentSampleBW))
 */
void
TcpStochasticLogWestwoodPlus::EstimateBW(const Time& rtt, Ptr<TcpSocketState> tcb)
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(!rtt.IsZero());

    m_currentBW = DataRate(m_ackedSegments * tcb->m_segmentSize * 8.0 / rtt.GetSeconds());
    m_IsCount = false;

    m_ackedSegments = 0;

    NS_LOG_LOGIC("Estimated BW: " << m_currentBW);

    // Filter the BW sample

    constexpr double ALPHA = 0.9;

    if (m_fType == TcpStochasticLogWestwoodPlus::TUSTIN)
    {
        DataRate sample_bwe = m_currentBW;
        m_currentBW = (m_lastBW * ALPHA) + (((sample_bwe + m_lastSampleBW) * 0.5) * (1 - ALPHA));
        m_lastSampleBW = sample_bwe;
        m_lastBW = m_currentBW;
    }

    NS_LOG_LOGIC("Estimated BW after filtering: " << m_currentBW);
}




// ANIK-NS3-OFFLINE-2
/**
 * \brief TcpStochasticLogWestwoodPlus congestion avoidance
 *
 * During congestion avoidance, cwnd is incremented by roughly 1 full-sized
 * segment per round-trip time (RTT).
 *
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 */
void
TcpStochasticLogWestwoodPlus::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    // LOG_UNCOND_YELLOW("At New written function... working!");

    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked > 0)
    {
        double adder = 1.0;

        double adder1 =
            static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();
        adder = std::max(adder, adder1);
        


        uint32_t segCwnd = tcb->GetCwndInSegments();
        double adder2 =
            static_cast<double>((1/m_alpha) * ((m_lastMaxCwnd*1.0 / segCwnd) - 1) * tcb->m_segmentSize);
        adder = std::max(adder, adder2);


        
        tcb->m_cWnd += static_cast<uint32_t>(adder);
        NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh);
    }
}




// ANIK-NS3-OFFLINE-2
/**
 * This is the update rule of the slow-start threshold
 * For WestWoodPlus, it is
 * max(2*seg_size, estimated BW * min_RTT)
 * This is basically an approach towards updating the CW and ssthresh more intelligently
 * than just blindly halving it
 * estimated BW * min_RTT gives an estimate of Bandwidth-Delay-Product(BDP)
 * Bandwidth-Delay-Product basically means, what amount should be sent at a time
 * it is identical to window size in terms of intuition
 */
uint32_t
TcpStochasticLogWestwoodPlus::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight [[maybe_unused]])
{
    // ANIK-NS3-OFFLINE-2
    uint32_t segCwnd = tcb->GetCwndInSegments();
    NS_LOG_INFO("Last max cwnd: " << m_lastMaxCwnd << " updated to " << segCwnd);
    m_lastMaxCwnd = segCwnd;


    uint32_t ssThresh = static_cast<uint32_t>((m_currentBW * tcb->m_minRtt) / 8.0);



    double cov = 0.25;
    double mean = ssThresh;
    
    double std = mean * cov;
    double variance = std*std;
    double bound = mean / 2;

    Ptr<NormalRandomVariable> x = CreateObject<NormalRandomVariable> ();
    x->SetAttribute ("Mean", DoubleValue (mean));
    x->SetAttribute ("Variance", DoubleValue (variance));
    x->SetAttribute ("Bound", DoubleValue (bound));

    // Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    // x->SetAttribute ("Min", DoubleValue (mean-bound));
    // x->SetAttribute ("Max", DoubleValue (mean+bound));

    ssThresh = x->GetInteger ();

    // LOG_UNCOND_MAGENTA("Set alpha = " + std::to_string(m_alpha));

    NS_LOG_LOGIC("CurrentBW: " << m_currentBW << " minRtt: " << tcb->m_minRtt
                               << " ssThresh: " << ssThresh);

    return std::max(2 * tcb->m_segmentSize, ssThresh);
}

Ptr<TcpCongestionOps>
TcpStochasticLogWestwoodPlus::Fork()
{
    return CreateObject<TcpStochasticLogWestwoodPlus>(*this);
}

} // namespace ns3
