/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Siddharth Gangadhar <siddharth@ittc.ku.edu>, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>,
 * and Greeshma Umapathi
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

#ifndef TCP_STOCHASTICLOGWESTWOOD_H
#define TCP_STOCHASTICLOGWESTWOOD_H

#include "tcp-congestion-ops.h"
#include "tcp-recovery-ops.h"

#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/traced-value.h"

namespace ns3
{

class Time;

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP StochasticLogWestwood+.
 *
 * StochasticLogWestwood+ employ the AIAD (Additive Increase/Adaptive Decrease)
 * congestion control paradigm. When a congestion episode happens,
 * instead of halving the cwnd, these protocols try to estimate the network's
 * bandwidth and use the estimated value to adjust the cwnd.
 * While StochasticLogWestwood performs the bandwidth sampling every ACK reception,
 * StochasticLogWestwood+ samples the bandwidth every RTT.
 *
 * The two main methods in the implementation are the CountAck (const TCPHeader&)
 * and the EstimateBW (int, const, Time). The CountAck method calculates
 * the number of acknowledged segments on the receipt of an ACK.
 * The EstimateBW estimates the bandwidth based on the value returned by CountAck
 * and the sampling interval (last RTT).
 *
 * WARNING: this TCP model lacks validation and regression tests; use with caution.
 */
class TcpStochasticLogWestwoodPlus : public TcpNewReno
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    TcpStochasticLogWestwoodPlus();
    /**
     * \brief Copy constructor
     * \param sock the object to copy
     */
    TcpStochasticLogWestwoodPlus(const TcpStochasticLogWestwoodPlus& sock);
    ~TcpStochasticLogWestwoodPlus() override;

    /**
     * \brief Filter type (None or Tustin)
     */
    enum FilterType
    {
        NONE,
        TUSTIN
    };

    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t packetsAcked, const Time& rtt) override;

    Ptr<TcpCongestionOps> Fork() override;

    // ANIK-NS3-OFFLINE-2
    void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

  private:
    /**
     * Update the total number of acknowledged packets during the current RTT
     *
     * \param [in] acked the number of packets the currently received ACK acknowledges
     */
    void UpdateAckedSegments(int acked);

    /**
     * Estimate the network's bandwidth
     *
     * \param [in] rtt the RTT estimation.
     * \param [in] tcb the socket state.
     */
    void EstimateBW(const Time& rtt, Ptr<TcpSocketState> tcb);

  protected:
    TracedValue<DataRate> m_currentBW; //!< Current value of the estimated BW
    DataRate m_lastSampleBW;           //!< Last bandwidth sample
    DataRate m_lastBW;                 //!< Last bandwidth sample after being filtered
    FilterType m_fType;                //!< 0 for none, 1 for Tustin

    uint32_t m_ackedSegments;  //!< The number of segments ACKed between RTTs
    bool m_IsCount;            //!< Start keeping track of m_ackedSegments for StochasticLogWestwood+ if TRUE
    EventId m_bwEstimateEvent; //!< The BW estimation event for StochasticLogWestwood+
    Time m_lastAck;            //!< The last ACK time

    // ANIK-NS3-OFFLINE-2
    uint32_t m_lastMaxCwnd; // last maxCwnd size in segments, NOT bytes

    // ANIK-NS3-OFFLINE-2
    double m_alpha;
};

} // namespace ns3

#endif /* TCP_STOCHASTICLOGWESTWOOD_H */
