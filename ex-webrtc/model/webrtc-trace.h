#pragma once
#include <iostream>
#include <fstream>
#include <string>
namespace ns3{
class WebrtcTrace{
public:
enum WebrtcTraceEnable:uint8_t{
    E_WEBRTC_OWD=0x01,
	E_WEBRTC_BW=0x02,
    E_WEBRTC_ALL=E_WEBRTC_OWD|E_WEBRTC_OWD
};
	WebrtcTrace(){};
	~WebrtcTrace();
    void Log(std::string &s,uint8_t enable);
    void OnBw(uint32_t now, uint32_t bps);
	void OnRtt(uint32_t now, int64_t rtt);
private:
	void Close();
	void OpenTraceRttFile(std::string &name);
    void OpenTraceBwFile(std::string &name);
    void CloseTraceRttFile();
    void CloseTraceBwFile();
	std::fstream m_rtt;
	std::fstream m_bw;
};
}
