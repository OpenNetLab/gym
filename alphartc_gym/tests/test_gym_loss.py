#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym
import os


ERROR = 0.2


def get_gym_stats(trace_name, duration_time_ms=3000):
    total_stats = []
    g = gym.Gym()
    trace_path = os.path.join(
        os.path.dirname(__file__),
        "data",
        trace_name)
    g.reset(trace_path=trace_path, duration_time_ms=duration_time_ms)
    
    while True:
        stats, done = g.step(1000000)
        if not done:
            total_stats += stats
        else:
            return total_stats


def test_loss_available():
    total_stats = get_gym_stats("trace_loss_dynamically.json")
    assert (total_stats)

    for stats in total_stats:
        assert (isinstance(stats, dict))


def test_loss_persistence():
    total_stats = get_gym_stats("trace_loss_0dot1.json")
    assert (total_stats)

    mp_src_seq = {}
    now_total, now_loss = 0, 0
    predict_error_rate = 0.1
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
    assert abs(now_loss / now_total - predict_error_rate) <= predict_error_rate * ERROR


def test_loss_dynamically():
    total_stats = get_gym_stats("trace_loss_dynamically.json")
    assert (total_stats)

    mp_src_seq = {}
    now_total, now_loss = 0, 0
    predict_error_rate = 0.2 / 2
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
    assert abs(now_loss / now_total - predict_error_rate) <= predict_error_rate * ERROR
