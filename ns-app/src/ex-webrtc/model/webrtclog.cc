#include "webrtclog.h"
#include <iostream>

namespace ns3
{

static  LogSinkConsole log;
static  bool log_init=false;

void LogSinkConsole::OnLogMessage(const std::string &message)
{
    std::cout<<message;
}

bool LogSinkConsole::Init()
{
    rtc::LogMessage::AddLogToStream(this,rtc::LS_INFO);
        return true;
}

void init_webrtc_log(){
    if(!log_init){
        log.Init();
        log_init=true;
    }
}
}