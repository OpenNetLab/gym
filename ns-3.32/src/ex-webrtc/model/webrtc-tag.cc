#include "ns3/webrtc-tag.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "byte-order.h"

namespace ns3 {

/*xxppsstt
xx reserve
pp packet type, rtp or rtcp
ss  seq length
tt time length*/

NS_LOG_COMPONENT_DEFINE("WebrtcTag");

enum SequenceNumberLength:uint8_t {
    SEQ_1BYTE = 1,
    SEQ_2BYTE = 2,
    SEQ_3BYTE = 3,
    SEQ_4BYTE = 4,
};

enum SequenceNumberLengthFlags:uint8_t {
  FLAGS_1BYTE =  0,           // 00
  FLAGS_2BYTE =  1,           // 01
  FLAGS_3BYTE =  1 << 1,      // 10
  FLAGS_4BYTE =  1 << 1 | 1,  // 11
};

SequenceNumberLength ReadNumberLength(uint8_t flags){
    switch (flags & FLAGS_4BYTE) {
    case FLAGS_4BYTE:
        return SEQ_4BYTE;
    case FLAGS_3BYTE:
        return SEQ_3BYTE;
    case FLAGS_2BYTE:
        return SEQ_2BYTE;
    case FLAGS_1BYTE:
        return SEQ_1BYTE;
    default:
        return SEQ_1BYTE;
  }
}

uint8_t GetNumberLength(uint32_t value){
    uint64_t number = value;
    if(number<(UINT64_C(1)<<(SEQ_1BYTE*8))){
        return SEQ_1BYTE;
    }else if(value<(UINT64_C(1)<<(SEQ_2BYTE*8))){
        return SEQ_2BYTE;
    }else if(value<(UINT64_C(1)<<(SEQ_3BYTE*8))){
        return SEQ_3BYTE;
    }else if(value<(UINT64_C(1)<<(SEQ_4BYTE*8))){
        return SEQ_4BYTE;
    }
    return 0;
}

uint8_t GetNumberFlags(uint32_t value){
    uint8_t seq_length = GetNumberLength(value);
    NS_ASSERT(seq_length);
    switch(seq_length){
        case SEQ_1BYTE:
        return FLAGS_1BYTE;
        case SEQ_2BYTE:
        return FLAGS_2BYTE;
        case SEQ_3BYTE:
        return FLAGS_3BYTE;
        case SEQ_4BYTE:
        return FLAGS_4BYTE;
    }
}

const uint8_t kTimeShift  =  0;
const uint8_t kSequenceNumberShift  =  2;
const uint8_t kPacketTypeShift  =  4;

TypeId WebrtcTag::GetTypeId (void){
   static TypeId tid = TypeId ("ns3::WebrtcTag")
                    .SetParent<Tag>()
                    .AddConstructor<WebrtcTag>()
                    .AddAttribute ("Time",
                                    "time stamp",
                                    EmptyAttributeValue (),
                                    MakeUintegerAccessor (&WebrtcTag::GetSentTime),
                                    MakeUintegerChecker<uint32_t> ())
                    .AddAttribute ("Number",
                                    "packet number",
                                    EmptyAttributeValue (),
                                    MakeUintegerAccessor (&WebrtcTag::GetSeq),
                                    MakeUintegerChecker<uint32_t> ());

   return tid;
}

TypeId WebrtcTag::GetInstanceTypeId (void) const {
    return GetTypeId ();
}

uint32_t WebrtcTag::GetSerializedSize (void) const {
    return 1+GetNumberLength(m_seq)+GetNumberLength(m_sentTime);
}

void WebrtcTag::Serialize (TagBuffer i) const {
    uint8_t public_flag = m_type<<kPacketTypeShift;
    public_flag |= (GetNumberFlags(m_seq)<<kSequenceNumberShift);
    public_flag |= (GetNumberFlags(m_sentTime)<<kTimeShift);
    i.WriteU8(public_flag);
    uint32_t seq_length = GetNumberLength(m_seq);
    uint32_t time_length = GetNumberLength(m_sentTime);
    // a must operation, to big endian.
    uint32_t number = basic::HostToNet32(m_seq);
    uint32_t time = basic::HostToNet32(m_sentTime);
    WriteVarintNumber(i,reinterpret_cast<char*>(&number)+sizeof(number)-seq_length,seq_length);
    WriteVarintNumber(i,reinterpret_cast<char*>(&time)+sizeof(time)-time_length,time_length);
}

void WebrtcTag::Deserialize (TagBuffer i){
    uint8_t public_flag = i.ReadU8();
    m_type = (public_flag&0x30)>>kPacketTypeShift;
    uint32_t seq_length = ReadNumberLength(public_flag>>kSequenceNumberShift);
    uint32_t time_length = ReadNumberLength(public_flag>>kTimeShift);
    uint32_t number = 0;
    uint32_t time = 0;
    ReadVarintNumber(i,reinterpret_cast<char*>(&number)+sizeof(number)-seq_length,seq_length);
    ReadVarintNumber(i,reinterpret_cast<char*>(&time)+sizeof(time)-time_length,time_length);
    number = basic::NetToHost32(number);
    time = basic::NetToHost32(time);
    m_seq = number;
    m_sentTime = time;
}

void WebrtcTag::Print (std::ostream &os) const {

}

void WebrtcTag::SetData(PacketType type,uint32_t seq,uint32_t sent_ts){
    m_type = (uint8_t)type;
    m_seq = seq;
    m_sentTime =  sent_ts;
}

void WebrtcTag::GetData(PacketType &type,uint32_t &seq,uint32_t &sent_ts){
    type = (PacketType)m_type;
    seq = m_seq;
    sent_ts = m_sentTime;
}

void WebrtcTag::WriteVarintNumber(TagBuffer& i, const void *value,uint32_t size) const {
    NS_ASSERT(size <= 4);
    uint8_t buf[4];
    memcpy(buf,value,size);
    for(uint32_t index = 0;index<size;index++){
        i.WriteU8(buf[index]);
    }
}

void WebrtcTag::ReadVarintNumber(TagBuffer& i, void *value,uint32_t size){
    NS_ASSERT(size <= 4);
    uint8_t buf[4];
    for(uint32_t index = 0;index<size;index++){
        buf[index] = i.ReadU8();
    }
    memcpy(value,buf,size);
}
}
