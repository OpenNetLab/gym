#include "webrtc-trace.h"
#include <unistd.h>
#include <memory.h>
namespace ns3{
WebrtcTrace::~WebrtcTrace(){
	Close();
}
void WebrtcTrace::Log(std::string &s,uint8_t enable){
    if(enable&E_WEBRTC_OWD){
        OpenTraceOwdFile(s);
    }
    if(enable&E_WEBRTC_BW){
        OpenTraceBwFile(s);
    }
}
void WebrtcTrace::OnOwd(uint32_t now,uint32_t seq,uint32_t owd){
	char line [256];
	memset(line,0,256);
	if(m_owd.is_open()){
		float time=float(now)/1000;
		sprintf (line, "%f %16d %16d",
				time,seq,owd);
		m_owd<<line<<std::endl;
	}
}
void WebrtcTrace::OnBW(uint32_t now,uint32_t bps){
	char line [256];
	memset(line,0,256);
	if(m_bw.is_open()){
		float time=float(now)/1000;
        float kbps=float(bps)/1000;
		sprintf (line, "%f %16f",
				time,kbps);
		m_bw<<line<<std::endl;
	}    
}
void WebrtcTrace::OpenTraceOwdFile(std::string &name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_owd.txt";
	m_owd.open(path.c_str(), std::fstream::out);
}
void WebrtcTrace::OpenTraceBwFile(std::string &name){
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
	std::string path = std::string (getcwd(buf, FILENAME_MAX)) + "/traces/"
			+name+"_bw.txt";
	m_bw.open(path.c_str(), std::fstream::out);
}
void WebrtcTrace::CloseTraceOwdFile(){
	if(m_owd.is_open()){
		m_owd.close();
	}
}
void WebrtcTrace::CloseTraceBwFile(){
	if(m_bw.is_open()){
		m_bw.close();
	}
}
void WebrtcTrace::Close(){
	CloseTraceOwdFile();
	CloseTraceBwFile();
}
}
