/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Depth Based Routing v4 (EDBRv4) for AquaSim-NG
 *
 * INVERSE SCALING formula with SNR:
 *    Th = Tmin + k * (1 - Δd/Dmax) / ((Eres/Einit + εE) * (Qsnr + εS))
 *
 * For LINEAR SCALING, use EDBRv3 instead.
 *
 * Author: Eng. Ahmed Amin
 * Website: https://www.amnxlab.site/
 * Github: https://github.com/amnxlab
 */

#include "aqua-sim-routing-edbrv4.h"
#include "aqua-sim-header.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-pt-tag.h"

#include "ns3/ipv4-header.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimEDBRv4");

#define EDBRV4_MAX_DELAY      0.2
#define EDBRV4_MAX_RANGE      100
#define EDBRV4_MIN_BACKOFF    0.0
#define EDBRV4_MAX_HOPS       3
#define EDBRV4_DEPTH_THRESHOLD 0.0
#define EDBRV4_SCALE          1.0

// ==================== Timer Implementations ====================

EDBRv4_BeaconTimer::~EDBRv4_BeaconTimer() { m_a = 0; delete m_a; }
void EDBRv4_BeaconTimer::Expire() { m_a->Beacon_Callback(); }
EDBRv4_SendingTimer::~EDBRv4_SendingTimer() { m_a = 0; delete m_a; }
void EDBRv4_SendingTimer::Expire() { m_a->Send_Callback(); }
QueueItemEdbrv4::~QueueItemEdbrv4() { m_p = 0; }

// ==================== EDBRv4PacketQueue ====================

void EDBRv4PacketQueue::insert(QueueItemEdbrv4 *q)
{
    std::deque<QueueItemEdbrv4*>::iterator iter = m_dq.begin();
    while (iter != m_dq.end()) {
        if ((*iter)->m_sendTime > q->m_sendTime) {
            m_dq.insert(iter, q);
            return;
        }
        iter++;
    }
    m_dq.push_back(q);
}

bool EDBRv4PacketQueue::update(Ptr<Packet> p, double t)
{
    DBRHeader dbrh; AquaSimHeader ash;
    p->RemoveHeader(ash); p->PeekHeader(dbrh); p->AddHeader(ash);
    uint32_t curID = dbrh.GetPacketID();
    std::deque<QueueItemEdbrv4*>::iterator iter = m_dq.begin();
    while (iter != m_dq.end()) {
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        if (dbrh.GetPacketID() == curID) {
            if ((*iter)->m_sendTime > t) { m_dq.erase(iter); return true; }
            else return false;
        }
    }
    return true;
}

bool EDBRv4PacketQueue::purge(Ptr<Packet> p)
{
    DBRHeader dbrh; AquaSimHeader ash;
    p->RemoveHeader(ash); p->PeekHeader(dbrh); p->AddHeader(ash);
    uint32_t curID = dbrh.GetPacketID();
    std::deque<QueueItemEdbrv4*>::iterator iter = m_dq.begin();
    while (iter != m_dq.end()) {
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        if (dbrh.GetPacketID() == curID) { m_dq.erase(iter); return true; }
        iter++;
    }
    return false;
}

void EDBRv4PacketQueue::dump() {
    std::deque<QueueItemEdbrv4*>::iterator iter = m_dq.begin();
    int i = 0;
    while (iter != m_dq.end()) {
        DBRHeader dbrh; AquaSimHeader ash;
        ((*iter)->m_p)->RemoveHeader(ash);
        ((*iter)->m_p)->PeekHeader(dbrh);
        ((*iter)->m_p)->AddHeader(ash);
        NS_LOG_INFO("EDBRv4PacketQueue[" << i << "] packetID " << dbrh.GetPacketID());
        iter++; i++;
    }
}

// ==================== EDBRv4NeighbTable ====================

EDBRv4NeighbTable::EDBRv4NeighbTable() {
    m_numEnts = 0; m_maxEnts = 100;
    m_tab = new EDBRv4NeighbEnt*[100];
    for (int i = 0; i < 100; i++) m_tab[i] = new EDBRv4NeighbEnt();
}

EDBRv4NeighbTable::~EDBRv4NeighbTable() {
    for (int i = 0; i < m_maxEnts; i++) delete m_tab[i];
    delete[] m_tab;
}

TypeId EDBRv4NeighbTable::GetTypeId() {
    static TypeId tid = TypeId("ns3::EDBRv4NeighbTable");
    return tid;
}

void EDBRv4NeighbTable::Dump(void) {
    for (int i = 0; i < m_numEnts; i++) {
        NS_LOG_INFO("EDBRv4NeighbTable[" << i << "]: " << m_tab[i]->m_netID);
    }
}

void EDBRv4NeighbTable::EntDelete(const EDBRv4NeighbEnt *ne) {
    int l = 0, r = m_numEnts - 1, m;
    while (l <= r) {
        m = l + (r - l)/2;
        if (m_tab[m]->m_netID < ne->m_netID) l = m + 1;
        else if (ne->m_netID < m_tab[m]->m_netID) r = m - 1;
        else break;
    }
    if (l > r) return;
    EDBRv4NeighbEnt *owslot = m_tab[m];
    int i = m + 1;
    while (i < m_numEnts) m_tab[i - 1] = m_tab[i+1];
    m_tab[m_numEnts-1] = owslot;
    m_numEnts--;
}

EDBRv4NeighbEnt* EDBRv4NeighbTable::EntAdd(const EDBRv4NeighbEnt *ne) {
    for (int i = 0; i < m_numEnts; i++)
        if (m_tab[i]->m_netID == ne->m_netID) {
            m_tab[i]->m_location = ne->m_location;
            return m_tab[i];
        }
    if (m_numEnts == m_maxEnts) {
        EDBRv4NeighbEnt **tmp = m_tab;
        m_maxEnts *= 2;
        m_tab = new EDBRv4NeighbEnt*[m_maxEnts];
        bcopy(tmp, m_tab, m_numEnts*sizeof(EDBRv4NeighbEnt*));
        for (int i = m_numEnts; i < m_maxEnts; i++) m_tab[i] = new EDBRv4NeighbEnt();
        delete[] tmp;
    }
    m_tab[m_numEnts]->m_netID = ne->m_netID;
    m_tab[m_numEnts]->m_location = ne->m_location;
    m_numEnts++;
    return m_tab[m_numEnts-1];
}

void EDBRv4NeighbTable::UpdateRouteFlag(AquaSimAddress addr, int val) {
    for (int i = 0; i < m_numEnts; i++)
        if (m_tab[i]->m_netID == addr) { m_tab[i]->m_routeFlag = val; return; }
}

EDBRv4NeighbEnt* EDBRv4NeighbTable::EntFindShallowest(Vector location) {
    EDBRv4NeighbEnt *ne = 0;
    double t = location.z;
    for (int i = 0; i < m_numEnts; i++) {
        if (m_tab[i]->m_routeFlag == 1) return m_tab[i];
        if (m_tab[i]->m_location.z > t) { t = m_tab[i]->m_location.z; ne = m_tab[i]; }
    }
    return ne;
}

// ==================== EDBRv4PktCache ====================

EDBRv4PktCache::EDBRv4PktCache() { m_maxSize = 1500; m_size = 0; m_pCache = new int[1500]; }
EDBRv4PktCache::~EDBRv4PktCache() { delete[] m_pCache; }
TypeId EDBRv4PktCache::GetTypeId() { static TypeId tid = TypeId("ns3::EDBRv4PktCache"); return tid; }

int EDBRv4PktCache::AccessPacket(int p) {
    for (int i = 0; i < m_size; i++) {
        if (m_pCache[i] == p) {
            int tmp = p;
            for (int j = i; j < m_size - 1; j++) m_pCache[j] = m_pCache[j+1];
            m_pCache[m_size-1] = tmp;
            return 1;
        }
    }
    return 0;
}

void EDBRv4PktCache::AddPacket(int p) {
    if (m_size == m_maxSize) { NS_LOG_WARN("Cache full!"); return; }
    m_pCache[m_size++] = p;
}

void EDBRv4PktCache::DeletePacket(int p) {}
void EDBRv4PktCache::Dump(void) { for (int i = 0; i < m_size; i++) NS_LOG_INFO("[" << i << "]: " << m_pCache[i]); }

// ==================== AquaSimEDBRv4 ====================

NS_OBJECT_ENSURE_REGISTERED(AquaSimEDBRv4);

AquaSimEDBRv4::AquaSimEDBRv4() :
    m_bInt(EDBRV4_BEACON_INT), m_bDesync(EDBRV4_BEACON_DESYNC), m_pktCnt(0),
    m_Tmin(0.1), m_k(4.0), m_Dmax(300.0), 
    m_epsilonE(0.01), m_epsilonS(0.01),
    m_snrMax(100.0), m_snrMin(10.0)
{
    m_nTab = new EDBRv4NeighbTable();
    m_pc = new EDBRv4PktCache();
    m_rand = CreateObject<UniformRandomVariable>();
    m_beaconTimer = new EDBRv4_BeaconTimer(this);
    m_beaconTimer->SetFunction(&EDBRv4_BeaconTimer::Expire, m_beaconTimer);
    m_beaconTimer->Schedule(Seconds(m_rand->GetValue(0.0, m_bInt)));
    m_sendTimer = new EDBRv4_SendingTimer(this);
    m_sendTimer->SetFunction(&EDBRv4_SendingTimer::Expire, m_sendTimer);
}

AquaSimEDBRv4::~AquaSimEDBRv4() { delete m_nTab; delete m_pc; }

TypeId AquaSimEDBRv4::GetTypeId() {
    static TypeId tid = TypeId("ns3::AquaSimEDBRv4")
        .SetParent<AquaSimRouting>()
        .AddConstructor<AquaSimEDBRv4>()
        .AddAttribute("Tmin", "Minimum backoff time", DoubleValue(0.1),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_Tmin), MakeDoubleChecker<double>())
        .AddAttribute("K", "Scaling factor", DoubleValue(4.0),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_k), MakeDoubleChecker<double>())
        .AddAttribute("Dmax", "Max depth gain", DoubleValue(300.0),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_Dmax), MakeDoubleChecker<double>())
        .AddAttribute("EpsilonE", "Energy safeguard", DoubleValue(0.01),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_epsilonE), MakeDoubleChecker<double>())
        .AddAttribute("EpsilonS", "SNR safeguard", DoubleValue(0.01),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_epsilonS), MakeDoubleChecker<double>())
        .AddAttribute("SnrMax", "Max SNR", DoubleValue(100.0),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_snrMax), MakeDoubleChecker<double>())
        .AddAttribute("SnrMin", "Min SNR", DoubleValue(10.0),
            MakeDoubleAccessor(&AquaSimEDBRv4::m_snrMin), MakeDoubleChecker<double>())
    ;
    return tid;
}

int64_t AquaSimEDBRv4::AssignStreams(int64_t stream) {
    m_rand->SetStream(stream);
    return 1;
}

void AquaSimEDBRv4::DoDispose() { m_rand = 0; AquaSimRouting::DoDispose(); }

double AquaSimEDBRv4::GetReceivedSnr(Ptr<Packet> p) {
    AquaSimHeader ash; DBRHeader dbrh;
    Ptr<Packet> pkt = p->Copy();
    pkt->RemoveHeader(ash); pkt->PeekHeader(dbrh);
    Vector senderPos = dbrh.GetPosition();
    
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (!model) return m_snrMax;
    Vector myPos = model->GetPosition();
    
    double dx = myPos.x - senderPos.x, dy = myPos.y - senderPos.y, dz = myPos.z - senderPos.z;
    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    double range = GetNetDevice()->GetPhy()->GetTransRange();
    double distRatio = std::min(1.0, dist / range);
    double snr = m_snrMax * (1.0 - distRatio * distRatio);
    return std::max(0.0, snr);
}

double AquaSimEDBRv4::NormalizeSnr(double snr) {
    if (snr < m_snrMin) return 0.0;
    return std::min(1.0, std::max(0.0, snr / m_snrMax));
}

/**
 * INVERSE formula: Th = Tmin + k * (1 - Δd/Dmax) / ((Eres/Einit + εE) * (Qsnr + εS))
 */
double AquaSimEDBRv4::CalculateEnergySnrDelay(double deltaDepth, double snr) {
    double Eres = GetNetDevice()->EnergyModel()->GetEnergy();
    double Einit = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    double energyRatio = (Einit > 0) ? std::min(1.0, std::max(0.0, Eres / Einit)) : 0.0;
    double qSnr = NormalizeSnr(snr);
    double depthFactor = std::min(1.0, std::max(0.0, 1.0 - deltaDepth / m_Dmax));
    
    // INVERSE formula
    double denominator = (energyRatio + m_epsilonE) * (qSnr + m_epsilonS);
    double delay = m_Tmin + m_k * depthFactor / denominator;
    
    NS_LOG_DEBUG("EDBRv4::CalculateDelay (INVERSE): energyRatio=" << energyRatio 
        << " qSnr=" << qSnr << " delay=" << delay);
    return delay;
}

Ptr<Packet> AquaSimEDBRv4::MakeBeacon(void) {
    Ptr<Packet> p = Create<Packet>();
    AquaSimHeader ash; DBRHeader dbrh; AquaSimPtTag ptag;
    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(dbrh.Size() + EDBRV4_IP_HDR_LEN);
    ash.SetDAddr(AquaSimAddress::GetBroadcast());
    ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    ptag.SetPacketType(AquaSimPtTag::PT_DBR);
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (model) dbrh.SetPosition(model->GetPosition());
    dbrh.SetMode(DBRH_BEACON); dbrh.SetNHops(1);
    p->AddHeader(dbrh); p->AddHeader(ash); p->AddPacketTag(ptag);
    return p;
}

void AquaSimEDBRv4::SendBeacon(void) {
    Ptr<Packet> p = MakeBeacon();
    AquaSimHeader ash; p->RemoveHeader(ash);
    if (p && !ash.GetErrorFlag()) {
        if (ash.GetDirection() == AquaSimHeader::UP) ash.SetDirection(AquaSimHeader::DOWN);
        p->AddHeader(ash);
        Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown, this, p, AquaSimAddress::GetBroadcast(), Seconds(0));
    }
}

void AquaSimEDBRv4::Send_Callback(void) {
    if (m_pq.empty()) return;
    QueueItemEdbrv4 *q = m_pq.front(); m_pq.pop();
    Simulator::Schedule(Seconds(0), &AquaSimRouting::SendDown, this, q->m_p, AquaSimAddress::GetBroadcast(), Seconds(0));
    AquaSimHeader ash; DBRHeader dbrh;
    (q->m_p)->RemoveHeader(ash); (q->m_p)->PeekHeader(dbrh); (q->m_p)->AddHeader(ash);
    m_pc->AddPacket(dbrh.GetPacketID());
    if (!m_pq.empty()) {
        q = m_pq.front();
        m_latest = q->m_sendTime;
        m_sendTimer->Schedule(Seconds(m_latest - Simulator::Now().ToDouble(Time::S)));
    }
}

void AquaSimEDBRv4::Beacon_Callback(void) { return; }
void AquaSimEDBRv4::DeadNeighb_Callback(EDBRv4NeighbEnt *ne) { m_nTab->EntDelete(ne); }

void AquaSimEDBRv4::ForwardPacket(Ptr<Packet> p, int flag) {
    AquaSimHeader ash; DBRHeader dbrh; AquaSimPtTag ptag;
    p->RemoveHeader(ash); p->RemoveHeader(dbrh); p->RemovePacketTag(ptag);
    
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    ash.SetDirection(AquaSimHeader::DOWN);
    ash.SetSize(dbrh.Size() + EDBRV4_IP_HDR_LEN);
    ptag.SetPacketType(AquaSimPtTag::PT_DBR);
    
    double delay = 0.0;
    EDBRv4NeighbEnt *ne = m_nTab->EntFindShallowest(model->GetPosition());
    
    if (dbrh.GetMode() == DBRH_DATA_GREEDY) {
        if (ne) ash.SetNextHop(ne->m_netID);
        else {
            ash.SetNextHop(AquaSimAddress::GetBroadcast());
            dbrh.SetMode(DBRH_DATA_RECOVER);
            dbrh.SetPrevHop(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
            dbrh.SetNHops(EDBRV4_MAX_HOPS);
        }
    } else if (dbrh.GetMode() == DBRH_DATA_RECOVER) {
        if (m_pc->AccessPacket(dbrh.GetPacketID())) return;
        m_pc->AddPacket(dbrh.GetPacketID());
        if (!ne || ne->m_netID == dbrh.GetPrevHop()) {
            if (dbrh.GetNHops() <= 0) { p = 0; return; }
            ash.SetNextHop(AquaSimAddress::GetBroadcast());
            dbrh.SetNHops(dbrh.GetNHops()-1);
            delay = m_rand->GetValue() * EDBRV4_JITTER;
        } else {
            ash.SetNextHop(ne->m_netID);
            dbrh.SetMode(DBRH_DATA_GREEDY);
        }
    }
    
    p->AddHeader(dbrh); p->AddHeader(ash); p->AddPacketTag(ptag);
    Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this, p, ash.GetNextHop(), Seconds(0));
}

void AquaSimEDBRv4::BeaconIn(Ptr<Packet> p) {
    AquaSimHeader ash; DBRHeader dbrh;
    p->RemoveHeader(ash); p->PeekHeader(dbrh); p->AddHeader(ash);
    EDBRv4NeighbEnt *ne = new EDBRv4NeighbEnt();
    ne->m_location = dbrh.GetPosition();
    ne->m_netID = ash.GetSAddr();
    m_nTab->EntAdd(ne);
    delete ne; p = 0;
}

bool AquaSimEDBRv4::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber) {
    AquaSimHeader ash; DBRHeader dbrh; AquaSimPtTag ptag;
    p->RemoveHeader(ash);
    if (ash.GetNumForwards() <= 0) p->AddHeader(dbrh);
    p->PeekHeader(dbrh); p->AddHeader(ash); p->PeekPacketTag(ptag);
    
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (!model) return false;
    
    if (dbrh.GetMode() == DBRH_BEACON) { BeaconIn(p); return true; }
    
    if (dbrh.GetMode() == DBRH_DATA_GREEDY || dbrh.GetMode() == DBRH_DATA_RECOVER) {
        double deltaDepth = model->GetPosition().z - dbrh.GetPosition().z;
        double snr = GetReceivedSnr(p);
        
        if (snr < m_snrMin) { p = 0; return false; }
        
        if (deltaDepth > EDBRV4_DEPTH_THRESHOLD) {
            double delay = CalculateEnergySnrDelay(deltaDepth, snr);
            if (m_pc->AccessPacket(dbrh.GetPacketID())) { p = 0; return false; }
            
            double sendTime = Simulator::Now().ToDouble(Time::S) + delay;
            QueueItemEdbrv4 *qi = new QueueItemEdbrv4(p->Copy(), sendTime, snr);
            m_pq.insert(qi);
            
            if (!m_sendTimer->IsRunning()) {
                m_latest = sendTime;
                m_sendTimer->Schedule(Seconds(delay));
            } else if (sendTime < m_latest) {
                m_sendTimer->Cancel();
                m_latest = sendTime;
                m_sendTimer->Schedule(Seconds(delay));
            }
        }
    }
    
    AquaSimAddress dst = ash.GetDAddr();
    if (dst == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()) ||
        dst == AquaSimAddress::GetBroadcast()) {
        SendUp(p);
    }
    return true;
}
