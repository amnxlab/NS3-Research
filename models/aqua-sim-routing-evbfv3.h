/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Vector Based Forwarding v3 (EVBFv3) for AquaSim-NG
 *
 * This version extends EVBF by adding SNR (Signal-to-Noise Ratio) as
 * a third factor in the holding time calculation, using LINEAR SCALING.
 * It combines:
 *   - Distance to routing vector (closer = better)
 *   - Residual energy (more energy = forward earlier)
 *   - Link quality (higher SNR = forward earlier)
 *
 * Linear Formula:
 *    α' = α * (1 + β*(1 - Eres/Einit)) * (1 + δ*(1 - Qsnr))
 *    Th = √α' * Tdelay + (R - d) / V0
 *
 * Where:
 *    Qsnr = min(1, max(0, SNR/SNRmax))  -- Normalized SNR quality
 *    Higher energy + higher SNR → smaller α' → shorter delay
 *
 * For INVERSE SCALING with SNR, use EVBFv4 instead.
 *
 * Author: Eng. Ahmed Amin
 * Website: https://www.amnxlab.site/
 * Github: https://github.com/amnxlab
 */

#ifndef AQUA_SIM_ROUTING_EVBF_V3_H
#define AQUA_SIM_ROUTING_EVBF_V3_H

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

// Neighborhood structure for EVBFv3 hash table
struct evbfv3_neighborhood {
    int number;
    Vector neighbor[MAX_NEIGHBOR];
};

typedef std::pair<AquaSimAddress, unsigned int> evbfv3_hash_entry;

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Packet Hash table for EVBFv3.
 */
class EVBFv3PktHashTable {
public:
    std::map<evbfv3_hash_entry, evbfv3_neighborhood*> m_htable;

    EVBFv3PktHashTable();
    ~EVBFv3PktHashTable();

    int m_windowSize;
    void Reset();
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum);
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p);
    evbfv3_neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
};

/**
 * \brief Data hash table for EVBFv3
 */
class EVBFv3DataHashTable {
public:
    std::map<int*, int*> m_htable;

    EVBFv3DataHashTable();
    ~EVBFv3DataHashTable();

    void Reset();
    void PutInHash(int *attr);
    int* GetHash(int *attr);
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Energy-Aware Vector Based Forwarding v3 (EVBFv3)
 * 
 * Extends VBF by incorporating BOTH residual energy AND link quality (SNR)
 * into holding time calculation. Nodes with higher energy and better link
 * quality forward packets earlier.
 */
class AquaSimEVBFv3 : public AquaSimRouting {
public:
    AquaSimEVBFv3();
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
    
    EVBFv3PktHashTable PktTable;
    EVBFv3PktHashTable SourceTable;
    EVBFv3PktHashTable Target_discoveryTable;
    EVBFv3PktHashTable SinkTable;

    double m_width;
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    // ========== EVBFv3 Energy+SNR-Aware Parameters (Linear Mode) ==========
    double m_beta;          // Energy bias factor (default: 0.5)
    double m_delta;         // SNR bias factor (default: 0.5)
    double m_Tdelay;        // Max delay constant (default: 0.5s)
    double m_snrMax;        // Maximum SNR for normalization (default: 100.0)
    double m_snrMin;        // Minimum acceptable SNR (default: 10.0)
    double m_v0;            // Sound speed in water (default: 1500.0 m/s)

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
    
    // Energy+SNR-aware delay calculation
    double CalculateEnergySnrDelay(Ptr<Packet> pkt, Vector* p1, double snr);
    
    // SNR extraction from packet (with fallback estimation)
    double GetReceivedSnr(Ptr<Packet> p, Vector* senderPos);
    
    // SNR normalization
    double NormalizeSnr(double snr);
    
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

#endif /* AQUA_SIM_ROUTING_EVBF_V3_H */
