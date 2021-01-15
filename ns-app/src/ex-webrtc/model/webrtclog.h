#pragma once
#include "rtc_base/logging.h"

namespace ns3 {

void init_webrtc_log();

class LogSinkConsole:public rtc::LogSink
{
public:
    LogSinkConsole(){}
    ~LogSinkConsole(){}
    void OnLogMessage(const std::string &message) override;
    bool Init();
};

}