#pragma once 
#include "rtc_base/logging.h"
#include "rtc_base/location.h"
#include "rtc_base/time_utils.h"
#include "test/frame_generator.h"
namespace ns3{
class MySender{
public:
	MySender();
	~MySender(){}
private:
	webrtc::test::SquareGenerator generator_;
};
void test_webrun();
}

