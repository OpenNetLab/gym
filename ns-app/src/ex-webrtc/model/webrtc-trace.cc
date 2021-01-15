#include "webrtc-trace.h"
#include <unistd.h>
#include <memory.h>

namespace ns3{

WebrtcTrace::~WebrtcTrace(){
    Close();
}

void WebrtcTrace::Log(std::string &s,uint8_t enable){
    if(enable & E_WEBRTC_OWD){
        OpenTraceRttFile(s);
    }
    if(enable & E_WEBRTC_BW){
        OpenTraceBwFile(s);
    }
}

void WebrtcTrace::OnRtt(
    uint32_t now,
    int64_t rtt) {
    char line [256];
    memset(line,0,256);
    if(m_rtt.is_open()) {
        float time = float(now)/1000;
        sprintf (line, "%f %16ld",
                time, rtt);
        m_rtt <<line<<std::endl;
    }
}

void WebrtcTrace::OnBw(uint32_t now, uint32_t bps){
    char line [256];
    memset(line,0,256);
    if(m_bw.is_open()){
        float time=float(now)/1000;
        float kbps=float(bps)/1000;
        sprintf (line, "%f %16f",
                time, kbps);
        m_bw<<line<<std::endl;
    }
}

void WebrtcTrace::OpenTraceRttFile(std::string &name) {
    char buf[FILENAME_MAX];
    memset(buf, 0, FILENAME_MAX);
    std::string path = std::string(getcwd(buf, FILENAME_MAX))
            + "/traces/"
            + name
            + "_rtt.txt";
    m_rtt.open(path.c_str(), std::fstream::out);
}

void WebrtcTrace::OpenTraceBwFile(std::string &name){
    char buf[FILENAME_MAX];
    memset(buf,0,FILENAME_MAX);
    std::string path = std::string(getcwd(buf, FILENAME_MAX))
            + "/traces/"
            + name
            + "_bw.txt";
    m_bw.open(path.c_str(), std::fstream::out);
}

void WebrtcTrace::CloseTraceRttFile(){
    if(m_rtt.is_open()){
        m_rtt.close();
    }
}

void WebrtcTrace::CloseTraceBwFile(){
    if(m_bw.is_open()){
        m_bw.close();
    }
}

void WebrtcTrace::Close(){
    CloseTraceRttFile();
    CloseTraceBwFile();
}

}
