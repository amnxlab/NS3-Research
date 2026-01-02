/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Depth Based Routing v2 (EDBRv2) for AquaSim-NG
 *
 * This version of EDBR is configured by default to use the "Inverse Scaling" formula,
 * providing a stronger bias against low-energy nodes compared to the linear model.
 * It inherits from the base EDBR structure but defaults to the more aggressive
 * energy conservation strategy.
 *
 * Formula Used (Inverse Scaling):
 *    Th = Tmin + k * (1 - Δd/Dmax) * (1 / (Eres/Einit + ε))
 *
 * Member Variables:
 * - m_Tmin: Minimum holding time
 * - m_k: Scaling factor
 * - m_Dmax: Max depth gain
 * - m_epsilon: Division safeguard
 * - m_useInverseScaling: Set to true by default for this variant.
 *
 * Author: Research Implementation
 */

#ifndef AQUA_SIM_ROUTING_EDBRv2_H
#define AQUA_SIM_ROUTING_EDBRv2_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"

#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/mobility-model.h"
#include "ns3/timer.h"

#include <deque>

#define EDBRv2_PORT           0xFF
#define EDBRv2_BEACON_DESYNC  0.1     // desynchronizing form for alive beacons
#define EDBRv2_BEACON_INT     10      // interval between beacons
#define EDBRv2_JITTER         1       // jitter for broadcasting
#define EDBRv2_IP_HDR_LEN     20

namespace ns3 {

class AquaSimEDBRv2v2;

// Timer for periodic beacons
class EDBRv2_BeaconTimer : public Timer {
public:
    EDBRv2_BeaconTimer(AquaSimEDBRv2v2 *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBRv2_BeaconTimer();
    void Expire();
private:
    AquaSimEDBRv2v2 *m_a;
};

// Timer for scheduled packet sending
class EDBRv2_SendingTimer : public Timer {
public:
    EDBRv2_SendingTimer(AquaSimEDBRv2v2 *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBRv2_SendingTimer();
    void Expire();
private:
    AquaSimEDBRv2v2 *m_a;
};

// Queue item for EDBRv2 packet queue
class QueueItemEdbrv2 : public Object {
public:
    QueueItemEdbrv2() : m_sendTime(0) {}
    QueueItemEdbrv2(Ptr<Packet> p, double t) : m_p(p), m_sendTime(t) {}
    ~QueueItemEdbrv2();

    Ptr<Packet> m_p;        // pointer to the packet
    double m_sendTime;      // time to send the packet
};

// Packet queue for EDBRv2
class EDBRv2PacketQueue : public Object {
public:
    EDBRv2PacketQueue() : m_dq() {}
    ~EDBRv2PacketQueue() {
        QueueItemEdbrv2* tmp;
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
    QueueItemEdbrv2* front() { return m_dq.front(); };
    void insert(QueueItemEdbrv2* q);
    bool update(Ptr<Packet> p, double t);
    bool purge(Ptr<Packet> p);

private:
    std::deque<QueueItemEdbrv2*> m_dq;
};

// Neighbor entry for EDBRv2
class EDBRv2NeighbEnt {
public:
    EDBRv2NeighbEnt() : m_routeFlag(0) {
        m_location = Vector();
    }

    Vector m_location;          // location of neighbor
    AquaSimAddress m_netID;     // IP of neighbor
    int m_routeFlag;            // indicates routing path exists
};

// Neighbor table for EDBRv2
class EDBRv2NeighbTable : public Object {
public:
    EDBRv2NeighbTable();
    ~EDBRv2NeighbTable();
    static TypeId GetTypeId(void);

    void Dump(void);
    void EntDelete(const EDBRv2NeighbEnt *e);
    EDBRv2NeighbEnt *EntAdd(const EDBRv2NeighbEnt *e);
    EDBRv2NeighbEnt *EntFindShallowest(Vector location);
    void UpdateRouteFlag(AquaSimAddress, int);

private:
    int m_numEnts;
    int m_maxEnts;
    EDBRv2NeighbEnt **m_tab;
};

// Packet cache for EDBRv2
class EDBRv2PktCache {
public:
    EDBRv2PktCache();
    ~EDBRv2PktCache();
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
 * \brief Energy-Aware Depth Based Routing (EDBRv2) protocol
 * 
 * Extends DBR by incorporating residual energy into holding time 
 * calculation. Higher-energy nodes forward earlier.
 */
class AquaSimEDBRv2v2 : public AquaSimRouting {
    friend class EDBRv2_BeaconTimer;
    friend class EDBRv2_SendingTimer;

public:
    AquaSimEDBRv2v2();
    virtual ~AquaSimEDBRv2v2();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void DeadNeighb_Callback(EDBRv2NeighbEnt *ne);
    void Beacon_Callback(void);
    void Send_Callback(void);

protected:
    // Beacon parameters
    double m_bInt;          // beacon interval
    double m_bDesync;       // desynchronizing term
    double m_latest;        // latest time to send packet

    // ========== EDBRv2 Energy-Aware Parameters ==========
    double m_Tmin;          // Minimum backoff time (default: 0.1s)
    double m_k;             // Scaling factor (default: 4.0s)
    double m_Dmax;          // Max useful depth gain (default: 300m)
    double m_epsilon;       // Division safeguard (default: 0.01)
    bool m_useInverseScaling; // Use inverse energy formula

    // Helper objects
    EDBRv2NeighbTable *m_nTab;
    EDBRv2_BeaconTimer *m_beaconTimer;
    EDBRv2_SendingTimer *m_sendTimer;

    EDBRv2PktCache *m_pc;
    EDBRv2PacketQueue m_pq;
    int m_pktCnt;
    Ptr<UniformRandomVariable> m_rand;

    // Core functions
    void ForwardPacket(Ptr<Packet>, int = 0);
    Ptr<Packet> MakeBeacon(void);
    void SendBeacon(void);
    void BeaconIn(Ptr<Packet>);
    void HandlePktForward(Ptr<Packet> p);
    
    // Energy-aware delay calculation
    double CalculateEnergyAwareDelay(double deltaDepth);

    virtual void DoDispose();
};

}  // namespace ns3

#endif  /* AQUA_SIM_ROUTING_EDBRv2_H */
