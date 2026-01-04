/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Energy-Aware Vector Based Forwarding v4 (EVBFv4) for AquaSim-NG
 *
 * INVERSE SCALING formula with SNR:
 *    α' = α / ((Eres/Einit + εE) * (Qsnr + εS))
 *    Th = √α' * Tdelay + (R - d) / V0
 *
 * For LINEAR SCALING, use EVBFv3 instead.
 *
 * Author: Eng. Ahmed Amin
 * Website: https://www.amnxlab.site/
 * Github: https://github.com/amnxlab
 */

#include "aqua-sim-routing-evbfv4.h"
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

NS_LOG_COMPONENT_DEFINE("AquaSimEVBFv4");

#define WINDOW_SIZE 19
#define DELAY 0.5
#define JITTER 1.5

// ==================== EVBFv4PktHashTable ====================

EVBFv4PktHashTable::EVBFv4PktHashTable() { m_windowSize = WINDOW_SIZE; }

EVBFv4PktHashTable::~EVBFv4PktHashTable() {
    for (auto it = m_htable.begin(); it != m_htable.end(); ++it) delete it->second;
    m_htable.clear();
}

void EVBFv4PktHashTable::Reset() { m_htable.clear(); }

evbfv4_neighborhood* EVBFv4PktHashTable::GetHash(AquaSimAddress senderAddr, unsigned int pk_num) {
    evbfv4_hash_entry entry = std::make_pair(senderAddr, pk_num);
    auto it = m_htable.find(entry);
    return (it == m_htable.end()) ? NULL : it->second;
}

void EVBFv4PktHashTable::PutInHash(AquaSimAddress sAddr, unsigned int pkNum) {
    evbfv4_hash_entry entry = std::make_pair(sAddr, pkNum);
    int k = pkNum - m_windowSize;
    if (k > 0) {
        for (int i = 0; i < k; i++) {
            entry.second = i;
            auto it = m_htable.find(entry);
            if (it != m_htable.end()) { delete it->second; m_htable.erase(it); }
        }
    }
    entry.second = pkNum;
    auto hashPtr = GetHash(sAddr, pkNum);
    if (hashPtr) {
        if (hashPtr->number < MAX_NEIGHBOR) {
            hashPtr->neighbor[hashPtr->number++] = Vector(0,0,0);
        }
        return;
    }
    hashPtr = new evbfv4_neighborhood[1];
    hashPtr[0].number = 1;
    hashPtr[0].neighbor[0] = Vector(0,0,0);
    m_htable.insert(std::make_pair(entry, hashPtr));
}

void EVBFv4PktHashTable::PutInHash(AquaSimAddress sAddr, unsigned int pkNum, Vector p) {
    evbfv4_hash_entry entry = std::make_pair(sAddr, pkNum);
    int k = pkNum - m_windowSize;
    if (k > 0) {
        for (int i = 0; i < k; i++) {
            entry.second = i;
            auto it = m_htable.find(entry);
            if (it != m_htable.end()) { delete it->second; m_htable.erase(it); }
        }
    }
    entry.second = pkNum;
    auto hashPtr = GetHash(sAddr, pkNum);
    if (hashPtr) {
        if (hashPtr->number < MAX_NEIGHBOR) hashPtr->neighbor[hashPtr->number++] = p;
        return;
    }
    hashPtr = new evbfv4_neighborhood[1];
    hashPtr[0].number = 1;
    hashPtr[0].neighbor[0] = p;
    m_htable.insert(std::make_pair(entry, hashPtr));
}

// ==================== AquaSimEVBFv4 ====================

NS_OBJECT_ENSURE_REGISTERED(AquaSimEVBFv4);

AquaSimEVBFv4::AquaSimEVBFv4() : m_pkCount(0), m_counter(0), m_priority(1.5), m_enableRouting(1),
    m_Tdelay(0.5), m_epsilonE(0.01), m_epsilonS(0.01), m_snrMax(100.0), m_snrMin(10.0), m_v0(1500.0)
{
    m_width = 0;
    m_targetPos = Vector();
    m_rand = CreateObject<UniformRandomVariable>();
}

TypeId AquaSimEVBFv4::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::AquaSimEVBFv4")
        .SetParent<AquaSimRouting>()
        .AddConstructor<AquaSimEVBFv4>()
        .AddAttribute("HopByHop", "Hop by hop", IntegerValue(0),
            MakeIntegerAccessor(&AquaSimEVBFv4::m_hopByHop), MakeIntegerChecker<int>())
        .AddAttribute("EnableRouting", "Enable routing", IntegerValue(1),
            MakeIntegerAccessor(&AquaSimEVBFv4::m_enableRouting), MakeIntegerChecker<int>())
        .AddAttribute("Width", "VBF pipe width", DoubleValue(100),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_width), MakeDoubleChecker<double>())
        .AddAttribute("TargetPos", "Target sink position",
            Vector3DValue(), MakeVector3DAccessor(&AquaSimEVBFv4::m_targetPos), MakeVector3DChecker())
        .AddAttribute("Tdelay", "Max delay constant", DoubleValue(0.5),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_Tdelay), MakeDoubleChecker<double>())
        .AddAttribute("EpsilonE", "Energy safeguard", DoubleValue(0.01),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_epsilonE), MakeDoubleChecker<double>())
        .AddAttribute("EpsilonS", "SNR safeguard", DoubleValue(0.01),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_epsilonS), MakeDoubleChecker<double>())
        .AddAttribute("SnrMax", "Max SNR", DoubleValue(100.0),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_snrMax), MakeDoubleChecker<double>())
        .AddAttribute("SnrMin", "Min SNR", DoubleValue(10.0),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_snrMin), MakeDoubleChecker<double>())
        .AddAttribute("V0", "Sound speed", DoubleValue(1500.0),
            MakeDoubleAccessor(&AquaSimEVBFv4::m_v0), MakeDoubleChecker<double>())
    ;
    return tid;
}

int64_t AquaSimEVBFv4::AssignStreams(int64_t stream) { m_rand->SetStream(stream); return 1; }
void AquaSimEVBFv4::DoDispose() { m_rand = 0; AquaSimRouting::DoDispose(); }
void AquaSimEVBFv4::SetTargetPos(Vector pos) { m_targetPos = pos; }

double AquaSimEVBFv4::GetReceivedSnr(Ptr<Packet> p, Vector* senderPos) {
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    if (!model) return m_snrMax;
    Vector myPos = model->GetPosition();
    double dx = myPos.x - senderPos->x, dy = myPos.y - senderPos->y, dz = myPos.z - senderPos->z;
    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    double range = m_device->GetPhy()->GetTransRange();
    double distRatio = std::min(1.0, dist / range);
    return std::max(0.0, m_snrMax * (1.0 - distRatio * distRatio));
}

double AquaSimEVBFv4::NormalizeSnr(double snr) {
    if (snr < m_snrMin) return 0.0;
    return std::min(1.0, std::max(0.0, snr / m_snrMax));
}

/**
 * INVERSE formula: α' = α / ((Eres/Einit + εE) * (Qsnr + εS))
 */
double AquaSimEVBFv4::CalculateEnergySnrDelay(Ptr<Packet> pkt, Vector* p1, double snr) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    
    Vector pos = GetNetDevice()->GetPosition();
    double fx = p1->x, fy = p1->y, fz = p1->z;
    double dx = pos.x - fx, dy = pos.y - fy, dz = pos.z - fz;
    double tx = vbh.GetExtraInfo().t.x, ty = vbh.GetExtraInfo().t.y, tz = vbh.GetExtraInfo().t.z;
    double dtx = tx - fx, dty = ty - fy, dtz = tz - fz;
    double dp = dx * dtx + dy * dty + dz * dtz;
    double p = Projection(pkt);
    double d = sqrt(dx*dx + dy*dy + dz*dz);
    double l = sqrt(dtx*dtx + dty*dty + dtz*dtz);
    double cos_theta = (d == 0 || l == 0) ? 0 : dp / (d * l);
    double range = m_device->GetPhy()->GetTransRange();
    
    double alpha = (p / m_width) + ((range - d * cos_theta) / range);
    
    // Get energy
    double Eres = GetNetDevice()->EnergyModel()->GetEnergy();
    double Einit = GetNetDevice()->EnergyModel()->GetInitialEnergy();
    double energyRatio = (Einit > 0) ? std::min(1.0, std::max(0.0, Eres / Einit)) : 0.0;
    double qSnr = NormalizeSnr(snr);
    
    // INVERSE formula: α' = α / ((Eres/Einit + εE) * (Qsnr + εS))
    double denominator = (energyRatio + m_epsilonE) * (qSnr + m_epsilonS);
    double alphaPrime = alpha / denominator;
    
    NS_LOG_DEBUG("EVBFv4::CalculateDelay (INVERSE): alpha=" << alpha 
        << " energyRatio=" << energyRatio << " qSnr=" << qSnr
        << " alphaPrime=" << alphaPrime);
    
    return alphaPrime;
}

double AquaSimEVBFv4::CalculateDelay(Ptr<Packet> pkt, Vector* p1) {
    double snr = GetReceivedSnr(pkt, p1);
    return CalculateEnergySnrDelay(pkt, p1, snr);
}

bool AquaSimEVBFv4::Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber) {
    AquaSimHeader ash; VBHeader vbh; AquaSimPtTag ptag;
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
        Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
        vbh.SetOriginalSource(model->GetPosition());
        vbh.SetExtraInfo_f(model->GetPosition());
        vbh.SetExtraInfo_t(m_targetPos);
        vbh.SetExtraInfo_o(model->GetPosition());
        packet->AddHeader(vbh);
    } else {
        packet->PeekHeader(vbh);
    }
    packet->AddHeader(ash);

    if (!m_enableRouting) {
        if (vbh.GetMessType() == AS_DATA) {
            if (vbh.GetSenderAddr() == GetNetDevice()->GetAddress()) {
                ptag.SetPacketType(AquaSimPtTag::PT_UWVB);
                packet->ReplacePacketTag(ptag);
                MACprepare(packet);
                MACsend(packet, m_rand->GetValue() * JITTER);
            } else if (vbh.GetTargetAddr() == GetNetDevice()->GetAddress()) {
                DataForSink(packet);
            }
        }
        return true;
    }

    auto hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
    if (hashPtr) {
        PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum(), vbh.GetExtraInfo().f);
        packet = 0;
        return false;
    }
    
    PktTable.PutInHash(vbh.GetSenderAddr(), vbh.GetPkNum(), vbh.GetExtraInfo().f);
    
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    Vector forwarder = vbh.GetExtraInfo().f;
    packet->RemoveHeader(ash);
    packet->RemoveHeader(vbh);
    vbh.SetExtraInfo_d(Vector(model->GetPosition().x - forwarder.x,
                               model->GetPosition().y - forwarder.y,
                               model->GetPosition().z - forwarder.z));
    packet->AddHeader(vbh);
    packet->AddHeader(ash);
    
    ConsiderNew(packet);
    return true;
}

void AquaSimEVBFv4::ConsiderNew(Ptr<Packet> pkt) {
    AquaSimHeader ash; VBHeader vbh;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    
    if (vbh.GetMessType() == AS_DATA) {
        if (vbh.GetSenderAddr() == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())) {
            MACprepare(pkt);
            MACsend(pkt, 0);
            return;
        }
        
        if (vbh.GetTargetAddr() == AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress())) {
            DataForSink(pkt);
            return;
        }
        
        if (IsCloseEnough(pkt)) {
            Vector *p1 = new Vector(vbh.GetExtraInfo().f);
            double snr = GetReceivedSnr(pkt, p1);
            
            if (snr < m_snrMin) {
                delete p1; pkt = 0;
                return;
            }
            
            double delay = CalculateEnergySnrDelay(pkt, p1, snr);
            delete p1;
            double d2 = (Distance(pkt) - m_device->GetPhy()->GetTransRange()) / m_v0;
            SetDelayTimer(pkt, sqrt(delay) * DELAY + d2 * 2);
        } else {
            pkt = 0;
        }
    }
}

void AquaSimEVBFv4::Reset() { PktTable.Reset(); }
void AquaSimEVBFv4::Terminate() {}

void AquaSimEVBFv4::SetDelayTimer(Ptr<Packet> pkt, double c) {
    if (c < 0) c = 0;
    Simulator::Schedule(Seconds(c), &AquaSimEVBFv4::Timeout, this, pkt);
}

void AquaSimEVBFv4::Timeout(Ptr<Packet> pkt) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    
    if (vbh.GetMessType() == AS_DATA) {
        auto hashPtr = PktTable.GetHash(vbh.GetSenderAddr(), vbh.GetPkNum());
        if (hashPtr && hashPtr->number > 1) {
            if (hashPtr->number == MAX_NEIGHBOR) { pkt = 0; return; }
            Vector *tp = new Vector(hashPtr->neighbor[0]);
            double snr = GetReceivedSnr(pkt, tp);
            double tdelay = CalculateEnergySnrDelay(pkt, tp, snr);
            for (int i = 1; i < hashPtr->number; i++) {
                tp->x = hashPtr->neighbor[i].x;
                tp->y = hashPtr->neighbor[i].y;
                tp->z = hashPtr->neighbor[i].z;
                snr = GetReceivedSnr(pkt, tp);
                double t2 = CalculateEnergySnrDelay(pkt, tp, snr);
                if (t2 < tdelay) tdelay = t2;
            }
            delete tp;
            double c = std::pow(2, hashPtr->number - 1);
            if (tdelay <= m_priority / c) {
                MACprepare(pkt); MACsend(pkt, 0);
            } else pkt = 0;
        } else {
            Vector *tp = new Vector(vbh.GetExtraInfo().f);
            double snr = GetReceivedSnr(pkt, tp);
            double delay = CalculateEnergySnrDelay(pkt, tp, snr);
            delete tp;
            if (delay <= m_priority) {
                MACprepare(pkt); MACsend(pkt, 0);
            } else pkt = 0;
        }
    }
}

double AquaSimEVBFv4::Distance(Ptr<Packet> pkt) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    Vector pos = GetNetDevice()->GetPosition();
    Vector f = vbh.GetExtraInfo().f;
    return sqrt(pow(pos.x - f.x, 2) + pow(pos.y - f.y, 2) + pow(pos.z - f.z, 2));
}

double AquaSimEVBFv4::Advance(Ptr<Packet> pkt) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    Vector o = vbh.GetExtraInfo().o, t = vbh.GetExtraInfo().t;
    Vector pos = GetNetDevice()->GetPosition();
    double fx = t.x - o.x, fy = t.y - o.y, fz = t.z - o.z;
    double f = sqrt(fx*fx + fy*fy + fz*fz);
    return ((pos.x - o.x)*fx + (pos.y - o.y)*fy + (pos.z - o.z)*fz) / f;
}

double AquaSimEVBFv4::Projection(Ptr<Packet> pkt) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    Vector o = vbh.GetExtraInfo().o, t = vbh.GetExtraInfo().t;
    Vector pos = GetNetDevice()->GetPosition();
    double x = pos.x - o.x, y = pos.y - o.y, z = pos.z - o.z;
    double fx = t.x - o.x, fy = t.y - o.y, fz = t.z - o.z;
    double f = sqrt(fx*fx + fy*fy + fz*fz);
    double p = (x*fx + y*fy + z*fz) / f;
    double d = sqrt(x*x + y*y + z*z);
    return (d == 0) ? 0 : sqrt(d*d - p*p);
}

bool AquaSimEVBFv4::IsTarget(Ptr<Packet> pkt) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    return vbh.GetTargetAddr() == GetNetDevice()->GetAddress();
}

bool AquaSimEVBFv4::IsCloseEnough(Ptr<Packet> pkt) {
    return Projection(pkt) < m_width;
}

void AquaSimEVBFv4::CalculatePosition(Ptr<Packet> pkt) {
    VBHeader vbh; AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->PeekHeader(vbh); pkt->AddHeader(ash);
    Vector f = vbh.GetExtraInfo().f, d = vbh.GetExtraInfo().d;
    GetNetDevice()->CX() = f.x + d.x;
    GetNetDevice()->CY() = f.y + d.y;
    GetNetDevice()->CZ() = f.z + d.z;
}

Ptr<Packet> AquaSimEVBFv4::CreatePacket() {
    Ptr<Packet> pkt = Create<Packet>();
    AquaSimHeader ash; VBHeader vbh;
    ash.SetSize(36);
    vbh.SetTs(Simulator::Now().ToDouble(Time::S));
    vbh.SetExtraInfo_o(GetNetDevice()->GetPosition());
    vbh.SetExtraInfo_f(GetNetDevice()->GetPosition());
    pkt->AddHeader(vbh); pkt->AddHeader(ash);
    return pkt;
}

Ptr<Packet> AquaSimEVBFv4::PrepareMessage(unsigned int dtype, AquaSimAddress to_addr, int msg_type) {
    Ptr<Packet> pkt = Create<Packet>();
    VBHeader vbh; AquaSimHeader ash;
    vbh.SetMessType(msg_type);
    vbh.SetPkNum(m_pkCount++);
    vbh.SetSenderAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetDataType(dtype);
    vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    vbh.SetTs(Simulator::Now().ToDouble(Time::S));
    pkt->AddHeader(vbh); pkt->AddHeader(ash);
    return pkt;
}

void AquaSimEVBFv4::MACprepare(Ptr<Packet> pkt) {
    AquaSimHeader ash; VBHeader vbh;
    pkt->RemoveHeader(ash); pkt->RemoveHeader(vbh);
    vbh.SetForwardAddr(AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()));
    ash.SetErrorFlag(false);
    ash.SetNextHop(AquaSimAddress::GetBroadcast());
    ash.SetDirection(AquaSimHeader::DOWN);
    Ptr<MobilityModel> model = GetNetDevice()->GetNode()->GetObject<MobilityModel>();
    vbh.SetExtraInfo_f(model->GetPosition());
    pkt->AddHeader(vbh); pkt->AddHeader(ash);
}

void AquaSimEVBFv4::MACsend(Ptr<Packet> pkt, double delay) {
    AquaSimHeader ash;
    pkt->RemoveHeader(ash); pkt->AddHeader(ash);
    Simulator::Schedule(Seconds(delay), &AquaSimRouting::SendDown, this, pkt, ash.GetNextHop(), Seconds(0));
}

void AquaSimEVBFv4::DataForSink(Ptr<Packet> pkt) {
    if (!SendUp(pkt)) NS_LOG_WARN("DataForSink failed");
}
