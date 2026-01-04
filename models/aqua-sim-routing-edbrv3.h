/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Depth Based Routing v3 (EDBRv3) for AquaSim-NG
 *
 * This version extends EDBR by adding SNR (Signal-to-Noise Ratio) as
 * a third factor in the holding time calculation, using LINEAR SCALING.
 * It combines:
 *   - Depth progress (closer to surface = better)
 *   - Residual energy (more energy = forward earlier)
 *   - Link quality (higher SNR = forward earlier)
 *
 * Linear Formula:
 *    Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit) * (1 - Qsnr)
 *
 * Where:
 *    Qsnr = min(1, max(0, SNR/SNRmax))  -- Normalized SNR quality
 *    Higher energy + higher SNR → smaller delay → forward earlier
 *
 * For INVERSE SCALING with SNR, use EDBRv4 instead.
 *
 * Author: Eng. Ahmed Amin
 * Website: https://www.amnxlab.site/
 * Github: https://github.com/amnxlab
 */

#ifndef AQUA_SIM_ROUTING_EDBR_V3_H
#define AQUA_SIM_ROUTING_EDBR_V3_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"

#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/mobility-model.h"
#include "ns3/timer.h"

#include <deque>

#define EDBRV3_PORT           0xFF
#define EDBRV3_BEACON_DESYNC  0.1     // desynchronizing form for alive beacons
#define EDBRV3_BEACON_INT     10      // interval between beacons
#define EDBRV3_JITTER         1       // jitter for broadcasting
#define EDBRV3_IP_HDR_LEN     20

namespace ns3 {

class AquaSimEDBRv3;

// Timer for periodic beacons
class EDBRv3_BeaconTimer : public Timer {
public:
    EDBRv3_BeaconTimer(AquaSimEDBRv3 *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBRv3_BeaconTimer();
    void Expire();
private:
    AquaSimEDBRv3 *m_a;
};

// Timer for scheduled packet sending
class EDBRv3_SendingTimer : public Timer {
public:
    EDBRv3_SendingTimer(AquaSimEDBRv3 *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBRv3_SendingTimer();
    void Expire();
private:
    AquaSimEDBRv3 *m_a;
};

// Queue item for EDBRv3 packet queue
class QueueItemEdbrv3 : public Object {
public:
    QueueItemEdbrv3() : m_sendTime(0), m_snr(0.0) {}
    QueueItemEdbrv3(Ptr<Packet> p, double t, double snr = 0.0) 
        : m_p(p), m_sendTime(t), m_snr(snr) {}
    ~QueueItemEdbrv3();

    Ptr<Packet> m_p;        // pointer to the packet
    double m_sendTime;      // time to send the packet
    double m_snr;           // SNR value when packet was received
};

// Packet queue for EDBRv3
class EDBRv3PacketQueue : public Object {
public:
    EDBRv3PacketQueue() : m_dq() {}
    ~EDBRv3PacketQueue() {
        QueueItemEdbrv3* tmp;
        while(!m_dq.empty()) {
            tmp = m_dq.back();
            m_dq.pop_back();
            delete tmp;
        }
        m_dq.clear();
    }

    bool empty() { return m_dq.empty(); }
    int size() { return m_dq.size(); }
    void dump();

    void pop() { m_dq.pop_front(); };
    QueueItemEdbrv3* front() { return m_dq.front(); };
    void insert(QueueItemEdbrv3* q);
    bool update(Ptr<Packet> p, double t);
    bool purge(Ptr<Packet> p);

private:
    std::deque<QueueItemEdbrv3*> m_dq;
};

// Neighbor entry for EDBRv3
class EDBRv3NeighbEnt {
public:
    EDBRv3NeighbEnt() : m_routeFlag(0) {
        m_location = Vector();
    }

    Vector m_location;          // location of neighbor
    AquaSimAddress m_netID;     // IP of neighbor
    int m_routeFlag;            // indicates routing path exists
};

// Neighbor table for EDBRv3
class EDBRv3NeighbTable : public Object {
public:
    EDBRv3NeighbTable();
    ~EDBRv3NeighbTable();
    static TypeId GetTypeId(void);

    void Dump(void);
    void EntDelete(const EDBRv3NeighbEnt *e);
    EDBRv3NeighbEnt *EntAdd(const EDBRv3NeighbEnt *e);
    EDBRv3NeighbEnt *EntFindShallowest(Vector location);
    void UpdateRouteFlag(AquaSimAddress, int);

private:
    int m_numEnts;
    int m_maxEnts;
    EDBRv3NeighbEnt **m_tab;
};

// Packet cache for EDBRv3
class EDBRv3PktCache {
public:
    EDBRv3PktCache();
    ~EDBRv3PktCache();
    static TypeId GetTypeId(void);

    int& Size(void) { return m_size; }

    int AccessPacket(int pid);
    void AddPacket(int pid);
    void DeletePacket(int pid);
    void Dump(void);

private:
    int *m_pCache;
    int m_size;
    int m_maxSize;
};

/**
 * \ingroup aqua-sim-ng
 *
 * \brief Energy-Aware Depth Based Routing v3 (EDBRv3) protocol
 * 
 * Extends DBR by incorporating BOTH residual energy AND link quality (SNR)
 * into holding time calculation. Nodes with higher energy and better link
 * quality forward packets earlier.
 */
class AquaSimEDBRv3 : public AquaSimRouting {
    friend class EDBRv3_BeaconTimer;
    friend class EDBRv3_SendingTimer;

public:
    AquaSimEDBRv3();
    virtual ~AquaSimEDBRv3();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void DeadNeighb_Callback(EDBRv3NeighbEnt *ne);
    void Beacon_Callback(void);
    void Send_Callback(void);

protected:
    // Beacon parameters
    double m_bInt;          // beacon interval
    double m_bDesync;       // desynchronizing term
    double m_latest;        // latest time to send packet

    // ========== EDBRv3 Energy+SNR-Aware Parameters (Linear Mode) ==========
    double m_Tmin;          // Minimum backoff time (default: 0.1s)
    double m_k;             // Scaling factor (default: 4.0s)
    double m_Dmax;          // Max useful depth gain (default: 300m)
    double m_snrMax;        // Maximum SNR for normalization (default: 100.0)
    double m_snrMin;        // Minimum acceptable SNR (default: 10.0)

    // Helper objects
    EDBRv3NeighbTable *m_nTab;
    EDBRv3_BeaconTimer *m_beaconTimer;
    EDBRv3_SendingTimer *m_sendTimer;

    EDBRv3PktCache *m_pc;
    EDBRv3PacketQueue m_pq;
    int m_pktCnt;
    Ptr<UniformRandomVariable> m_rand;

    // Core functions
    void ForwardPacket(Ptr<Packet>, int = 0);
    Ptr<Packet> MakeBeacon(void);
    void SendBeacon(void);
    void BeaconIn(Ptr<Packet>);
    void HandlePktForward(Ptr<Packet> p);
    
    // Energy+SNR-aware delay calculation
    double CalculateEnergySnrDelay(double deltaDepth, double snr);
    
    // SNR extraction from packet (with fallback estimation)
    double GetReceivedSnr(Ptr<Packet> p);
    
    // SNR normalization
    double NormalizeSnr(double snr);

    virtual void DoDispose();
};

}  // namespace ns3

#endif  /* AQUA_SIM_ROUTING_EDBR_V3_H */
