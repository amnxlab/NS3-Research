/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Vector Based Forwarding v2 (EVBFv2) for AquaSim-NG
 *
 * This protocol extends EVBF by using an INVERSE SCALING formula for
 * energy-aware delay calculation instead of the Linear Penalty formula.
 *
 * Formula Used (Inverse Scaling):
 *    α' = α / (Eres/Einit + ε)
 *    - Reduces delay inversely proportional to residual energy.
 *    - Higher energy -> Lower α' -> Shorter delay -> Forwards first.
 *
 * Final Delay Calculation:
 *    Th = √α' * Tdelay + (R - d) / V0
 *
 * Member Variables:
 * - m_beta: Kept for compatibility (unused in Inverse Scaling)
 * - m_Tdelay: Maximum delay constant (default: 0.5s)
 * - m_epsilon: Division safeguard (default: 0.01)
 *
 * Author: Research Implementation
 */

#ifndef AQUA_SIM_ROUTING_EVBFV2_H
#define AQUA_SIM_ROUTING_EVBFV2_H

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

// Neighborhood structure for EVBFv2 hash table
struct evbfv2_neighborhood {
    int number;
    Vector neighbor[MAX_NEIGHBOR];
};

typedef std::pair<AquaSimAddress, unsigned int> evbfv2_hash_entry;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Packet Hash table for EVBFv2.
 */
class EVBFv2PktHashTable {
public:
    std::map<evbfv2_hash_entry, evbfv2_neighborhood*> m_htable;

    EVBFv2PktHashTable();
    ~EVBFv2PktHashTable();

    int m_windowSize;
    void Reset();
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum);
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p);
    evbfv2_neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
};

/**
 * \brief Data hash table for EVBFv2
 */
class EVBFv2DataHashTable {
public:
    std::map<int*, int*> m_htable;

    EVBFv2DataHashTable();
    ~EVBFv2DataHashTable();

    void Reset();
    void PutInHash(int *attr);
    int* GetHash(int *attr);
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Energy-Aware Vector Based Forwarding v2 (EVBFv2)
 * 
 * Uses Inverse Scaling formula for energy-aware delay calculation.
 * Higher-energy nodes forward packets earlier.
 */
class AquaSimEVBFv2 : public AquaSimRouting {
public:
    AquaSimEVBFv2();
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
    
    EVBFv2PktHashTable PktTable;
    EVBFv2PktHashTable SourceTable;
    EVBFv2PktHashTable Target_discoveryTable;
    EVBFv2PktHashTable SinkTable;

    double m_width;
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    // ========== EVBFv2 Energy-Aware Parameters ==========
    double m_beta;          // Kept for compatibility
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
    
    // Energy-aware delay calculation (Inverse Scaling)
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

#endif /* AQUA_SIM_ROUTING_EVBFV2_H */
