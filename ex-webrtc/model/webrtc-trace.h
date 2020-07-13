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
    void OnBW(uint32_t now,uint32_t bps);
	void OnOwd(uint32_t now,uint32_t seq,uint32_t owd);
private:
	void Close();
	void OpenTraceOwdFile(std::string &name);
    void OpenTraceBwFile(std::string &name);
    void CloseTraceOwdFile();
    void CloseTraceBwFile();
	std::fstream m_owd;
	std::fstream m_bw;
};
}
