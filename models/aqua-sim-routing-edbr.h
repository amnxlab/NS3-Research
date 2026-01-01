/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Depth Based Routing (EDBR) for AquaSim-NG
 *
 * This implementation extends the standard DBR protocol by incorporating residual
 * energy into the holding time calculation. It aims to balance energy consumption
 * across the network by assigning shorter holding times (and thus higher forwarding
 * priority) to nodes with higher residual energy.
 *
 * Formulas supported:
 *    Linear Penalty (Default):
 *    Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit)
 *    - Uses a linear energy penalty.
 *
 * Member Variables:
 * - m_Tmin: Minimum holding time (default: 0.1s)
 * - m_k: Scaling factor for depth difference (default: 4.0s)
 * - m_Dmax: Maximum theoretical depth difference (default: 300m)
 * - m_epsilon: Small constant to avoid division by zero (default: 0.01)
 *
 * Author: Research Implementation
 */

#ifndef AQUA_SIM_ROUTING_EDBR_H
#define AQUA_SIM_ROUTING_EDBR_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"

#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include "ns3/mobility-model.h"
#include "ns3/timer.h"

#include <deque>

#define EDBR_PORT           0xFF
#define EDBR_BEACON_DESYNC  0.1     // desynchronizing form for alive beacons
#define EDBR_BEACON_INT     10      // interval between beacons
#define EDBR_JITTER         1       // jitter for broadcasting
#define EDBR_IP_HDR_LEN     20

namespace ns3 {

class AquaSimEDBR;

// Timer for periodic beacons
class EDBR_BeaconTimer : public Timer {
public:
    EDBR_BeaconTimer(AquaSimEDBR *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBR_BeaconTimer();
    void Expire();
private:
    AquaSimEDBR *m_a;
};

// Timer for scheduled packet sending
class EDBR_SendingTimer : public Timer {
public:
    EDBR_SendingTimer(AquaSimEDBR *a) :
        Timer(Timer::CANCEL_ON_DESTROY), m_a(a) {}
    ~EDBR_SendingTimer();
    void Expire();
private:
    AquaSimEDBR *m_a;
};

// Queue item for EDBR packet queue
class QueueItemEdbr : public Object {
public:
    QueueItemEdbr() : m_sendTime(0) {}
    QueueItemEdbr(Ptr<Packet> p, double t) : m_p(p), m_sendTime(t) {}
    ~QueueItemEdbr();

    Ptr<Packet> m_p;        // pointer to the packet
    double m_sendTime;      // time to send the packet
};

// Packet queue for EDBR
class EDBRPacketQueue : public Object {
public:
    EDBRPacketQueue() : m_dq() {}
    ~EDBRPacketQueue() {
        QueueItemEdbr* tmp;
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
    QueueItemEdbr* front() { return m_dq.front(); };
    void insert(QueueItemEdbr* q);
    bool update(Ptr<Packet> p, double t);
    bool purge(Ptr<Packet> p);

private:
    std::deque<QueueItemEdbr*> m_dq;
};

// Neighbor entry for EDBR
class EDBRNeighbEnt {
public:
    EDBRNeighbEnt() : m_routeFlag(0) {
        m_location = Vector();
    }

    Vector m_location;          // location of neighbor
    AquaSimAddress m_netID;     // IP of neighbor
    int m_routeFlag;            // indicates routing path exists
};

// Neighbor table for EDBR
class EDBRNeighbTable : public Object {
public:
    EDBRNeighbTable();
    ~EDBRNeighbTable();
    static TypeId GetTypeId(void);

    void Dump(void);
    void EntDelete(const EDBRNeighbEnt *e);
    EDBRNeighbEnt *EntAdd(const EDBRNeighbEnt *e);
    EDBRNeighbEnt *EntFindShallowest(Vector location);
    void UpdateRouteFlag(AquaSimAddress, int);

private:
    int m_numEnts;
    int m_maxEnts;
    EDBRNeighbEnt **m_tab;
};

// Packet cache for EDBR
class EDBRPktCache {
public:
    EDBRPktCache();
    ~EDBRPktCache();
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
 * \brief Energy-Aware Depth Based Routing (EDBR) protocol
 * 
 * Extends DBR by incorporating residual energy into holding time 
 * calculation. Higher-energy nodes forward earlier.
 */
class AquaSimEDBR : public AquaSimRouting {
    friend class EDBR_BeaconTimer;
    friend class EDBR_SendingTimer;

public:
    AquaSimEDBR();
    virtual ~AquaSimEDBR();
    static TypeId GetTypeId(void);
    int64_t AssignStreams(int64_t stream);

    virtual bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);

    void DeadNeighb_Callback(EDBRNeighbEnt *ne);
    void Beacon_Callback(void);
    void Send_Callback(void);

protected:
    // Beacon parameters
    double m_bInt;          // beacon interval
    double m_bDesync;       // desynchronizing term
    double m_latest;        // latest time to send packet

    // ========== EDBR Energy-Aware Parameters ==========
    double m_Tmin;          // Minimum backoff time (default: 0.1s)
    double m_k;             // Scaling factor (default: 4.0s)
    double m_Dmax;          // Max useful depth gain (default: 300m)
    double m_epsilon;       // Division safeguard (default: 0.01)


    // Helper objects
    EDBRNeighbTable *m_nTab;
    EDBR_BeaconTimer *m_beaconTimer;
    EDBR_SendingTimer *m_sendTimer;

    EDBRPktCache *m_pc;
    EDBRPacketQueue m_pq;
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

#endif  /* AQUA_SIM_ROUTING_EDBR_H */
