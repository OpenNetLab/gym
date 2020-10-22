#pragma once
#include "ns3/tag.h"
namespace ns3{
class WebrtcTag:public Tag{
public:
    enum PacketType:uint8_t{
        RTP,
        RTCP,
    };

    static TypeId GetTypeId (void);
    virtual TypeId GetInstanceTypeId (void) const override;
    virtual uint32_t GetSerializedSize (void) const override;
    virtual void Serialize (TagBuffer i) const override;
    virtual void Deserialize (TagBuffer i) override;
    virtual void Print (std::ostream &os) const override; 
    void SetData(PacketType type,uint32_t seq,uint32_t sent_ts);
    void GetData(PacketType &type,uint32_t &seq,uint32_t &sent_ts);
    void SetPacketType(PacketType type){
        m_type=(uint8_t)type;
    }

    PacketType GetPacketType() const{
        return (PacketType)m_type;
    }
    void SetSeq(uint32_t seq){
        m_seq=seq;
    }
    uint32_t GetSeq() const{
        return m_seq;
    }
    void SetSentTime(uint32_t sent_ts){
        m_sentTime= sent_ts;
    }
    uint32_t GetSentTime() const {
        return m_sentTime;
    }

private:
    void WriteVarintNumber(TagBuffer& i, const void *value,uint32_t size) const;
    void ReadVarintNumber(TagBuffer& i, void *value,uint32_t size);
    
    uint8_t m_type{RTP};
    uint32_t m_seq{0};
    uint32_t m_sentTime{0};
};    
}