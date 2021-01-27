#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import os
import signal

__ROOT_PATH__ = os.path.dirname(os.path.abspath(__file__))
__GYM_PROCESS_PATH__ = os.path.join(__ROOT_PATH__, "target", "webrtc_test")


class GymProcess(object):
    def __init__(
        self,
        gym_id: str = "gym",
        trace_path: str = "",
        report_interval_ms: int = 60,
        duration_time_ms: int = 3000):
        process_args = [__GYM_PROCESS_PATH__, "--standalone_test_only=false"]
        process_args.append("--gym_id="+str(gym_id))
        if trace_path:
            process_args.append("--trace_path="+trace_path)
        if report_interval_ms:
            process_args.append("--report_interval_ms="+str(report_interval_ms))
        if duration_time_ms:
            process_args.append("--duration_time_ms="+str(duration_time_ms))
        self.gym = subprocess.Popen(process_args)

    def wait(self, timeout = None):
        return self.gym.wait(timeout)

    def __del__(self):
        self.gym.send_signal(signal.SIGINT)
        self.gym.send_signal(signal.SIGKILL)
