#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym
import os, json


ERROR_STATIC = 0.05
ERROR_DYNAMICAL = 0.07


def get_gym_stats(trace_path, duration_time_ms=3000, bandwidth_bps=1000000):
    total_stats = []
    g = gym.Gym()
    g.reset(trace_path=trace_path, duration_time_ms=duration_time_ms)

    while True:
        stats, done = g.step(bandwidth_bps)
        if not done:
            total_stats += stats
        else:
            return total_stats


def get_info_from_trace(trace_file):
    with open(trace_file, 'r') as f:
        return json.loads(f.read())


def get_abs_path_by_name(trace_name):
    trace_path = os.path.join(
            os.path.dirname(__file__),
            "data/loss",
            trace_name)
    return trace_path


def single_loss_available(trace_path):
    total_stats = get_gym_stats(trace_path)
    assert (total_stats)

    for stats in total_stats:
        assert (isinstance(stats, dict))


def single_loss_persistence(trace_path, run_times=1, bandwidth_bps=1000000):
    # get information from trace
    trace_data = get_info_from_trace(trace_path)
    trace_pattern = trace_data["uplink"]["trace_pattern"]
    predict_error_rate = trace_pattern[0]["loss"]
    duration_time_ms = sum([item["duration"] for item in trace_pattern]) * run_times

    total_stats = get_gym_stats(trace_path, duration_time_ms=duration_time_ms, bandwidth_bps=bandwidth_bps)
    assert (total_stats)

    mp_src_seq = {}
    now_total, now_loss = 0, 0
    for i in range(len(total_stats)):
        ssrc = total_stats[i]["ssrc"]
        if ssrc not in mp_src_seq:
            mp_src_seq[ssrc] = 0
        # calculate loss packet
        if mp_src_seq[ssrc] + 1 < total_stats[i]["sequence_number"]:
            while mp_src_seq[ssrc] + 1 < total_stats[i]["sequence_number"]:
                now_total += 1
                now_loss += 1
                mp_src_seq[ssrc] += 1
        now_total += 1
        mp_src_seq[ssrc] += 1
    assert abs(now_loss / now_total - predict_error_rate) <= predict_error_rate * ERROR_STATIC


def single_loss_dynamically(trace_path, run_times=1, bandwidth_bps=1000000):
    # get information from trace
    trace_data = get_info_from_trace(trace_path)
    trace_pattern = trace_data["uplink"]["trace_pattern"]
    trace_duration_time_ms = sum([item["duration"] for item in trace_pattern])
    predict_error_rate = sum([item["loss"] * (item["duration"] / trace_duration_time_ms) for item in trace_pattern])
    duration_time_ms = sum([item["duration"] for item in trace_pattern]) * run_times

    total_stats = get_gym_stats(trace_path, duration_time_ms=duration_time_ms, bandwidth_bps=bandwidth_bps)
    assert (total_stats)

    mp_src_seq = {}
    now_total, now_loss = 0, 0
    for i in range(len(total_stats)):
        ssrc = total_stats[i]["ssrc"]
        if ssrc not in mp_src_seq:
            mp_src_seq[ssrc] = 0
        # calculate loss packet
        if mp_src_seq[ssrc] + 1 < total_stats[i]["sequence_number"]:
            while mp_src_seq[ssrc] + 1 < total_stats[i]["sequence_number"]:
                now_total += 1
                now_loss += 1
                mp_src_seq[ssrc] += 1
        now_total += 1
        mp_src_seq[ssrc] += 1
    assert abs(now_loss / now_total - predict_error_rate) <= predict_error_rate * ERROR_DYNAMICAL


def test_loss_available():
    traces_name = ["trace_loss_0.json", "trace_loss_0dot1.json", "trace_loss_0dot5.json"]
    for trace in traces_name:
        trace_path = get_abs_path_by_name(trace)
        single_loss_available(trace_path)


def test_loss_persistence():
    traces_name = ["trace_loss_0.json", "trace_loss_0dot1.json", "trace_loss_0dot5.json"]
    for trace in traces_name:
        trace_path = get_abs_path_by_name(trace)
        single_loss_persistence(trace_path, run_times=1000, bandwidth_bps=300000)

def test_loss_dynamically():
    traces_name = ["trace_loss_pattern_2.json", "trace_loss_pattern_3.json", "trace_loss_pattern_4.json"]
    for trace in traces_name:
        trace_path = get_abs_path_by_name(trace)
        single_loss_dynamically(trace_path, run_times=1000, bandwidth_bps=300000)
