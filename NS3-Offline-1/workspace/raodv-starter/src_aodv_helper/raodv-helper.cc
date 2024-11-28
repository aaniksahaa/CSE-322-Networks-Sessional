/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 * Copyright (c) 2024 Sheikh Azizul Hakim
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
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/AODV-UU
 *
 *      A Reverse AODV Routing Protocol in Ad Hoc Mobile Networks
 *      https://dl.ifip.org/db/conf/euc/eucw2006/KimTA06.pdf 
 *
 * Authors (AODV):      Elena Buchatskaia <borovkovaes@iitp.ru>
 *                      Pavel Boyko <boyko@iitp.ru>
 * Authors (R-AODV):    Sheikh Azizul Hakim <hakim@cse.buet.ac.bd>
 */
 
#include "raodv-helper.h"
#include "ns3/raodv-routing-protocol.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3 {

    RAodvHelper::RAodvHelper() :
            Ipv4RoutingHelper() {
        m_agentFactory.SetTypeId("ns3::raodv::RoutingProtocol");
    }

    RAodvHelper *
    RAodvHelper::Copy(void) const {
        return new RAodvHelper(*this);
    }

    Ptr <Ipv4RoutingProtocol>
    RAodvHelper::Create(Ptr <Node> node) const {
        Ptr <raodv::RoutingProtocol> agent = m_agentFactory.Create<raodv::RoutingProtocol>();
        node->AggregateObject(agent);
        return agent;
    }

    void
    RAodvHelper::Set(std::string name, const AttributeValue &value) {
        m_agentFactory.Set(name, value);
    }

    uint32_t RAodvHelper::getSizeOfControlPackets(NodeContainer nodes) {
        Ptr<Node> node = nodes.Get(0);
        Ptr <Ipv4> ipv4 = node->GetObject<Ipv4>();
        NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");
        Ptr <Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
        NS_ASSERT_MSG(proto, "Ipv4 routing not installed on node");
        Ptr <raodv::RoutingProtocol> aodv = DynamicCast<raodv::RoutingProtocol>(proto);
        return aodv->getSizeOfControlPackets();
    }

    int64_t
    RAodvHelper::AssignStreams(NodeContainer c, int64_t stream) {
        int64_t currentStream = stream;
        Ptr <Node> node;
        for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
            node = (*i);
            Ptr <Ipv4> ipv4 = node->GetObject<Ipv4>();
            NS_ASSERT_MSG(ipv4, "Ipv4 not installed on node");
            Ptr <Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol();
            NS_ASSERT_MSG(proto, "Ipv4 routing not installed on node");
            Ptr <raodv::RoutingProtocol> aodv = DynamicCast<raodv::RoutingProtocol>(proto);
            if (aodv) {
                currentStream += aodv->AssignStreams(currentStream);
                continue;
            }
            // RAodv may also be in a list
            Ptr <Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting>(proto);
            if (list) {
                int16_t priority;
                Ptr <Ipv4RoutingProtocol> listProto;
                Ptr <raodv::RoutingProtocol> listRAodv;
                for (uint32_t i = 0; i < list->GetNRoutingProtocols(); i++) {
                    listProto = list->GetRoutingProtocol(i, priority);
                    listRAodv = DynamicCast<raodv::RoutingProtocol>(listProto);
                    if (listRAodv) {
                        currentStream += listRAodv->AssignStreams(currentStream);
                        break;
                    }
                }
            }
        }
        return (currentStream - stream);
    }

}
