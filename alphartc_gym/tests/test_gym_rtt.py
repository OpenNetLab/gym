#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym
import os, json


ERROR = 0.05


def get_gym_stats(trace_path, duration_time_ms=3000, bandwidth_bps=1000):
    total_stats = []
    g = gym.Gym()
    g.reset(trace_path=trace_path, duration_time_ms=duration_time_ms)

    while True:
        stats, done = g.step(bandwidth_bps)
        if not done:
            total_stats += stats
        else:
            return total_stats


def cal_pkt_transmit_time_ms(header_bytes, payload_bytes, bw_kps):
    return (header_bytes + payload_bytes) * 8 / bw_kps


def get_info_from_trace(trace_file):
    with open(trace_file, 'r') as f:
        return json.loads(f.read())


def get_abs_path_by_name(trace_name):
    trace_path = os.path.join(
            os.path.dirname(__file__),
            "data/rtt",
            trace_name)
    return trace_path


def single_rtt_available(trace_path):
    total_stats = get_gym_stats(trace_path)
    assert (total_stats)

    for stats in total_stats:
        assert (isinstance(stats, dict))


def single_rtt_persistence(trace_path):
    # get information from trace
    trace_data = get_info_from_trace(trace_path)
    trace_pattern = trace_data["uplink"]["trace_pattern"]
    one_way_delay = trace_pattern[0]["rtt"] / 2
    bandwidth_kbps = trace_pattern[0]["capacity"]

    total_stats = get_gym_stats(trace_path)
    assert (total_stats)

    for status in total_stats:
        transmission_delay = cal_pkt_transmit_time_ms(status["header_length"], 
                                                            status["payload_size"], bw_kps=bandwidth_kbps)
        predict_arrival_time_ms = status["send_time_ms"] + one_way_delay + transmission_delay
        assert abs(status["arrival_time_ms"] - predict_arrival_time_ms) <= ERROR * status["arrival_time_ms"]


def single_rtt_dynamically(trace_path, run_times=1):
    # get information from trace
    trace_data = get_info_from_trace(trace_path)
    trace_pattern = trace_data["uplink"]["trace_pattern"]
    one_way_delay_list = sorted([item["rtt"] / 2 for item in trace_pattern])
    rtt_sample_cnt = [0] * len(one_way_delay_list)
    bandwidth_kbps = trace_pattern[0]["capacity"]
    duration_time_ms = sum([item["duration"] for item in trace_pattern]) * run_times

    total_stats = get_gym_stats(trace_path, duration_time_ms=duration_time_ms)
    assert (total_stats)

    for status in total_stats:
        transmission_delay = cal_pkt_transmit_time_ms(status["header_length"], 
                                                            status["payload_size"], bw_kps=bandwidth_kbps)
        actual_delay = status["arrival_time_ms"] - status["send_time_ms"] - transmission_delay
        rtt_in_trace = False
        for i in range(len(one_way_delay_list)):
            if abs(actual_delay - one_way_delay_list[i]) <= ERROR * one_way_delay_list[i]:
                rtt_sample_cnt[i] += 1
                rtt_in_trace = True

        assert rtt_in_trace == True, "actual rtt not exist in trace"

    for i in range(len(rtt_sample_cnt)):
        assert rtt_sample_cnt[i] > 0
        if i:
            assert abs(rtt_sample_cnt[i-1] - rtt_sample_cnt[i]) <= ERROR * rtt_sample_cnt[i]


def test_rtt_available():
    traces_name = ["trace_rtt_200.json", "trace_rtt_400.json", "trace_rtt_600.json"]
    for trace in traces_name:
        trace_path = get_abs_path_by_name(trace)
        single_rtt_available(trace_path)


def test_rtt_persistence():
    traces_name = ["trace_rtt_200.json", "trace_rtt_400.json", "trace_rtt_600.json"]
    for trace in traces_name:
        trace_path = get_abs_path_by_name(trace)
        single_rtt_persistence(trace_path)


def test_rtt_dynamically():
    traces_name = ["trace_rtt_pattern_2.json", "trace_rtt_pattern_3.json", "trace_rtt_pattern_4.json"]
    for trace in traces_name:
        trace_path = get_abs_path_by_name(trace)
        single_rtt_dynamically(trace_path, run_times=20)
