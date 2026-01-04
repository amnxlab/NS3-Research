/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Depth Based Routing v4 (EDBRv4) for AquaSim-NG
 *
 * This version extends EDBR by adding SNR (Signal-to-Noise Ratio) as
 * a third factor in the holding time calculation, using INVERSE SCALING.
 * It combines:
 *   - Depth progress (closer to surface = better)
 *   - Residual energy (more energy = forward earlier)
 *   - Link quality (higher SNR = forward earlier)
 *
 * Inverse Formula:
 *    Th = Tmin + k * (1 - Δd/Dmax) / ((Eres/Einit + εE) * (Qsnr + εS))
 *
 * Where:
 *    Qsnr = min(1, max(0, SNR/SNRmax))  -- Normalized SNR quality
 *    εE, εS = Division safeguards to avoid division by zero
 *    Low energy OR low SNR → very high delay → forward last
 *
 * For LINEAR SCALING with SNR, use EDBRv3 instead.
 *
 * Author: Eng. Ahmed Amin
 * Website: https://www.amnxlab.site/
 * Github: https://github.com/amnxlab
 */

#ifndef AQUA_SIM_ROUTING_EDBR_V4_H
#define AQUA_SIM_ROUTING_EDBR_V4_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"

#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/mobility-model.h"
#include "ns3/timer.h"

#include <deque>

#define EDBRV4_PORT           0xFF
#define EDBRV4_BEACON_DESYNC  0.1
#define EDBRV4_BEACON_INT     10
#define EDBRV4_JITTER         1
#define EDBRV4_IP_HDR_LEN     20

namespace ns3 {

class AquaSimEDBRv4;

// Timer for periodic beacons
class EDBRv4_BeaconTimer : public Timer {
public:
    EDBRv4_BeaconTimer(AquaSimEDBRv4 *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBRv4_BeaconTimer();
    void Expire();
private:
    AquaSimEDBRv4 *m_a;
};

// Timer for scheduled packet sending
class EDBRv4_SendingTimer : public Timer {
public:
    EDBRv4_SendingTimer(AquaSimEDBRv4 *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBRv4_SendingTimer();
    void Expire();
private:
    AquaSimEDBRv4 *m_a;
};

// Queue item for EDBRv4 packet queue
class QueueItemEdbrv4 : public Object {
public:
    QueueItemEdbrv4() : m_sendTime(0), m_snr(0.0) {}
    QueueItemEdbrv4(Ptr<Packet> p, double t, double snr = 0.0) 
        : m_p(p), m_sendTime(t), m_snr(snr) {}
    ~QueueItemEdbrv4();

    Ptr<Packet> m_p;
    double m_sendTime;
    double m_snr;
};

// Packet queue for EDBRv4
class EDBRv4PacketQueue : public Object {
public:
    EDBRv4PacketQueue() : m_dq() {}
    ~EDBRv4PacketQueue() {
        QueueItemEdbrv4* tmp;
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
    QueueItemEdbrv4* front() { return m_dq.front(); };
    void insert(QueueItemEdbrv4* q);
    bool update(Ptr<Packet> p, double t);
    bool purge(Ptr<Packet> p);

private:
    std::deque<QueueItemEdbrv4*> m_dq;
};

// Neighbor entry for EDBRv4
class EDBRv4NeighbEnt {
public:
    EDBRv4NeighbEnt() : m_routeFlag(0) {
        m_location = Vector();
    }

    Vector m_location;
    AquaSimAddress m_netID;
    int m_routeFlag;
};

// Neighbor table for EDBRv4
class EDBRv4NeighbTable : public Object {
public:
    EDBRv4NeighbTable();
    ~EDBRv4NeighbTable();
    static TypeId GetTypeId(void);

    void Dump(void);
    void EntDelete(const EDBRv4NeighbEnt *e);
    EDBRv4NeighbEnt *EntAdd(const EDBRv4NeighbEnt *e);
    EDBRv4NeighbEnt *EntFindShallowest(Vector location);
    void UpdateRouteFlag(AquaSimAddress, int);

private:
    int m_numEnts;
    int m_maxEnts;
    EDBRv4NeighbEnt **m_tab;
};

// Packet cache for EDBRv4
class EDBRv4PktCache {
public:
    EDBRv4PktCache();
    ~EDBRv4PktCache();
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
 * \brief Energy-Aware Depth Based Routing v4 (EDBRv4) protocol
 * 
 * Extends DBR by incorporating BOTH residual energy AND link quality (SNR)
 * into holding time calculation using INVERSE SCALING formula.
 * Strong bias: low energy or low SNR = very high delay.
 */
class AquaSimEDBRv4 : public AquaSimRouting {
    friend class EDBRv4_BeaconTimer;
    friend class EDBRv4_SendingTimer;

public:
    AquaSimEDBRv4();
    virtual ~AquaSimEDBRv4();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void DeadNeighb_Callback(EDBRv4NeighbEnt *ne);
    void Beacon_Callback(void);
    void Send_Callback(void);

protected:
    double m_bInt;
    double m_bDesync;
    double m_latest;

    // ========== EDBRv4 Energy+SNR-Aware Parameters (Inverse Mode) ==========
    double m_Tmin;          // Minimum backoff time (default: 0.1s)
    double m_k;             // Scaling factor (default: 4.0s)
    double m_Dmax;          // Max useful depth gain (default: 300m)
    double m_epsilonE;      // Energy division safeguard (default: 0.01)
    double m_epsilonS;      // SNR division safeguard (default: 0.01)
    double m_snrMax;        // Maximum SNR for normalization (default: 100.0)
    double m_snrMin;        // Minimum acceptable SNR (default: 10.0)

    EDBRv4NeighbTable *m_nTab;
    EDBRv4_BeaconTimer *m_beaconTimer;
    EDBRv4_SendingTimer *m_sendTimer;

    EDBRv4PktCache *m_pc;
    EDBRv4PacketQueue m_pq;
    int m_pktCnt;
    Ptr<UniformRandomVariable> m_rand;

    void ForwardPacket(Ptr<Packet>, int = 0);
    Ptr<Packet> MakeBeacon(void);
    void SendBeacon(void);
    void BeaconIn(Ptr<Packet>);
    void HandlePktForward(Ptr<Packet> p);
    
    // Energy+SNR-aware delay calculation (INVERSE formula)
    double CalculateEnergySnrDelay(double deltaDepth, double snr);
    
    double GetReceivedSnr(Ptr<Packet> p);
    double NormalizeSnr(double snr);

    virtual void DoDispose();
};

}  // namespace ns3

#endif  /* AQUA_SIM_ROUTING_EDBR_V4_H */
