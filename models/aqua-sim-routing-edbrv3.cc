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

#include "aqua-sim-routing-edbrv3.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-pt-tag.h"

#include "ns3/ipv4-header.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimEDBRv3");

// Constants for delay calculation
#define EDBRV3_MAX_DELAY      0.2     // maximal propagation delay for one hop
#define EDBRV3_MAX_RANGE      100     // maximal transmission range
#define EDBRV3_MIN_BACKOFF    0.0     // minimal backoff time
#define EDBRV3_MAX_HOPS       3
#define EDBRV3_DEPTH_THRESHOLD 0.0
#define EDBRV3_SCALE          1.0

// ==================== Timer Implementations ====================

EDBRv3_BeaconTimer::~EDBRv3_BeaconTimer()
{
    m_a = 0;
    delete m_a;
}

void EDBRv3_BeaconTimer::Expire()
{
    m_a->Beacon_Callback();
}

EDBRv3_SendingTimer::~EDBRv3_SendingTimer()
{
    m_a = 0;
    delete m_a;
}

void EDBRv3_SendingTimer::Expire()
{
    m_a->Send_Callback();
}

// ==================== QueueItemEdbrv3 ====================

QueueItemEdbrv3::~QueueItemEdbrv3()
{
    m_p = 0;
}

// ==================== EDBRv3PacketQueue ====================

void EDBRv3PacketQueue::insert(QueueItemEdbrv3 *q)
{
    QueueItemEdbrv3 *tmp;
    std::deque<QueueItemEdbrv3*>::iterator iter;

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

bool EDBRv3PacketQueue::update(Ptr<Packet> p, double t)
{
    Ptr<Packet> pkt;
    uint32_t curID;
    std::deque<QueueItemEdbrv3*>::iterator iter;
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

bool EDBRv3PacketQueue::purge(Ptr<Packet> p)
{
    Ptr<Packet> pkt;
    uint32_t curID;
    std::deque<QueueItemEdbrv3*>::iterator iter;
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

void EDBRv3PacketQueue::dump()
{
    std::deque<QueueItemEdbrv3*>::iterator iter;
    DBRHeader dbrh;
    AquaSimHeader ash;
    int i = 0;

    iter = m_dq.begin();
    while (iter != m_dq.end())
    {
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        NS_LOG_INFO("EDBRv3PacketQueue::dump:[" << i << "] packetID " <<
            dbrh.GetPacketID() << ", send time " << (*iter)->m_sendTime);
        iter++;
        i++;
    }
}

// ==================== EDBRv3NeighbTable ====================

EDBRv3NeighbTable::EDBRv3NeighbTable()
{
    m_numEnts = 0;
    m_maxEnts = 100;
    m_tab = new EDBRv3NeighbEnt* [100];

    for (int i = 0; i < 100; i++)
        m_tab[i] = new EDBRv3NeighbEnt();
}

EDBRv3NeighbTable::~EDBRv3NeighbTable()
{
    for (int i = 0; i < m_maxEnts; i++)
        delete m_tab[i];
    delete[] m_tab;
}

TypeId EDBRv3NeighbTable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EDBRv3NeighbTable");
    return tid;
}

void EDBRv3NeighbTable::Dump(void)
{
    for (int i = 0; i < m_numEnts; i++)
    {
        NS_LOG_INFO("EDBRv3NeighbTable::dump: m_tab[" << i << "]: " << m_tab[i]->m_netID
            << " position(" << m_tab[i]->m_location.x << "," << m_tab[i]->m_location.y
            << "," << m_tab[i]->m_location.z << ")");
    }
}

void EDBRv3NeighbTable::EntDelete(const EDBRv3NeighbEnt *ne)
{
    int l, r, m;
    EDBRv3NeighbEnt *owslot;

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

EDBRv3NeighbEnt* EDBRv3NeighbTable::EntAdd(const EDBRv3NeighbEnt *ne)
{
    EDBRv3NeighbEnt *pe;
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
        EDBRv3NeighbEnt **tmp = m_tab;
        m_maxEnts *= 2;
        m_tab = new EDBRv3NeighbEnt* [m_maxEnts];
        bcopy(tmp, m_tab, m_numEnts*sizeof(EDBRv3NeighbEnt *));

        for (i = m_numEnts; i < m_maxEnts; i++)
            m_tab[i] = new EDBRv3NeighbEnt();

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

void EDBRv3NeighbTable::UpdateRouteFlag(AquaSimAddress addr, int val)
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

EDBRv3NeighbEnt* EDBRv3NeighbTable::EntFindShallowest(Vector location)
{
    EDBRv3NeighbEnt *ne = 0;
    double t = location.z;

    NS_LOG_DEBUG("EDBRv3NeighbTable::EntFindShallowest: location=(" <<
        location.x << "," << location.y << "," << location.z <<
        ") has " << m_numEnts << " neighbors");

    for (int i = 0; i < m_numEnts; i++)
    {
        NS_LOG_DEBUG("EDBRv3NeighbTable::EntFindShallowest: [" << m_tab[i]->m_netID <<
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

// ==================== EDBRv3PktCache ====================

EDBRv3PktCache::EDBRv3PktCache()
{
    m_maxSize = 1500;
    m_size = 0;
    m_pCache = new int[1500];
}

EDBRv3PktCache::~EDBRv3PktCache()
{
    delete[] m_pCache;
}

TypeId EDBRv3PktCache::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EDBRv3PktCache");
    return tid;
}

int EDBRv3PktCache::AccessPacket(int p)
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

void EDBRv3PktCache::AddPacket(int p)
{
    if (m_size == m_maxSize) {
        NS_LOG_WARN("Cache is full!");
        return;
    }
    m_pCache[m_size] = p;
    m_size++;
}

void EDBRv3PktCache::DeletePacket(int p)
{
}

void EDBRv3PktCache::Dump(void)
{
    for (int i = 0; i < m_size; i++)
    {
        NS_LOG_INFO("[" << i << "]: " << m_pCache[i]);
    }
}

// ==================== AquaSimEDBRv3 ====================

NS_OBJECT_ENSURE_REGISTERED(AquaSimEDBRv3);

AquaSimEDBRv3::AquaSimEDBRv3() :
    m_bInt(EDBRV3_BEACON_INT), m_bDesync(EDBRV3_BEACON_DESYNC), m_pktCnt(0),
    m_Tmin(0.1), m_k(4.0), m_Dmax(300.0), 
    m_snrMax(100.0), m_snrMin(10.0)
{
    m_nTab = new EDBRv3NeighbTable();
    m_pc = new EDBRv3PktCache();
    m_rand = CreateObject<UniformRandomVariable>();

    m_beaconTimer = new EDBRv3_BeaconTimer(this);
    m_beaconTimer->SetFunction(&EDBRv3_BeaconTimer::Expire, m_beaconTimer);
    m_beaconTimer->Schedule(Seconds(m_rand->GetValue(0.0, m_bInt)));

    m_sendTimer = new EDBRv3_SendingTimer(this);
    m_sendTimer->SetFunction(&EDBRv3_SendingTimer::Expire, m_sendTimer);
}

AquaSimEDBRv3::~AquaSimEDBRv3()
{
    delete m_nTab;
    delete m_pc;
}

TypeId AquaSimEDBRv3::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AquaSimEDBRv3")
        .SetParent<AquaSimRouting>()
        .AddConstructor<AquaSimEDBRv3>()
        .AddAttribute("Tmin", "Minimum backoff time in seconds",
            DoubleValue(0.1),
            MakeDoubleAccessor(&AquaSimEDBRv3::m_Tmin),
            MakeDoubleChecker<double>())
        .AddAttribute("K", "Scaling factor for delay calculation",
            DoubleValue(4.0),
            MakeDoubleAccessor(&AquaSimEDBRv3::m_k),
            MakeDoubleChecker<double>())
        .AddAttribute("Dmax", "Maximum useful depth gain in meters",
            DoubleValue(300.0),
            MakeDoubleAccessor(&AquaSimEDBRv3::m_Dmax),
            MakeDoubleChecker<double>())
        .AddAttribute("SnrMax", "Maximum SNR for normalization",
            DoubleValue(100.0),
            MakeDoubleAccessor(&AquaSimEDBRv3::m_snrMax),
            MakeDoubleChecker<double>())
        .AddAttribute("SnrMin", "Minimum acceptable SNR threshold",
            DoubleValue(10.0),
            MakeDoubleAccessor(&AquaSimEDBRv3::m_snrMin),
            MakeDoubleChecker<double>())
    ;
    return tid;
}

int64_t AquaSimEDBRv3::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rand->SetStream(stream);
    return 1;
}

void AquaSimEDBRv3::DoDispose()
{
    m_rand = 0;
    AquaSimRouting::DoDispose();
}

/**
 * Get SNR from received packet using distance-based estimation.
 */
double AquaSimEDBRv3::GetReceivedSnr(Ptr<Packet> p)
{
    AquaSimHeader ash;
    DBRHeader dbrh;
    
    Ptr<Packet> pkt = p->Copy();
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(dbrh);
    
    // Get sender position from DBR header
    Vector senderPos = dbrh.GetPosition();
    
    // Get our position
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model == nullptr) {
        NS_LOG_WARN("MobilityModel not found, using default SNR");
        return m_snrMax;
    }
    Vector myPos = model->GetPosition();
    
    // Calculate distance
    double dx = myPos.x - senderPos.x;
    double dy = myPos.y - senderPos.y;
    double dz = myPos.z - senderPos.z;
    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Get transmission range
    double range = GetNetDevice()->GetPhy()->GetTransRange();
    
    // Estimate SNR: SNR decreases with distance squared in underwater
    double distRatio = dist / range;
    if (distRatio > 1.0) distRatio = 1.0;
    
    double estimatedSnr = m_snrMax * (1.0 - distRatio * distRatio);
    if (estimatedSnr < 0) estimatedSnr = 0;
    
    NS_LOG_DEBUG("AquaSimEDBRv3::GetReceivedSnr: distance=" << dist 
        << " range=" << range 
        << " estimatedSNR=" << estimatedSnr);
    
    return estimatedSnr;
}

/**
 * Normalize SNR to [0, 1] range
 */
double AquaSimEDBRv3::NormalizeSnr(double snr)
{
    if (snr < m_snrMin) {
        NS_LOG_DEBUG("SNR " << snr << " below minimum " << m_snrMin);
        return 0.0;
    }
    
    double qSnr = snr / m_snrMax;
    if (qSnr > 1.0) qSnr = 1.0;
    if (qSnr < 0.0) qSnr = 0.0;
    
    return qSnr;
}

/**
 * Calculate energy+SNR-aware delay using LINEAR formula.
 * 
 * Formula: Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit) * (1 - Qsnr)
 * 
 * Higher energy + higher SNR → smaller delay → forward earlier
 */
double AquaSimEDBRv3::CalculateEnergySnrDelay(double deltaDepth, double snr)
{
    // Get residual and initial energy
    double Eres = GetNetDevice()->EnergyModel()->GetEnergy();
    double Einit = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    
    // Calculate energy ratio (clamped to [0, 1])
    double energyRatio = (Einit > 0) ? (Eres / Einit) : 0.0;
    if (energyRatio > 1.0) energyRatio = 1.0;
    if (energyRatio < 0.0) energyRatio = 0.0;
    
    // Normalize SNR to [0, 1]
    double qSnr = NormalizeSnr(snr);
    
    // Calculate depth factor: (1 - Δd/Dmax)
    double depthFactor = 1.0 - (deltaDepth / m_Dmax);
    if (depthFactor < 0.0) depthFactor = 0.0;
    if (depthFactor > 1.0) depthFactor = 1.0;
    
    // LINEAR formula: Th = Tmin + k * (1 - Δd/Dmax) * (1 - Eres/Einit) * (1 - Qsnr)
    double energyPenalty = 1.0 - energyRatio;
    double snrPenalty = 1.0 - qSnr;
    double delay = m_Tmin + m_k * depthFactor * energyPenalty * snrPenalty;
    
    NS_LOG_DEBUG("AquaSimEDBRv3::CalculateEnergySnrDelay (LINEAR): "
        << "Eres=" << Eres << " Einit=" << Einit 
        << " SNR=" << snr << " qSnr=" << qSnr
        << " deltaDepth=" << deltaDepth 
        << " delay=" << delay);
    
    return delay;
}

// Construct beacon packet
Ptr<Packet> AquaSimEDBRv3::MakeBeacon(void)
{
    Ptr<Packet> p = Create<Packet>();
    AquaSimHeader ash;
    DBRHeader dbrh;
    AquaSimPtTag ptag;

    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(dbrh.Size() + EDBRV3_IP_HDR_LEN);
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

void AquaSimEDBRv3::SendBeacon(void)
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
        NS_LOG_WARN("AquaSimEDBRv3::SendBeacon: ERROR, can't make new beacon!");
    }
}

void AquaSimEDBRv3::Send_Callback(void)
{
    QueueItemEdbrv3 *q;
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

void AquaSimEDBRv3::Beacon_Callback(void)
{
    return;  // Beacons disabled for now
}

void AquaSimEDBRv3::DeadNeighb_Callback(EDBRv3NeighbEnt *ne)
{
    m_nTab->EntDelete(ne);
}

void AquaSimEDBRv3::ForwardPacket(Ptr<Packet> p, int flag)
{
    NS_LOG_FUNCTION(this);

    AquaSimHeader ash;
    DBRHeader dbrh;
    AquaSimPtTag ptag;
    p->RemoveHeader(ash);
    p->RemoveHeader(dbrh);
    p->RemovePacketTag(ptag);
    EDBRv3NeighbEnt *ne;

    double delay = 0.0;

    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model == nullptr)
    {
        NS_LOG_DEBUG("MobilityModel does not exist for device " << GetNetDevice());
    }

    NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: node=" <<
        GetNetDevice()->GetAddress() << " is forwarding to " << ash.GetDAddr());

    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(dbrh.Size() + EDBRV3_IP_HDR_LEN);
    ptag.SetPacketType(AquaSimPtTag::PT_DBR);

    switch (dbrh.GetMode())
    {
    case DBRH_DATA_GREEDY:
        ne = m_nTab->EntFindShallowest(model->GetPosition());
        if (ne)
        {
            ash.SetNextHop(ne->m_netID);
            NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: node=" <<
                GetNetDevice()->GetAddress() << " -> " << ne->m_netID);
        }
        else
        {
            NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: " <<
                GetNetDevice()->GetAddress() << " put pkt into recovery mode!");

            ash.SetNextHop(AquaSimAddress::GetBroadcast());
            dbrh.SetMode(DBRH_DATA_RECOVER);
            dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
            dbrh.SetOwner(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
            dbrh.SetNHops(EDBRV3_MAX_HOPS);
        }
        break;
    case DBRH_DATA_RECOVER:
        NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: " <<
            GetNetDevice()->GetAddress() << " forward in recovery algorithm!");

        if (m_pc->AccessPacket(dbrh.GetPacketID()))
        {
            NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: " <<
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
                NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: " <<
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
                delay = m_rand->GetValue() * EDBRV3_JITTER;
            }
        }
        else if (ne->m_netID != dbrh.GetPrevHop())
        {
            ash.SetNextHop(ne->m_netID);
            dbrh.SetMode(DBRH_DATA_GREEDY);
            NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: back to greedy:" <<
                GetNetDevice()->GetAddress() << " -> " << ne->m_netID);
        }
        else
        {
            NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: address:" <<
                GetNetDevice()->GetAddress() << ":dbrh->nhops=" << dbrh.GetNHops());

            if (dbrh.GetNHops() <= 0)
            {
                NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: " <<
                    GetNetDevice()->GetAddress() << " drops pkt! (nhops < 0)");
                p = 0;
                return;
            }
            else
            {
                NS_LOG_DEBUG("AquaSimEDBRv3::ForwardPacket: " <<
                    GetNetDevice()->GetAddress() << " is broadcasting!");
                ash.SetNextHop(AquaSimAddress::GetBroadcast());
                dbrh.SetMode(DBRH_DATA_RECOVER);
                dbrh.SetOwner(dbrh.GetPrevHop());
                dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
                dbrh.SetNHops((dbrh.GetNHops()-1));
                delay = m_rand->GetValue() * EDBRV3_JITTER;
            }
        }
        break;
    default:
        NS_LOG_WARN("AquaSimEDBRv3::ForwardPacket: Wrong! The packet can't be forwarded!");
        break;
    }

    NS_ASSERT(!ash.GetErrorFlag());
    p->AddHeader(dbrh);
    p->AddHeader(ash);
    p->AddPacketTag(ptag);
    Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this,
                        p, ash.GetNextHop(), Seconds(0));
}

void AquaSimEDBRv3::BeaconIn(Ptr<Packet> p)
{
    AquaSimHeader ash;
    DBRHeader dbrh;
    p->RemoveHeader(ash);
    p->PeekHeader(dbrh);
    p->AddHeader(ash);

    AquaSimAddress src = ash.GetSAddr();

    EDBRv3NeighbEnt *ne;
    ne = new EDBRv3NeighbEnt();

    NS_LOG_DEBUG("AquaSimEDBRv3::BeaconIn: " << GetNetDevice()->GetAddress() <<
        " got beacon from " << src);

    ne->m_location.x = dbrh.GetPosition().x;
    ne->m_location.y = dbrh.GetPosition().y;
    ne->m_location.z = dbrh.GetPosition().z;
    ne->m_netID = src;

    m_nTab->EntAdd(ne);
    delete ne;
    p = 0;
}

bool AquaSimEDBRv3::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
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
        BeaconIn(p);
        return true;
    }

    if (dbrh.GetMode() == DBRH_DATA_GREEDY || dbrh.GetMode() == DBRH_DATA_RECOVER)
    {
        // Calculate depth gain
        double senderDepth = dbrh.GetPosition().z;
        double myDepth = model->GetPosition().z;
        double deltaDepth = myDepth - senderDepth;

        // Get SNR for this packet
        double snr = GetReceivedSnr(p);
        
        // Check SNR threshold
        if (snr < m_snrMin) {
            NS_LOG_DEBUG("AquaSimEDBRv3::Recv: SNR " << snr << " below threshold " 
                << m_snrMin << ", dropping packet");
            p = 0;
            return false;
        }

        // Only forward if we're shallower (making progress toward surface)
        if (deltaDepth > EDBRV3_DEPTH_THRESHOLD)
        {
            // Calculate energy+SNR-aware delay using LINEAR formula
            double delay = CalculateEnergySnrDelay(deltaDepth, snr);

            NS_LOG_DEBUG("AquaSimEDBRv3::Recv: node " << GetNetDevice()->GetAddress()
                << " deltaDepth=" << deltaDepth
                << " SNR=" << snr
                << " holdTime=" << delay);

            // Check if already in cache
            if (m_pc->AccessPacket(dbrh.GetPacketID()))
            {
                NS_LOG_DEBUG("AquaSimEDBRv3::Recv: packet already in cache, drop");
                p = 0;
                return false;
            }

            // Schedule forwarding with calculated delay
            double sendTime = Simulator::Now().ToDouble(Time::S) + delay;
            
            QueueItemEdbrv3 *qi = new QueueItemEdbrv3(p->Copy(), sendTime, snr);
            m_pq.insert(qi);

            if (!m_sendTimer->IsRunning())
            {
                m_latest = sendTime;
                m_sendTimer->Schedule(Seconds(delay));
            }
            else if (sendTime < m_latest)
            {
                m_sendTimer->Cancel();
                m_latest = sendTime;
                m_sendTimer->Schedule(Seconds(delay));
            }
        }
        else
        {
            NS_LOG_DEBUG("AquaSimEDBRv3::Recv: no depth progress, not forwarding");
        }
    }

    // Check if this is the destination
    if (dst == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) ||
        dst == AquaSimAddress::GetBroadcast())
    {
        if (!SendUp(p))
        {
            NS_LOG_WARN("SendUp failed");
        }
        return true;
    }

    return true;
}
