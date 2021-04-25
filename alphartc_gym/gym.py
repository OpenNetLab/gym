#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym_process
from alphartc_gym import gym_connect

import os
import uuid

class Gym(object):
    def __init__(self, gym_id: str = None):
        if gym_id == None:
            gym_id = str(uuid.uuid4().hex)
        self.gym_id = gym_id
        self.gym_conn = None
        self.gym_process = None

    def reset(
        self,
        trace_path: str = "",
        report_interval_ms: int = 60,
        duration_time_ms: int = 3000):
        """ Reset gym environment, this function will destroy the old context
            and create a new one for next step.

            Parameters
            trace_path: a path to indicate a trace file with json format.
                The following is an exmaple of trace file
                {
                    "type": "video",
                    "downlink": {},
                    "uplink": {
                        "trace_pattern": [
                            {
                                "duration": 60000, # duration time
                                "capacity": 300,   # link bandwidth (kbps)
                                "loss": 0.1,       # loss rate (0.0~0.1)
                                "rtt" : 85,        # round trip delay (ms)
                                "jitter": 0,       # not supported in current version
                            },
                            ...
                        ]
                    }
                }
            report_interfal_ms: to indicate the report interval. It indicates
                the a step's duration, the unit is Millisecond.
            duration_time_ms: to indicate the duration for an epoch.
                If this value is None or 0, the duration will be caculated
                according to the trace file.
        """
        if self.gym_conn:
            del self.gym_conn
        if self.gym_process:
            del self.gym_process
        self.gym_process = gym_process.GymProcess(
            self.gym_id,
            trace_path,
            report_interval_ms,
            duration_time_ms)
        self.gym_conn = gym_connect.GymConnector(self.gym_id)

    def step(self, bandwidth_bps: int):
        """ Set a estimated bandwidth for Gym

            Parameters
            bandwidth_bps: the estimated bandwidth, the unit is bps

            Return value: a pair of stats list and done
                stats list: it's a list of packet stat, each stat is a python
                dict. The stats could be empty list.
                The following is an example for the stats
                [
                    {
                        'arrival_time_ms': 66113,
                        'header_length': 24,
                        'padding_length': 0,
                        'payload_size': 1389,
                        'payload_type': 126,
                        'send_time_ms': 60999,
                        'sequence_number': 54366,
                        'ssrc': 12648429
                    },
                    {
                        'arrival_time_ms': 66181,
                        'header_length': 24,
                        'padding_length': 0,
                        'payload_size': 1389,
                        'payload_type': 126,
                        'send_time_ms': 61069,
                        'sequence_number': 54411,
                        'ssrc': 12648429}
                ]
            done: A flag to indicate whether this epoch is finished.
                True means this epoch is done.
        """
        stats = self.gym_conn.step(bandwidth_bps)
        if stats != None:
            return stats, False
        return [], True

    def __del__(self):
        if os.path.exists(gym_connect.__ZMQ_PATH__ + self.gym_id):
            os.remove(gym_connect.__ZMQ_PATH__ + self.gym_id)
