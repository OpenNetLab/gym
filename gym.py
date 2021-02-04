#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gym_process
import gym_connect

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
        stats = self.gym_conn.step(bandwidth_bps)
        if stats != None:
            return stats, False
        return [], True

    def __del__(self):
        if os.path.exists(gym_connect.__ZMQ_PATH__ + self.gym_id):
            os.remove(gym_connect.__ZMQ_PATH__ + self.gym_id)
