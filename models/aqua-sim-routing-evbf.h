/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Vector Based Forwarding (EVBF) for AquaSim-NG
 *
 * This protocol extends the VBF algorithm by modifying the "desirableness factor" (alpha)
 * based on the node's residual energy. In VBF, alpha determines the holding time;
 * lower alpha means shorter delay. EVBF reduces alpha for nodes with high energy,
 * making them more likely to forward packets.
 *
 * Formula Used (Linear Penalty):
 *    α' = α * [1 + β * (1 - Eres/Einit)]
 *    - Increases alpha (delay) linearly as energy depletes.
 *
 * Final Delay Calculation:
 *    Th = √α' * Tdelay + (R - d) / V0
 *
 * Member Variables:
 * - m_beta: Energy bias factor (default: 0.5)
 * - m_Tdelay: Maximum delay constant (default: 0.5s)
 * - m_epsilon: Division safeguard (default: 0.01)
 *
 * Author: Research Implementation
 */

#ifndef AQUA_SIM_ROUTING_EVBF_H
#define AQUA_SIM_ROUTING_EVBF_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"

#include <map>

namespace ns3 {

class VBHeader;

// Neighborhood structure for EVBF hash table
struct evbf_neighborhood {
    int number;
    Vector neighbor[MAX_NEIGHBOR];
};

typedef std::pair<AquaSimAddress, unsigned int> evbf_hash_entry;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Packet Hash table for EVBF to assist in specialized tables.
 */
class EVBFPktHashTable {
public:
    std::map<evbf_hash_entry, evbf_neighborhood*> m_htable;

    EVBFPktHashTable();
    ~EVBFPktHashTable();

    int m_windowSize;
    void Reset();
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum);
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p);
    evbf_neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
};

/**
 * \brief Data hash table for EVBF
 */
class EVBFDataHashTable {
public:
    std::map<int*, int*> m_htable;

    EVBFDataHashTable();
    ~EVBFDataHashTable();

    void Reset();
    void PutInHash(int *attr);
    int* GetHash(int *attr);
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Energy-Aware Vector Based Forwarding (EVBF)
 * 
 * Extends VBF by incorporating residual energy into desirableness
 * calculation. Higher-energy nodes forward packets earlier.
 */
class AquaSimEVBF : public AquaSimRouting {
public:
    AquaSimEVBF();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void SetTargetPos(Vector pos);

protected:
    int m_pkCount;
    int m_counter;
    int m_hopByHop;
    int m_enableRouting;
    double m_priority;
    bool m_measureStatus;
    
    EVBFPktHashTable PktTable;
    EVBFPktHashTable SourceTable;
    EVBFPktHashTable Target_discoveryTable;
    EVBFPktHashTable SinkTable;

    double m_width;
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    // ========== EVBF Energy-Aware Parameters ==========
    double m_beta;          // Energy bias factor (default: 0.5)
    double m_Tdelay;        // Max delay constant (default: 0.5s)
    double m_epsilon;       // Division safeguard (default: 0.01)


    // Core functions
    void Terminate();
    void Reset();
    void ConsiderNew(Ptr<Packet> pkt);
    void SetDelayTimer(Ptr<Packet>, double);
    void Timeout(Ptr<Packet>);
    double Advance(Ptr<Packet>);
    double Distance(Ptr<Packet>);
    double Projection(Ptr<Packet>);
    double CalculateDelay(Ptr<Packet>, Vector*);
    
    // Energy-aware delay calculation
    double CalculateEnergyAwareDelay(Ptr<Packet> pkt, Vector* p1);
    
    void CalculatePosition(Ptr<Packet>);
    void SetMeasureTimer(Ptr<Packet>, double);
    bool IsTarget(Ptr<Packet>);
    bool IsCloseEnough(Ptr<Packet>);

    Ptr<Packet> CreatePacket();
    Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);

    void DataForSink(Ptr<Packet> pkt);
    void StopSource();
    void MACprepare(Ptr<Packet> pkt);
    void MACsend(Ptr<Packet> pkt, double delay = 0);

    virtual void DoDispose();
};

}  // namespace ns3

#endif /* AQUA_SIM_ROUTING_EVBF_H */
