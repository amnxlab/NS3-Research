/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Vector Based Forwarding v4 (EVBFv4) for AquaSim-NG
 *
 * This version extends EVBF by adding SNR (Signal-to-Noise Ratio) as
 * a third factor in the holding time calculation, using INVERSE SCALING.
 * It combines:
 *   - Distance to routing vector (closer = better)
 *   - Residual energy (more energy = forward earlier)
 *   - Link quality (higher SNR = forward earlier)
 *
 * Inverse Formula:
 *    α' = α / ((Eres/Einit + εE) * (Qsnr + εS))
 *    Th = √α' * Tdelay + (R - d) / V0
 *
 * Where:
 *    Qsnr = min(1, max(0, SNR/SNRmax))
 *    εE, εS = Division safeguards
 *    Low energy OR low SNR → very high α' → longer delay
 *
 * For LINEAR SCALING with SNR, use EVBFv3 instead.
 *
 * Author: Eng. Ahmed Amin
 * Website: https://www.amnxlab.site/
 * Github: https://github.com/amnxlab
 */

#ifndef AQUA_SIM_ROUTING_EVBF_V4_H
#define AQUA_SIM_ROUTING_EVBF_V4_H

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

struct evbfv4_neighborhood {
    int number;
    Vector neighbor[MAX_NEIGHBOR];
};

typedef std::pair<AquaSimAddress, unsigned int> evbfv4_hash_entry;

class EVBFv4PktHashTable {
public:
    std::map<evbfv4_hash_entry, evbfv4_neighborhood*> m_htable;

    EVBFv4PktHashTable();
    ~EVBFv4PktHashTable();

    int m_windowSize;
    void Reset();
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum);
    void PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p);
    evbfv4_neighborhood* GetHash(AquaSimAddress senderAddr, unsigned int pkt_num);
};

/**
 * \brief Energy-Aware Vector Based Forwarding v4 (EVBFv4)
 * 
 * Uses INVERSE SCALING formula with SNR factor.
 */
class AquaSimEVBFv4 : public AquaSimRouting {
public:
    AquaSimEVBFv4();
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
    
    EVBFv4PktHashTable PktTable;
    EVBFv4PktHashTable SourceTable;
    EVBFv4PktHashTable Target_discoveryTable;
    EVBFv4PktHashTable SinkTable;

    double m_width;
    Vector m_targetPos;
    Ptr<UniformRandomVariable> m_rand;

    // ========== EVBFv4 Energy+SNR-Aware Parameters (Inverse Mode) ==========
    double m_Tdelay;        // Max delay constant (default: 0.5s)
    double m_epsilonE;      // Energy division safeguard (default: 0.01)
    double m_epsilonS;      // SNR division safeguard (default: 0.01)
    double m_snrMax;        // Maximum SNR for normalization (default: 100.0)
    double m_snrMin;        // Minimum acceptable SNR (default: 10.0)
    double m_v0;            // Sound speed in water (default: 1500.0 m/s)

    void Terminate();
    void Reset();
    void ConsiderNew(Ptr<Packet> pkt);
    void SetDelayTimer(Ptr<Packet>, double);
    void Timeout(Ptr<Packet>);
    double Advance(Ptr<Packet>);
    double Distance(Ptr<Packet>);
    double Projection(Ptr<Packet>);
    double CalculateDelay(Ptr<Packet>, Vector*);
    double CalculateEnergySnrDelay(Ptr<Packet> pkt, Vector* p1, double snr);
    double GetReceivedSnr(Ptr<Packet> p, Vector* senderPos);
    double NormalizeSnr(double snr);
    void CalculatePosition(Ptr<Packet>);
    bool IsTarget(Ptr<Packet>);
    bool IsCloseEnough(Ptr<Packet>);
    Ptr<Packet> CreatePacket();
    Ptr<Packet> PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type);
    void DataForSink(Ptr<Packet> pkt);
    void MACprepare(Ptr<Packet> pkt);
    void MACsend(Ptr<Packet> pkt, double delay = 0);
    virtual void DoDispose();
};

}  // namespace ns3

#endif /* AQUA_SIM_ROUTING_EVBF_V4_H */
