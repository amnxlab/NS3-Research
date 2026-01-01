/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Vector Based Forwarding (EVBF) for AquaSim-NG
 *
 * This protocol extends the VBF algorithm by modifying the "desirableness factor" (alpha)
 * based on the node's residual energy. In VBF, alpha determines the holding time;
 * lower alpha means shorter delay. EVBF reduces alpha for nodes with high energy,
 * making them more likely to forward packets.
 *
 * Formula Used (Linear Penalty):
 *    α' = α * [1 + β * (1 - Eres/Einit)]
 *    - Increases alpha (delay) linearly as energy depletes.
 *
 * Final Delay Calculation:
 *    Th = √α' * Tdelay + (R - d) / V0
 *
 * Author: Research Implementation
 */

#include "aqua-sim-routing-evbf.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"

#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AquaSimEVBF");

// VBF Constants
#define WINDOW_SIZE 19
#define DELAY 0.5
#define JITTER 1.5

// ==================== EVBFPktHashTable ====================

EVBFPktHashTable::EVBFPktHashTable() {
    NS_LOG_FUNCTION(this);
    m_windowSize = WINDOW_SIZE;
}

EVBFPktHashTable::~EVBFPktHashTable()
{
    NS_LOG_FUNCTION(this);
    for (std::map<evbf_hash_entry, evbf_neighborhood*>::iterator it = m_htable.begin(); 
         it != m_htable.end(); ++it) {
        delete it->second;
    }
    m_htable.clear();
}

void EVBFPktHashTable::Reset()
{
    m_htable.clear();
}

evbf_neighborhood* EVBFPktHashTable::GetHash(AquaSimAddress senderAddr, unsigned int pk_num)
{
    evbf_hash_entry entry = std::make_pair(senderAddr, pk_num);
    std::map<evbf_hash_entry, evbf_neighborhood*>::iterator it;

    it = m_htable.find(entry);

    if (it == m_htable.end())
        return NULL;

    return it->second;
}

void EVBFPktHashTable::PutInHash(AquaSimAddress sAddr, unsigned int pkNum)
{
    evbf_neighborhood* hashPtr;
    evbf_hash_entry entry = std::make_pair(sAddr, pkNum);
    std::map<evbf_hash_entry, evbf_neighborhood*>::iterator it;

    int k = pkNum - m_windowSize;
    if (k > 0)
    {
        for (int i = 0; i < k; i++)
        {
            entry.second = i;
            it = m_htable.find(entry);
            if (it != m_htable.end())
            {
                hashPtr = it->second;
                delete hashPtr;
                m_htable.erase(it);
            }
        }
    }

    entry.second = pkNum;
    hashPtr = GetHash(sAddr, pkNum);
    if (hashPtr != NULL) {
        int m = hashPtr->number;
        if (m < MAX_NEIGHBOR) {
            hashPtr->number++;
            hashPtr->neighbor[m].x = 0;
            hashPtr->neighbor[m].y = 0;
            hashPtr->neighbor[m].z = 0;
        }
        return;
    }
    hashPtr = new evbf_neighborhood[1];
    hashPtr[0].number = 1;
    hashPtr[0].neighbor[0].x = 0;
    hashPtr[0].neighbor[0].y = 0;
    hashPtr[0].neighbor[0].z = 0;
    std::pair<evbf_hash_entry, evbf_neighborhood*> newPair;
    newPair.first = entry; newPair.second = hashPtr;
    if (m_htable.insert(newPair).second == false)
    {
        delete newPair.second;
    }
}

void EVBFPktHashTable::PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p)
{
    NS_LOG_DEBUG("PutinHash begin:" << sAddr << "," << pkNum << ",(" << p.x << "," << p.y << "," << p.z << ")");
    evbf_neighborhood* hashPtr;
    evbf_hash_entry entry = std::make_pair(sAddr, pkNum);
    std::map<evbf_hash_entry, evbf_neighborhood*>::iterator it;
    
    int k = pkNum - m_windowSize;
    if (k > 0)
    {
        for (int i = 0; i < k; i++)
        {
            entry.second = i;
            it = m_htable.find(entry);
            if (it != m_htable.end())
            {
                hashPtr = it->second;
                delete hashPtr;
                m_htable.erase(it);
            }
        }
    }

    entry.second = pkNum;
    hashPtr = GetHash(sAddr, pkNum);
    if (hashPtr != NULL)
    {
        int m = hashPtr->number;
        if (m < MAX_NEIGHBOR) {
            hashPtr->number++;
            hashPtr->neighbor[m].x = p.x;
            hashPtr->neighbor[m].y = p.y;
            hashPtr->neighbor[m].z = p.z;
        }
        return;
    }
    hashPtr = new evbf_neighborhood[1];
    hashPtr[0].number = 1;
    hashPtr[0].neighbor[0].x = p.x;
    hashPtr[0].neighbor[0].y = p.y;
    hashPtr[0].neighbor[0].z = p.z;

    std::pair<evbf_hash_entry, evbf_neighborhood*> newPair;
    newPair.first = entry; newPair.second = hashPtr;
    if (m_htable.insert(newPair).second == false)
    {
        delete newPair.second;
    }
}

// ==================== EVBFDataHashTable ====================

EVBFDataHashTable::EVBFDataHashTable() {
    NS_LOG_FUNCTION(this);
    Reset();
}

EVBFDataHashTable::~EVBFDataHashTable()
{
    NS_LOG_FUNCTION(this);
    Reset();
}

void EVBFDataHashTable::Reset()
{
    for (std::map<int*, int*>::iterator it = m_htable.begin(); it != m_htable.end(); ++it) {
        delete it->first;
        delete it->second;
    }
    m_htable.clear();
}

int* EVBFDataHashTable::GetHash(int* attr)
{
    std::map<int*, int*>::iterator it;
    it = m_htable.find(attr);
    if (it == m_htable.end())
        return NULL;
    return it->second;
}

void EVBFDataHashTable::PutInHash(int* attr)
{
    if (m_htable.count(attr) > 0)
        return;
    int *hashPtr = new int[1];
    hashPtr[0] = 1;
    m_htable.insert(std::pair<int*, int*>(attr, hashPtr));
}

// ==================== AquaSimEVBF ====================

NS_OBJECT_ENSURE_REGISTERED(AquaSimEVBF);

AquaSimEVBF::AquaSimEVBF()
{
    m_pkCount = 0;
    m_width = 0;
    m_counter = 0;
    m_priority = 1.5;
    m_enableRouting = 1;
    m_targetPos = Vector();
    m_rand = CreateObject<UniformRandomVariable>();
    
    // EVBF energy parameters
    m_beta = 0.5;
    m_Tdelay = 0.5;
    m_epsilon = 0.01;
}

TypeId AquaSimEVBF::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::AquaSimEVBF")
        .SetParent<AquaSimRouting>()
        .AddConstructor<AquaSimEVBF>()
        .AddAttribute("HopByHop", "Hop by hop EVBF setting. Default 0 is false.",
            IntegerValue(0),
            MakeIntegerAccessor(&AquaSimEVBF::m_hopByHop),
            MakeIntegerChecker<int>())
        .AddAttribute("EnableRouting", "Enable routing EVBF setting. Default 1 is true.",
            IntegerValue(1),
            MakeIntegerAccessor(&AquaSimEVBF::m_enableRouting),
            MakeIntegerChecker<int>())
        .AddAttribute("Width", "Width of EVBF. Default is 100.",
            DoubleValue(100),
            MakeDoubleAccessor(&AquaSimEVBF::m_width),
            MakeDoubleChecker<double>())
        .AddAttribute("TargetPos", "Position of target sink (x,y,z).",
            Vector3DValue(),
            MakeVector3DAccessor(&AquaSimEVBF::m_targetPos),
            MakeVector3DChecker())
        // ========== EVBF Energy-Aware Attributes ==========
        .AddAttribute("Beta", "Energy bias factor for linear penalty",
            DoubleValue(0.5),
            MakeDoubleAccessor(&AquaSimEVBF::m_beta),
            MakeDoubleChecker<double>())
        .AddAttribute("Tdelay", "Maximum delay constant for holding time",
            DoubleValue(0.5),
            MakeDoubleAccessor(&AquaSimEVBF::m_Tdelay),
            MakeDoubleChecker<double>())
        .AddAttribute("Epsilon", "Small constant to avoid division by zero",
            DoubleValue(0.01),
            MakeDoubleAccessor(&AquaSimEVBF::m_epsilon),
            MakeDoubleChecker<double>())

    ;
    return tid;
}

int64_t AquaSimEVBF::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rand->SetStream(stream);
    return 1;
}

void AquaSimEVBF::DoDispose()
{
    m_rand = 0;
    AquaSimRouting::DoDispose();
}

void AquaSimEVBF::SetTargetPos(Vector pos)
{
    m_targetPos = pos;
}

bool AquaSimEVBF::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this);
    AquaSimHeader ash;
    VBHeader vbh;
    AquaSimPtTag ptag;
    packet->RemoveHeader(ash);
    
    if (ash.GetNumForwards() <= 0) {
        ash.SetDirection(AquaSimHeader::DOWN);
        ash.SetNumForwards(1);
        ash.SetSAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        ash.SetDAddr(AquaSimAddress::ConvertFrom(dest));
        ash.SetTimeStamp(Simulator::Now());
        ash.SetUId(packet->GetUid());

        vbh.SetMessType(AS_DATA);
        vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
        vbh.SetTargetAddr(AquaSimAddress::ConvertFrom(dest));
        vbh.SetPkNum(packet->GetUid());

        Ptr<Object> sObject = GetNetDevice()->GetNode();
        Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel>();
        vbh.SetOriginalSource(sModel->GetPosition());
        vbh.SetExtraInfo_f(sModel->GetPosition());
        vbh.SetExtraInfo_t(m_targetPos);
        vbh.SetExtraInfo_o(sModel->GetPosition());

        packet->AddHeader(vbh);
    } else {
        packet->PeekHeader(vbh);
    }
    
    std::string msg;
    if (ash.GetDirection() == AquaSimHeader::DOWN)
    {
        msg = ") transmitting packet: ";
    }
    else
    {
        msg = ") receiving packet: ";
    }

    NS_LOG_INFO("Me(" << AquaSimAddress::ConvertFrom(m_device->GetAddress()).GetAsInt() << msg
                << ash.GetSize() << " bytes ; "
                << ash.GetTxTime().GetSeconds() << " sec. ; Dest: "
                << ash.GetDAddr().GetAsInt()
                << " ; Src: " << ash.GetSAddr().GetAsInt()
                << " ; Forwards: " << ash.GetNumForwards()
                << " ; ");
    packet->AddHeader(ash);

    if (!m_enableRouting) {
        if (vbh.GetMessType() != AS_DATA) {
            packet = 0;
            return false;
        }

        if (vbh.GetSenderAddr() == GetNetDevice()->GetAddress()) {
            ptag.SetPacketType(AquaSimPtTag::PT_UWVB);
            packet->ReplacePacketTag(ptag);
            MACprepare(packet);
            MACsend(packet, (m_rand->GetValue() * JITTER));
        }
        else if (vbh.GetTargetAddr() == GetNetDevice()->GetAddress()) {
            DataForSink(packet);
        }
        return true;
    }

    evbf_neighborhood *hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());

    if (hashPtr != NULL) {
        PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum(), vbh.GetExtraInfo().f);
        packet = 0;
        return false;
    }
    else {
        PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum(), vbh.GetExtraInfo().f);

        Ptr<Object> sObject = GetNetDevice()->GetNode();
        Ptr<MobilityModel> sModel = sObject->GetObject<MobilityModel>();
        Vector forwarder = vbh.GetExtraInfo().f;

        packet->RemoveHeader(ash);
        packet->RemoveHeader(vbh);
        Vector d = Vector(sModel->GetPosition().x - forwarder.x,
                          sModel->GetPosition().y - forwarder.y,
                          sModel->GetPosition().z - forwarder.z);
        vbh.SetExtraInfo_d(d);
        packet->AddHeader(vbh);
        packet->AddHeader(ash);

        ConsiderNew(packet);
    }

    return true;
}

void AquaSimEVBF::ConsiderNew(Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION(this);
    AquaSimHeader ash;
    VBHeader vbh;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    unsigned char msg_type = vbh.GetMessType();
    double l;

    evbf_neighborhood *hashPtr;
    AquaSimAddress from_nodeAddr;

    Ptr<Packet> gen_pkt;
    VBHeader gen_vbh;

    switch (msg_type) {
    case INTEREST:
        hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
        NS_LOG_INFO("ConsiderNew: INTEREST, hashptr #=" << hashPtr->number);

        from_nodeAddr = vbh.GetSenderAddr();

        if (GetNetDevice()->GetAddress() == from_nodeAddr) {
            MACprepare(pkt);
            MACsend(pkt, m_rand->GetValue() * JITTER);
        }
        else
        {
            if (IsTarget(pkt))
            {
                l = Advance(pkt);
                pkt->RemoveHeader(ash);
                pkt->RemoveHeader(vbh);
                vbh.SetMessType(SOURCE_DISCOVERY);
                pkt->AddHeader(vbh);
                pkt->AddHeader(ash);
                SetDelayTimer(pkt, l * JITTER);
            }
            else {
                l = Advance(pkt);
                if (IsCloseEnough(pkt)) {
                    MACprepare(pkt);
                    MACsend(pkt, m_rand->GetValue() * JITTER);
                }
                else {
                    pkt = 0;
                }
            }
        }
        return;

    case TARGET_DISCOVERY:
        if (GetNetDevice()->GetAddress() == vbh.GetTargetAddr()) {
            DataForSink(pkt);
            NS_LOG_INFO("AquaSimEVBF::ConsiderNew: target is " << GetNetDevice()->GetAddress());
        } else {
            pkt = 0;
        }
        return;

    case SOURCE_DISCOVERY:
        pkt = 0;
        return;

    case DATA_READY:
        from_nodeAddr = vbh.GetSenderAddr();
        if (GetNetDevice()->GetAddress() == from_nodeAddr) {
            MACprepare(pkt);
            MACsend(pkt, m_rand->GetValue() * JITTER);
            return;
        }
        if (GetNetDevice()->GetAddress() == vbh.GetTargetAddr())
        {
            NS_LOG_INFO("AquaSimEVBF::ConsiderNew: target is " << GetNetDevice()->GetAddress());
            DataForSink(pkt);
        }
        else {
            MACprepare(pkt);
            MACsend(pkt, m_rand->GetValue() * JITTER);
        }
        return;

    case AS_DATA:
        NS_LOG_INFO("AquaSimEVBF::ConsiderNew: data packet");
        from_nodeAddr = vbh.GetSenderAddr();

        if (GetNetDevice()->GetAddress() == from_nodeAddr) {
            MACprepare(pkt);
            MACsend(pkt, 0);
            return;
        }
        l = Advance(pkt);

        if (GetNetDevice()->GetAddress() == vbh.GetTargetAddr())
        {
            DataForSink(pkt);
        }
        else {
            if (IsCloseEnough(pkt)) {
                Vector *p1;
                p1 = new Vector[1];
                p1[0].x = vbh.GetExtraInfo().f.x;
                p1[0].y = vbh.GetExtraInfo().f.y;
                p1[0].z = vbh.GetExtraInfo().f.z;
                
                // ========== EVBF: Use energy-aware delay ==========
                double delay = CalculateEnergyAwareDelay(pkt, p1);
                // ========== END EVBF ==========
                
                delete p1;
                double d2 = (Distance(pkt) - m_device->GetPhy()->GetTransRange()) / ns3::SOUND_SPEED_IN_WATER;
                SetDelayTimer(pkt, (sqrt(delay) * DELAY + d2 * 2));
            }
            else {
                pkt = 0;
            }
        }
        return;

    default:
        pkt = 0;
        break;
    }
}

void AquaSimEVBF::Reset()
{
    PktTable.Reset();
}

void AquaSimEVBF::Terminate()
{
    NS_LOG_DEBUG("AquaSimEVBF::Terminate: Node=" << GetNetDevice()->GetAddress() <<
        ": remaining energy=" << GetNetDevice()->EnergyModel()->GetEnergy() <<
        ", initial energy=" << GetNetDevice()->EnergyModel()->GetInitialEnergy());
}

void AquaSimEVBF::StopSource()
{
}

Ptr<Packet> AquaSimEVBF::CreatePacket()
{
    NS_LOG_FUNCTION(this);

    Ptr<Packet> pkt = Create<Packet>();

    if (pkt == nullptr) return NULL;

    AquaSimHeader ash;
    VBHeader vbh;
    ash.SetSize(36);
    vbh.SetTs(Simulator::Now().ToDouble(Time::S));

    vbh.SetExtraInfo_o(GetNetDevice()->GetPosition());
    vbh.SetExtraInfo_f(GetNetDevice()->GetPosition());

    pkt->AddHeader(vbh);
    pkt->AddHeader(ash);
    return pkt;
}

Ptr<Packet> AquaSimEVBF::PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type)
{
    Ptr<Packet> pkt = Create<Packet>();
    VBHeader vbh;
    AquaSimHeader ash;

    vbh.SetMessType(msg_type);
    vbh.SetPkNum(m_pkCount);
    m_pkCount++;
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetDataType(dtype);
    vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetTs(Simulator::Now().ToDouble(Time::S));

    pkt->AddHeader(vbh);
    pkt->AddHeader(ash);
    return pkt;
}

void AquaSimEVBF::MACprepare(Ptr<Packet> pkt)
{
    AquaSimHeader ash;
    VBHeader vbh;
    pkt->RemoveHeader(ash);
    pkt->RemoveHeader(vbh);

    vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));

    ash.SetErrorFlag(false);
    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetDirection(AquaSimHeader::DOWN);

    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    vbh.SetExtraInfo_f(model->GetPosition());

    pkt->AddHeader(vbh);
    pkt->AddHeader(ash);
}

void AquaSimEVBF::MACsend(Ptr<Packet> pkt, double delay)
{
    NS_LOG_INFO("MACsend: delay " << delay << " at time " << Simulator::Now().GetSeconds());
    AquaSimHeader ash;
    VBHeader vbh;
    pkt->RemoveHeader(ash);
    pkt->RemoveHeader(vbh);

    pkt->AddHeader(vbh);
    pkt->AddHeader(ash);

    Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this,
                        pkt, ash.GetNextHop(), Seconds(0));
}

void AquaSimEVBF::DataForSink(Ptr<Packet> pkt)
{
    if (!SendUp(pkt))
        NS_LOG_WARN("DataForSink: Something went wrong when passing packet up to dmux.");
}

void AquaSimEVBF::SetDelayTimer(Ptr<Packet> pkt, double c)
{
    NS_LOG_FUNCTION(this << c);
    if (c < 0) c = 0;
    Simulator::Schedule(Seconds(c), &AquaSimEVBF::Timeout, this, pkt);
}

void AquaSimEVBF::Timeout(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    unsigned char msg_type = vbh.GetMessType();
    evbf_neighborhood *hashPtr;

    switch (msg_type) {
    case AS_DATA:
        hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
        if (hashPtr != NULL) {
            int num_neighbor = hashPtr->number;
            if (num_neighbor != 1) {
                if (num_neighbor == MAX_NEIGHBOR) {
                    pkt = 0;
                    return;
                }
                else
                {
                    int i = 0;
                    Vector* tp;
                    tp = new Vector[1];

                    tp[0].x = hashPtr->neighbor[i].x;
                    tp[0].y = hashPtr->neighbor[i].y;
                    tp[0].z = hashPtr->neighbor[i].z;
                    double tdelay = CalculateEnergyAwareDelay(pkt, tp);
                    i++;
                    double c = 1;
                    while (i < num_neighbor) {
                        c = c * 2;
                        tp[0].x = hashPtr->neighbor[i].x;
                        tp[0].y = hashPtr->neighbor[i].y;
                        tp[0].z = hashPtr->neighbor[i].z;
                        double t2delay = CalculateEnergyAwareDelay(pkt, tp);
                        if (t2delay < tdelay)
                            tdelay = t2delay;
                        i++;
                    }

                    delete tp;
                    if (tdelay <= (m_priority / c)) {
                        MACprepare(pkt);
                        MACsend(pkt, 0);
                    }
                    else {
                        pkt = 0;
                    }
                }
            }
            else {
                Vector* tp;
                tp = new Vector[1];
                tp[0].x = vbh.GetExtraInfo().f.x;
                tp[0].y = vbh.GetExtraInfo().f.y;
                tp[0].z = vbh.GetExtraInfo().f.z;
                double delay = CalculateEnergyAwareDelay(pkt, tp);

                delete tp;
                if (delay <= m_priority) {
                    MACprepare(pkt);
                    MACsend(pkt, 0);
                }
                else {
                    pkt = 0;
                }
                return;
            }
        }
        break;
    default:
        pkt = 0;
        break;
    }
}

void AquaSimEVBF::CalculatePosition(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    double fx = vbh.GetExtraInfo().f.x;
    double fy = vbh.GetExtraInfo().f.y;
    double fz = vbh.GetExtraInfo().f.z;

    double dx = vbh.GetExtraInfo().d.x;
    double dy = vbh.GetExtraInfo().d.y;
    double dz = vbh.GetExtraInfo().d.z;

    GetNetDevice()->CX() = fx + dx;
    GetNetDevice()->CY() = fy + dy;
    GetNetDevice()->CZ() = fz + dz;
}

/**
 * Calculate energy-aware delay based on VBF desirableness and residual energy.
 * 
 * Form 1 (Linear Penalty): α' = α * [1 + β * (1 - Eres/Einit)]
 * Form 2 (Inverse Scaling): α' = α / (Eres/Einit + ε)
 * 
 * Alpha is the VBF desirableness factor
 * Higher energy nodes get smaller alpha' -> smaller delay -> forward earlier
 */
double AquaSimEVBF::CalculateEnergyAwareDelay(Ptr<Packet> pkt, Vector* p1)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    
    double fx = p1->x;
    double fy = p1->y;
    double fz = p1->z;

    Vector pos = GetNetDevice()->GetPosition();
    double dx = pos.x - fx;
    double dy = pos.y - fy;
    double dz = pos.z - fz;

    double tx = vbh.GetExtraInfo().t.x;
    double ty = vbh.GetExtraInfo().t.y;
    double tz = vbh.GetExtraInfo().t.z;

    double dtx = tx - fx;
    double dty = ty - fy;
    double dtz = tz - fz;

    double dp = dx * dtx + dy * dty + dz * dtz;

    double p = Projection(pkt);
    double d = sqrt((dx * dx) + (dy * dy) + (dz * dz));
    double l = sqrt((dtx * dtx) + (dty * dty) + (dtz * dtz));
    double cos_theta;
    if (d == 0 || l == 0) cos_theta = 0;
    else cos_theta = dp / (d * l);
    
    double range = m_device->GetPhy()->GetTransRange();
    
    // Original VBF alpha (desirableness factor)
    double alpha = (p / m_width) + ((range - d * cos_theta) / range);
    
    // ========== EVBF: Energy-aware modification ==========
    double Eres = GetNetDevice()->EnergyModel()->GetEnergy();
    double Einit = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    
    double energyRatio = (Einit > 0) ? (Eres / Einit) : 0.0;
    if (energyRatio > 1.0) energyRatio = 1.0;
    if (energyRatio < 0.0) energyRatio = 0.0;
    
    // Form 1: α' = α * [1 + β * (1 - Eres/Einit)]
    // Linear energy penalty - low energy nodes get higher delay
    double alphaPrime = alpha * (1.0 + m_beta * (1.0 - energyRatio));

    // ========== END EVBF ==========
    
    NS_LOG_DEBUG("CalculateEnergyAwareDelay(" << GetNetDevice()->GetAddress() 
        << ") projection is " << p 
        << ", cos is " << cos_theta 
        << ", d is " << d 
        << ", alpha is " << alpha
        << ", energyRatio is " << energyRatio
        << ", alphaPrime is " << alphaPrime);
    
    return alphaPrime;
}

double AquaSimEVBF::CalculateDelay(Ptr<Packet> pkt, Vector* p1)
{
    // Wrapper that calls energy-aware version
    return CalculateEnergyAwareDelay(pkt, p1);
}

double AquaSimEVBF::Distance(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    double tx = vbh.GetExtraInfo().f.x;
    double ty = vbh.GetExtraInfo().f.y;
    double tz = vbh.GetExtraInfo().f.z;

    Vector pos = GetNetDevice()->GetPosition();
    return sqrt((tx - pos.x) * (tx - pos.x) + (ty - pos.y) * (ty - pos.y) + (tz - pos.z) * (tz - pos.z));
}

double AquaSimEVBF::Advance(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    
    double ox = vbh.GetExtraInfo().o.x;
    double oy = vbh.GetExtraInfo().o.y;
    double oz = vbh.GetExtraInfo().o.z;

    double tx = vbh.GetExtraInfo().t.x;
    double ty = vbh.GetExtraInfo().t.y;
    double tz = vbh.GetExtraInfo().t.z;

    Vector pos = GetNetDevice()->GetPosition();
    double cx = pos.x;
    double cy = pos.y;
    double cz = pos.z;

    double delay = sqrt((ox - tx) * (ox - tx) + (oy - ty) * (oy - ty) + (oz - tz) * (oz - tz)) -
                   sqrt((cx - tx) * (cx - tx) + (cy - ty) * (cy - ty) + (cz - tz) * (cz - tz));
    return delay;
}

double AquaSimEVBF::Projection(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    
    double ox = vbh.GetExtraInfo().o.x;
    double oy = vbh.GetExtraInfo().o.y;
    double oz = vbh.GetExtraInfo().o.z;

    double tx = vbh.GetExtraInfo().t.x;
    double ty = vbh.GetExtraInfo().t.y;
    double tz = vbh.GetExtraInfo().t.z;

    Vector pos = GetNetDevice()->GetPosition();
    double cx = pos.x;
    double cy = pos.y;
    double cz = pos.z;

    double l = sqrt((tx - ox) * (tx - ox) + (ty - oy) * (ty - oy) + (tz - oz) * (tz - oz));
    double advance = Advance(pkt);

    double x0 = ox + advance * (tx - ox) / l;
    double y0 = oy + advance * (ty - oy) / l;
    double z0 = oz + advance * (tz - oz) / l;

    return sqrt((cx - x0) * (cx - x0) + (cy - y0) * (cy - y0) + (cz - z0) * (cz - z0));
}

void AquaSimEVBF::SetMeasureTimer(Ptr<Packet> pkt, double c)
{
}

bool AquaSimEVBF::IsTarget(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);
    
    if (vbh.GetTargetAddr() == GetNetDevice()->GetAddress())
        return true;
    return false;
}

bool AquaSimEVBF::IsCloseEnough(Ptr<Packet> pkt)
{
    VBHeader vbh;
    AquaSimHeader ash;
    pkt->RemoveHeader(ash);
    pkt->PeekHeader(vbh);
    pkt->AddHeader(ash);

    double range = Projection(pkt);
    if (range <= m_width)
        return true;
    return false;
}
