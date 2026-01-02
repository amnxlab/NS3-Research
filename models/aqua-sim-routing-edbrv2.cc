/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Depth Based Routing (EDBRv2) for AquaSim-NG
 * 
 * This protocol extends DBR by incorporating residual energy into the 
 * packet holding time calculation. Higher-energy nodes forward packets 
 * earlier, balancing energy consumption across the network.
 * 
 * Based on the paper formulas:
 * Form 1 (Linear Penalty): Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit)
 * Form 2 (Inverse Scaling): Th = Tmin + k * (1 - Δd/Dmax) * (1 / (Eres/Einit + ε))
 * 
 * Author: Research Implementation
 */

#include "aqua-sim-routing-edbrv2.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-pt-tag.h"

#include "ns3/ipv4-header.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimEDBRv2v2");

// Constants for delay calculation
#define EDBRv2_MAX_DELAY      0.2     // maximal propagation delay for one hop
#define EDBRv2_MAX_RANGE      100     // maximal transmission range
#define EDBRv2_MIN_BACKOFF    0.0     // minimal backoff time
#define EDBRv2_MAX_HOPS       3
#define EDBRv2_DEPTH_THRESHOLD 0.0
#define EDBRv2_SCALE          1.0

// ==================== Timer Implementations ====================

EDBRv2_BeaconTimer::~EDBRv2_BeaconTimer()
{
    m_a = 0;
    delete m_a;
}

void EDBRv2_BeaconTimer::Expire()
{
    m_a->Beacon_Callback();
}

EDBRv2_SendingTimer::~EDBRv2_SendingTimer()
{
    m_a = 0;
    delete m_a;
}

void EDBRv2_SendingTimer::Expire()
{
    m_a->Send_Callback();
}

// ==================== QueueItemEdbrv2 ====================

QueueItemEdbrv2::~QueueItemEdbrv2()
{
    m_p = 0;
}

// ==================== EDBRv2PacketQueue ====================

void EDBRv2PacketQueue::insert(QueueItemEdbrv2 *q)
{
    QueueItemEdbrv2 *tmp;
    std::deque<QueueItemEdbrv2*>::iterator iter;

    iter = m_dq.begin();
    while (iter != m_dq.end())
    {
        tmp = *iter;
        if (tmp->m_sendTime > q->m_sendTime)
        {
            m_dq.insert(iter, q);
            return;
        }
        iter++;
    }
    m_dq.push_back(q);
}

bool EDBRv2PacketQueue::update(Ptr<Packet> p, double t)
{
    Ptr<Packet> pkt;
    uint32_t curID;
    std::deque<QueueItemEdbrv2*>::iterator iter;
    DBRHeader dbrh;
    AquaSimHeader ash;
    p->RemoveHeader(ash);
    p->PeekHeader(dbrh);
    p->AddHeader(ash);

    curID = dbrh.GetPacketID();

    iter = m_dq.begin();
    while (iter != m_dq.end())
    {
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        if (dbrh.GetPacketID() == curID)
        {
            if ((*iter)->m_sendTime > t)
            {
                m_dq.erase(iter);
                return true;
            }
            else
                return false;
        }
    }
    return true;
}

bool EDBRv2PacketQueue::purge(Ptr<Packet> p)
{
    Ptr<Packet> pkt;
    uint32_t curID;
    std::deque<QueueItemEdbrv2*>::iterator iter;
    DBRHeader dbrh;
    AquaSimHeader ash;
    p->RemoveHeader(ash);
    p->PeekHeader(dbrh);
    p->AddHeader(ash);

    curID = dbrh.GetPacketID();

    iter = m_dq.begin();
    while (iter != m_dq.end())
    {
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        if (dbrh.GetPacketID() == curID)
        {
            m_dq.erase(iter);
            return true;
        }
        iter++;
    }
    return false;
}

void EDBRv2PacketQueue::dump()
{
    std::deque<QueueItemEdbrv2*>::iterator iter;
    DBRHeader dbrh;
    AquaSimHeader ash;
    int i = 0;

    iter = m_dq.begin();
    while (iter != m_dq.end())
    {
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        NS_LOG_INFO("EDBRv2PacketQueue::dump:[" << i << "] packetID " <<
            dbrh.GetPacketID() << ", send time " << (*iter)->m_sendTime);
        iter++;
        i++;
    }
}

// ==================== EDBRv2NeighbTable ====================

EDBRv2NeighbTable::EDBRv2NeighbTable()
{
    m_numEnts = 0;
    m_maxEnts = 100;
    m_tab = new EDBRv2NeighbEnt* [100];

    for (int i = 0; i < 100; i++)
        m_tab[i] = new EDBRv2NeighbEnt();
}

EDBRv2NeighbTable::~EDBRv2NeighbTable()
{
    for (int i = 0; i < m_maxEnts; i++)
        delete m_tab[i];
    delete[] m_tab;
}

TypeId EDBRv2NeighbTable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EDBRv2NeighbTable");
    return tid;
}

void EDBRv2NeighbTable::Dump(void)
{
    for (int i = 0; i < m_numEnts; i++)
    {
        NS_LOG_INFO("EDBRv2NeighbTable::dump: m_tab[" << i << "]: " << m_tab[i]->m_netID
            << " position(" << m_tab[i]->m_location.x << "," << m_tab[i]->m_location.y
            << "," << m_tab[i]->m_location.z << ")");
    }
}

void EDBRv2NeighbTable::EntDelete(const EDBRv2NeighbEnt *ne)
{
    int l, r, m;
    EDBRv2NeighbEnt *owslot;

    l = 0; r = m_numEnts - 1;
    while (l <= r)
    {
        m = l + (r - l)/2;
        if (m_tab[m]->m_netID < ne->m_netID)
            l = m + 1;
        else if (ne->m_netID < m_tab[m]->m_netID)
            r = m - 1;
        else
            break;
    }

    if (l > r) return;

    owslot = m_tab[m];
    int i = m + 1;
    while (i < m_numEnts)
        m_tab[i - 1] = m_tab[i+1];

    m_tab[m_numEnts-1] = owslot;
    m_numEnts--;
}

EDBRv2NeighbEnt* EDBRv2NeighbTable::EntAdd(const EDBRv2NeighbEnt *ne)
{
    EDBRv2NeighbEnt *pe;
    int i, j;
    int l, r, m;

    for (i = 0; i < m_numEnts; i++)
        if (m_tab[i]->m_netID == ne->m_netID)
        {
            m_tab[i]->m_location.x = ne->m_location.x;
            m_tab[i]->m_location.y = ne->m_location.y;
            m_tab[i]->m_location.z = ne->m_location.z;
            return m_tab[i];
        }

    if (m_numEnts == m_maxEnts)
    {
        EDBRv2NeighbEnt **tmp = m_tab;
        m_maxEnts *= 2;
        m_tab = new EDBRv2NeighbEnt* [m_maxEnts];
        bcopy(tmp, m_tab, m_numEnts*sizeof(EDBRv2NeighbEnt *));

        for (i = m_numEnts; i < m_maxEnts; i++)
            m_tab[i] = new EDBRv2NeighbEnt();

        delete[] tmp;
    }

    if (m_numEnts == 0)
        i = 0;
    else
    {
        l = 0;
        r = m_numEnts - 1;

        while (r > l)
        {
            m = l + (r - l) / 2;
            if (ne->m_netID < m_tab[m]->m_netID)
                r = m - 1;
            else
                l = m + 1;
        }

        if (r < l)
            i = r + 1;
        else
            if (ne->m_netID < m_tab[r]->m_netID)
                i = r;
            else
                i = r + 1;
    }

    if (i <= (m_numEnts - 1))
        pe = m_tab[m_numEnts];

    j = m_numEnts - 1;
    while (j >= i)
    {
        m_tab[j+1] = m_tab[j];
        j--;
    }

    if (i <= (m_numEnts - 1))
        m_tab[i] = pe;
    m_tab[i]->m_netID = ne->m_netID;
    m_tab[i]->m_location.x = ne->m_location.x;
    m_tab[i]->m_location.y = ne->m_location.y;
    m_tab[i]->m_location.z = ne->m_location.z;
    m_numEnts++;

    return m_tab[i];
}

void EDBRv2NeighbTable::UpdateRouteFlag(AquaSimAddress addr, int val)
{
    for (int i = 0; i < m_numEnts; i++)
    {
        if (m_tab[i]->m_netID == addr)
        {
            m_tab[i]->m_routeFlag = val;
            return;
        }
    }
}

EDBRv2NeighbEnt* EDBRv2NeighbTable::EntFindShallowest(Vector location)
{
    EDBRv2NeighbEnt *ne = 0;
    double t = location.z;

    NS_LOG_DEBUG("EDBRv2NeighbTable::EntFindShallowest: location=(" <<
        location.x << "," << location.y << "," << location.z <<
        ") has " << m_numEnts << " neighbors");

    for (int i = 0; i < m_numEnts; i++)
    {
        NS_LOG_DEBUG("EDBRv2NeighbTable::EntFindShallowest: [" << m_tab[i]->m_netID <<
            "] position(" << m_tab[i]->m_location.x << "," <<
            m_tab[i]->m_location.y << "," << m_tab[i]->m_location.z << ")");

        if (m_tab[i]->m_routeFlag == 1)
        {
            ne = m_tab[i];
            return ne;
        }

        if (m_tab[i]->m_location.z > t)
        {
            t = m_tab[i]->m_location.z;
            ne = m_tab[i];
        }
    }
    return ne;
}

// ==================== EDBRv2PktCache ====================

EDBRv2PktCache::EDBRv2PktCache()
{
    m_maxSize = 1500;
    m_size = 0;
    m_pCache = new int[1500];
}

EDBRv2PktCache::~EDBRv2PktCache()
{
    delete[] m_pCache;
}

TypeId EDBRv2PktCache::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EDBRv2PktCache");
    return tid;
}

int EDBRv2PktCache::AccessPacket(int p)
{
    int i, j;
    int tmp;

    for (i = 0; i < m_size; i++) {
        if (m_pCache[i] == p) {
            tmp = p;
            for (j = i; j < m_size - 1; j++)
                m_pCache[j] = m_pCache[j+1];
            m_pCache[m_size-1] = tmp;
            return 1;
        }
    }
    return 0;
}

void EDBRv2PktCache::AddPacket(int p)
{
    if (m_size == m_maxSize) {
        NS_LOG_WARN("Cache is full!");
        return;
    }
    m_pCache[m_size] = p;
    m_size++;
}

void EDBRv2PktCache::DeletePacket(int p)
{
}

void EDBRv2PktCache::Dump(void)
{
    for (int i = 0; i < m_size; i++)
    {
        NS_LOG_INFO("[" << i << "]: " << m_pCache[i]);
    }
}

// ==================== AquaSimEDBRv2v2 ====================

NS_OBJECT_ENSURE_REGISTERED(AquaSimEDBRv2v2);

AquaSimEDBRv2v2::AquaSimEDBRv2v2() :
    m_bInt(EDBRv2_BEACON_INT), m_bDesync(EDBRv2_BEACON_DESYNC), m_pktCnt(0),
    m_Tmin(0.1), m_k(4.0), m_Dmax(300.0), m_epsilon(0.01), m_useInverseScaling(true)
{
    m_nTab = new EDBRv2NeighbTable();
    m_pc = new EDBRv2PktCache();
    m_rand = CreateObject<UniformRandomVariable>();

    m_beaconTimer = new EDBRv2_BeaconTimer(this);
    m_beaconTimer->SetFunction(&EDBRv2_BeaconTimer::Expire, m_beaconTimer);
    m_beaconTimer->Schedule(Seconds(m_rand->GetValue(0.0, m_bInt)));

    m_sendTimer = new EDBRv2_SendingTimer(this);
    m_sendTimer->SetFunction(&EDBRv2_SendingTimer::Expire, m_sendTimer);
}

AquaSimEDBRv2v2::~AquaSimEDBRv2v2()
{
    delete m_nTab;
    delete m_pc;
}

TypeId AquaSimEDBRv2v2::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AquaSimEDBRv2v2")
        .SetParent<AquaSimRouting>()
        .AddConstructor<AquaSimEDBRv2v2>()
        .AddAttribute("Tmin", "Minimum backoff time in seconds",
            DoubleValue(0.1),
            MakeDoubleAccessor(&AquaSimEDBRv2v2::m_Tmin),
            MakeDoubleChecker<double>())
        .AddAttribute("K", "Scaling factor for delay calculation",
            DoubleValue(4.0),
            MakeDoubleAccessor(&AquaSimEDBRv2v2::m_k),
            MakeDoubleChecker<double>())
        .AddAttribute("Dmax", "Maximum useful depth gain in meters",
            DoubleValue(300.0),
            MakeDoubleAccessor(&AquaSimEDBRv2v2::m_Dmax),
            MakeDoubleChecker<double>())
        .AddAttribute("Epsilon", "Small constant to avoid division by zero",
            DoubleValue(0.01),
            MakeDoubleAccessor(&AquaSimEDBRv2v2::m_epsilon),
            MakeDoubleChecker<double>())
        .AddAttribute("UseInverseScaling", "Use inverse energy scaling formula",
            BooleanValue(false),
            MakeBooleanAccessor(&AquaSimEDBRv2v2::m_useInverseScaling),
            MakeBooleanChecker())
    ;
    return tid;
}

int64_t AquaSimEDBRv2v2::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rand->SetStream(stream);
    return 1;
}

void AquaSimEDBRv2v2::DoDispose()
{
    m_rand = 0;
    AquaSimRouting::DoDispose();
}

/**
 * Calculate energy-aware delay based on depth gain and residual energy.
 * 
 * Form 1 (Linear Penalty): Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit)
 * Form 2 (Inverse Scaling): Th = Tmin + k * (1 - Δd/Dmax) * (1 / (Eres/Einit + ε))
 * 
 * Higher energy nodes get smaller delay -> forward earlier
 * Higher depth gain -> smaller delay -> forward earlier
 */
double AquaSimEDBRv2v2::CalculateEnergyAwareDelay(double deltaDepth)
{
    // Get residual and initial energy
    double Eres = GetNetDevice()->EnergyModel()->GetEnergy();
    double Einit = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    
    // Avoid division by zero for energy ratio
    double energyRatio = (Einit > 0) ? (Eres / Einit) : 0.0;
    
    // Clamp energy ratio to [0, 1]
    if (energyRatio > 1.0) energyRatio = 1.0;
    if (energyRatio < 0.0) energyRatio = 0.0;
    
    // Calculate depth factor: (1 - Δd/Dmax)
    // Higher depth gain means smaller factor -> smaller delay
    double depthFactor = 1.0 - (deltaDepth / m_Dmax);
    if (depthFactor < 0.0) depthFactor = 0.0;
    if (depthFactor > 1.0) depthFactor = 1.0;
    
    double delay;
    
    if (m_useInverseScaling)
    {
        // Form 2: Th = Tmin + k * (1 - Δd/Dmax) * (1 / (Eres/Einit + ε))
        // Strong energy bias - low energy nodes get very high delay
        double inverseFactor = 1.0 / (energyRatio + m_epsilon);
        delay = m_Tmin + m_k * depthFactor * inverseFactor;
    }
    else
    {
        // Form 1: Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit)
        // Linear energy penalty - high energy means small penalty
        double energyPenalty = 1.0 - energyRatio;
        delay = m_Tmin + m_k * depthFactor * energyPenalty;
    }
    
    NS_LOG_DEBUG("AquaSimEDBRv2v2::CalculateEnergyAwareDelay: "
        << "Eres=" << Eres << " Einit=" << Einit 
        << " deltaDepth=" << deltaDepth 
        << " depthFactor=" << depthFactor
        << " delay=" << delay);
    
    return delay;
}

// Construct beacon packet
Ptr<Packet> AquaSimEDBRv2v2::MakeBeacon(void)
{
    Ptr<Packet> p = Create<Packet>();
    AquaSimHeader ash;
    DBRHeader dbrh;
    AquaSimPtTag ptag;

    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(dbrh.Size() + EDBRv2_IP_HDR_LEN);
    ash.SetDAddr(AquaSimAddress::GetBroadcast());
    ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

    ptag.SetPacketType(AquaSimPtTag::PT_DBR);

    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model == nullptr)
    {
        NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

    dbrh.SetPosition(model->GetPosition());
    dbrh.SetMode(DBRH_BEACON);
    dbrh.SetNHops(1);

    p->AddHeader(dbrh);
    p->AddHeader(ash);
    p->AddPacketTag(ptag);
    return p;
}

void AquaSimEDBRv2v2::SendBeacon(void)
{
    Ptr<Packet> p = MakeBeacon();
    AquaSimHeader ash;
    p->RemoveHeader(ash);

    if (p)
    {
        NS_ASSERT(!ash.GetErrorFlag());
        if (ash.GetDirection() == AquaSimHeader::UP)
            ash.SetDirection(AquaSimHeader::DOWN);
        p->AddHeader(ash);
        Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown, this,
                            p, AquaSimAddress::GetBroadcast(), Seconds(0));
    }
    else
    {
        NS_LOG_WARN("AquaSimEDBRv2v2::SendBeacon: ERROR, can't make new beacon!");
    }
}

void AquaSimEDBRv2v2::Send_Callback(void)
{
    QueueItemEdbrv2 *q;
    DBRHeader dbrh;
    AquaSimHeader ash;

    if (m_pq.empty())
        return;

    q = m_pq.front();
    m_pq.pop();
    Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown, this,
                        q->m_p, AquaSimAddress::GetBroadcast(), Seconds(0));

    (q->m_p)->RemoveHeader(ash);
    (q->m_p)->PeekHeader(dbrh);
    (q->m_p)->AddHeader(ash);
    m_pc->AddPacket(dbrh.GetPacketID());

    if (!m_pq.empty())
    {
        q = m_pq.front();
        m_latest = q->m_sendTime;
        m_sendTimer->Schedule(Seconds(m_latest - Simulator::Now().ToDouble(Time::S)));
    }
}

void AquaSimEDBRv2v2::Beacon_Callback(void)
{
    return;  // Beacons disabled for now
}

void AquaSimEDBRv2v2::DeadNeighb_Callback(EDBRv2NeighbEnt *ne)
{
    m_nTab->EntDelete(ne);
}

void AquaSimEDBRv2v2::ForwardPacket(Ptr<Packet> p, int flag)
{
    NS_LOG_FUNCTION(this);

    AquaSimHeader ash;
    DBRHeader dbrh;
    AquaSimPtTag ptag;
    p->RemoveHeader(ash);
    p->RemoveHeader(dbrh);
    p->RemovePacketTag(ptag);
    EDBRv2NeighbEnt *ne;

    double delay = 0.0;

    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model == nullptr)
    {
        NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

    NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: node=" <<
        GetNetDevice()->GetAddress() << " is forwarding to " << ash.GetDAddr());

    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(dbrh.Size() + EDBRv2_IP_HDR_LEN);
    ptag.SetPacketType(AquaSimPtTag::PT_DBR);

    switch (dbrh.GetMode())
    {
    case DBRH_DATA_GREEDY:
        ne = m_nTab->EntFindShallowest(model->GetPosition());
        if (ne)
        {
            ash.SetNextHop(ne->m_netID);
            NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: node=" <<
                GetNetDevice()->GetAddress() << " -> " << ne->m_netID);
        }
        else
        {
            NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: " <<
                GetNetDevice()->GetAddress() << " put pkt into recovery mode!");

            ash.SetNextHop(AquaSimAddress::GetBroadcast());
            dbrh.SetMode(DBRH_DATA_RECOVER);
            dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
            dbrh.SetOwner(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
            dbrh.SetNHops(EDBRv2_MAX_HOPS);
        }
        break;
    case DBRH_DATA_RECOVER:
        NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: " <<
            GetNetDevice()->GetAddress() << " forward in recovery algorithm!");

        if (m_pc->AccessPacket(dbrh.GetPacketID()))
        {
            NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: " <<
                GetNetDevice()->GetAddress() << " packet is already in cache!");
            return;
        }
        else
            m_pc->AddPacket(dbrh.GetPacketID());

        ne = m_nTab->EntFindShallowest(model->GetPosition());
        if (ne == NULL)
        {
            if (dbrh.GetNHops() <= 0)
            {
                NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: " <<
                    GetNetDevice()->GetAddress() << " drops pkt! (nhops < 0)");
                p = 0;
                return;
            }
            else
            {
                ash.SetNextHop(AquaSimAddress::GetBroadcast());
                dbrh.SetMode(DBRH_DATA_RECOVER);
                dbrh.SetOwner(dbrh.GetPrevHop());
                dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
                dbrh.SetNHops((dbrh.GetNHops()-1));
                delay = m_rand->GetValue() * EDBRv2_JITTER;
            }
        }
        else if (ne->m_netID != dbrh.GetPrevHop())
        {
            ash.SetNextHop(ne->m_netID);
            dbrh.SetMode(DBRH_DATA_GREEDY);
            NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: back to greedy:" <<
                GetNetDevice()->GetAddress() << " -> " << ne->m_netID);
        }
        else
        {
            NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: address:" <<
                GetNetDevice()->GetAddress() << ":dbrh->nhops=" << dbrh.GetNHops());

            if (dbrh.GetNHops() <= 0)
            {
                NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: " <<
                    GetNetDevice()->GetAddress() << " drops pkt! (nhops < 0)");
                p = 0;
                return;
            }
            else
            {
                NS_LOG_DEBUG("AquaSimEDBRv2v2::ForwardPacket: " <<
                    GetNetDevice()->GetAddress() << " is broadcasting!");
                ash.SetNextHop(AquaSimAddress::GetBroadcast());
                dbrh.SetMode(DBRH_DATA_RECOVER);
                dbrh.SetOwner(dbrh.GetPrevHop());
                dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
                dbrh.SetNHops((dbrh.GetNHops()-1));
                delay = m_rand->GetValue() * EDBRv2_JITTER;
            }
        }
        break;
    default:
        NS_LOG_WARN("AquaSimEDBRv2v2::ForwardPacket: Wrong! The packet can't be forwarded!");
        break;
    }

    NS_ASSERT(!ash.GetErrorFlag());
    p->AddHeader(dbrh);
    p->AddHeader(ash);
    p->AddPacketTag(ptag);
    Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this,
                        p, ash.GetNextHop(), Seconds(0));
}

void AquaSimEDBRv2v2::BeaconIn(Ptr<Packet> p)
{
    AquaSimHeader ash;
    DBRHeader dbrh;
    p->RemoveHeader(ash);
    p->PeekHeader(dbrh);
    p->AddHeader(ash);

    AquaSimAddress src = ash.GetSAddr();

    EDBRv2NeighbEnt *ne;
    ne = new EDBRv2NeighbEnt();

    NS_LOG_DEBUG("AquaSimEDBRv2v2::BeaconIn: " << GetNetDevice()->GetAddress() <<
        " got beacon from " << src);

    ne->m_location.x = dbrh.GetPosition().x;
    ne->m_location.y = dbrh.GetPosition().y;
    ne->m_location.z = dbrh.GetPosition().z;
    ne->m_netID = src;

    m_nTab->EntAdd(ne);
    delete ne;
    p = 0;
}

bool AquaSimEDBRv2v2::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this);

    AquaSimHeader ash;
    DBRHeader dbrh;
    AquaSimPtTag ptag;

    p->RemoveHeader(ash);
    if (ash.GetNumForwards() <= 0)
    {
        p->AddHeader(dbrh);
    }

    p->PeekHeader(dbrh);
    p->AddHeader(ash);
    p->PeekPacketTag(ptag);

    AquaSimAddress src = ash.GetSAddr();
    AquaSimAddress dst = ash.GetDAddr();

    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model == nullptr)
    {
        NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

    if (dbrh.GetMode() == DBRH_BEACON)
    {
        NS_LOG_DEBUG("AquaSimEDBRv2v2::Recv: address:" <<
            GetNetDevice()->GetAddress() << ": beacon pkt is received.");

        if (src != GetNetDevice()->GetAddress())
            BeaconIn(p);
        return true;
    }

    if ((src == GetNetDevice()->GetAddress()) &&
        (ash.GetNumForwards() == 0))
    {
        p->RemoveHeader(ash);
        p->RemoveHeader(dbrh);
        p->RemovePacketTag(ptag);

        ash.SetDirection(AquaSimHeader::DOWN);
        ptag.SetPacketType(AquaSimPtTag::PT_DBR);
        ash.SetSize(dbrh.Size() + EDBRv2_IP_HDR_LEN);
        ash.SetNextHop(AquaSimAddress::GetBroadcast());

        dbrh.SetMode(DBRH_DATA_GREEDY);
        dbrh.SetPacketID(m_pktCnt++);
        dbrh.SetDepth(model->GetPosition().z);

        NS_ASSERT(!ash.GetErrorFlag());
        p->AddHeader(dbrh);
        p->AddHeader(ash);
        p->AddPacketTag(ptag);
        Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown, this,
                            p, ash.GetNextHop(), Seconds(0));
        return true;
    }

    if ((src == GetNetDevice()->GetAddress()) &&
        (dbrh.GetMode() == DBRH_DATA_GREEDY))
    {
        NS_LOG_DEBUG("AquaSimEDBRv2v2::Recv: address:" <<
            GetNetDevice()->GetAddress() << ": got the pkt I've sent");
        p = 0;
        return false;
    }

    if (dst == GetNetDevice()->GetAddress())
    {
        NS_LOG_DEBUG("AquaSimEDBRv2v2::Recv: address:" <<
            GetNetDevice()->GetAddress() << ": packet is delivered!");

        if (!SendUp(p))
            NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
        return true;
    }

    HandlePktForward(p);
    return true;
}

void AquaSimEDBRv2v2::HandlePktForward(Ptr<Packet> p)
{
    AquaSimHeader ash;
    DBRHeader dbrh;
    AquaSimPtTag ptag;
    p->RemoveHeader(ash);
    p->RemoveHeader(dbrh);
    p->RemovePacketTag(ptag);

    double delta;
    double delay = .0;

    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model == nullptr)
    {
        NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

    ash.SetNumForwards((ash.GetNumForwards()+1));
    ash.SetDirection(AquaSimHeader::DOWN);
    ptag.SetPacketType(AquaSimPtTag::PT_DBR);
    ash.SetSize(dbrh.Size() + EDBRv2_IP_HDR_LEN);
    ash.SetNextHop(AquaSimAddress::GetBroadcast());

    switch (dbrh.GetMode())
    {
    case DBRH_DATA_GREEDY:
        delta = model->GetPosition().z - dbrh.GetDepth();

        if (delta < EDBRv2_DEPTH_THRESHOLD)
        {
            p->AddHeader(ash);
            p->AddPacketTag(ptag);
            m_pq.purge(p);
            p = 0;
            return;
        }

        NS_LOG_DEBUG("[" << GetNetDevice()->GetAddress() << "]: z=" <<
            model->GetPosition().z << ", depth=" << dbrh.GetDepth() <<
            ", delta=" << delta);

        dbrh.SetDepth(model->GetPosition().z);

        // ========== EDBRv2: Energy-Aware Delay Calculation ==========
        delay = CalculateEnergyAwareDelay(delta);
        // ========== END EDBRv2 ==========

        break;
    case DBRH_DATA_RECOVER:
        if (dbrh.GetNHops() <= 0)
        {
            NS_LOG_DEBUG("AquaSimEDBRv2v2::Recv: address:" <<
                GetNetDevice()->GetAddress() << ": drops pkt! (nhops < 0)");
            p = 0;
            return;
        }
        dbrh.SetNHops((dbrh.GetNHops()-1));
        break;
    default:
        NS_LOG_WARN("AquaSimEDBRv2v2::Recv: Unknown data type!");
        return;
    }

    dbrh.SetOwner(dbrh.GetPrevHop());
    dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

    NS_LOG_DEBUG("[" << GetNetDevice()->GetAddress() << "]: delay " <<
        delay << " before broadcasting!");

    if (m_pc == NULL) {
        NS_LOG_WARN("AquaSimEDBRv2v2::Recv: packet cache pointer is null!\n");
        exit(-1);
    }

    if (m_pc->AccessPacket(dbrh.GetPacketID()))
    {
        p = 0;
        return;
    }

    double expected_send_time = Simulator::Now().ToDouble(Time::S) + delay;

    p->AddHeader(dbrh);
    p->AddHeader(ash);
    p->AddPacketTag(ptag);
    QueueItemEdbrv2 *q = new QueueItemEdbrv2(p, expected_send_time);

    if (m_pq.empty())
    {
        m_pq.insert(q);
        m_latest = expected_send_time;
        if (m_sendTimer->IsRunning()) m_sendTimer->Cancel();
        m_sendTimer->Schedule(Seconds(delay));
    }
    else
    {
        if (m_pq.update(p, expected_send_time))
        {
            m_pq.insert(q);
            if (expected_send_time < m_latest)
            {
                m_latest = expected_send_time;
                if (m_sendTimer->IsRunning()) m_sendTimer->Cancel();
                m_sendTimer->Schedule(Seconds(delay));
            }
        }
    }
}
