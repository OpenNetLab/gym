#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gym
from utils.packet_info import PacketInfo
from utils.packet_record import PacketRecord
import os
import json


GROUND_TRUTH = 300000
ERROR = 0.2

def check_result(result, except_min, except_max, info):
    assert result >= except_min, \
        f"{info}={result}, less than minimal except {except_min}"
    assert result <= except_max, \
        f"{info}={result}, larger than maximal except {except_max}"

def gym_single_test(trace_path, load_ratio):
    g = gym.Gym("test_gym")
    g.reset(trace_path=trace_path, report_interval_ms=60, duration_time_ms=0)
    packet_record = PacketRecord()
    packet_record.reset()

    while True:
        packet_list, done = g.step(GROUND_TRUTH * load_ratio)
        if not done:
            for pkt in packet_list:
                packet_info = PacketInfo()
                packet_info.payload_type = pkt["payload_type"]
                packet_info.ssrc = pkt["ssrc"]
                packet_info.sequence_number = pkt["sequence_number"]
                packet_info.send_timestamp = pkt["send_time_ms"]
                packet_info.receive_timestamp = pkt["arrival_time_ms"]
                packet_info.padding_length = pkt["padding_length"]
                packet_info.header_length = pkt["header_length"]
                packet_info.payload_size = pkt["payload_size"]
                packet_info.bandwidth_prediction = GROUND_TRUTH * load_ratio
                packet_record.on_receive(packet_info)                
        else:
            break
    
    # Check whole receiving rate at the end
    receiving_rate = packet_record.calculate_receiving_rate()
    check_result(receiving_rate, GROUND_TRUTH * min(1, load_ratio) * (1 - ERROR),
                 GROUND_TRUTH * min(1, load_ratio * (1 + ERROR)), 'receiving_rate')

def test_gym_stability():
    g = gym.Gym("test_gym")
    trace_path = os.path.join(
        os.path.dirname(__file__),
        "data",
        "trace_300k.json")
    gym_single_test(trace_path, 0.75)
    gym_single_test(trace_path, 1.25)

if __name__ == "__main__":
    test_gym_stability()