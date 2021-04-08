#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym
import os


def get_gym_stats(trace_name, duration_time_ms=3000):
    total_stats = []
    g = gym.Gym()
    trace_path = os.path.join(
        os.path.dirname(__file__),
        "data",
        trace_name)
    g.reset(trace_path=trace_path, duration_time_ms=duration_time_ms)
    
    while True:
        stats, done = g.step(1000)
        if not done:
            total_stats += stats
        else:
            return total_stats


def cal_pkt_transmit_time_ms(header_bytes, payload_bytes, bw_kps):
    return (header_bytes + payload_bytes) * 8 / bw_kps


def test_rtt_available():
    total_stats = get_gym_stats("trace_rtt_dynamically.json")

    assert (total_stats)
    for stats in total_stats:
        assert (isinstance(stats, dict))


def test_rtt_persistence():
    total_stats_rtt_400 = get_gym_stats("trace_rtt_400.json")
    assert (total_stats_rtt_400)

    for i in range(len(total_stats_rtt_400)):
        transmission_delay = cal_pkt_transmit_time_ms(total_stats_rtt_400[i]["header_length"], 
                                                            total_stats_rtt_400[i]["payload_size"], bw_kps=1000)
        predict_arrival_time_ms = total_stats_rtt_400[i]["send_time_ms"] + 400/2 + transmission_delay
        assert abs(total_stats_rtt_400[i]["arrival_time_ms"] - predict_arrival_time_ms) <= 1


def test_rtt_dynamically():
    total_stats_rtt_dynamically = get_gym_stats("trace_rtt_dynamically.json", duration_time_ms=10000)

    assert (total_stats_rtt_dynamically)
    
    rtt_cnt_200 = 0
    rtt_cnt_400 = 0
    for i in range(len(total_stats_rtt_dynamically)):

        transmission_delay = cal_pkt_transmit_time_ms(total_stats_rtt_dynamically[i]["header_length"], 
                                                            total_stats_rtt_dynamically[i]["payload_size"], bw_kps=1000)
        one_way_delay = total_stats_rtt_dynamically[i]["arrival_time_ms"] - total_stats_rtt_dynamically[i]["send_time_ms"] - transmission_delay
        if abs(one_way_delay - 200/2) <= 1:
            rtt_cnt_200 += 1
        elif abs(one_way_delay - 400/2) <= 1:
            rtt_cnt_400 += 1
        else:
            assert 0 == 1, "other rtt"

    assert (rtt_cnt_200 > 0)
    assert (rtt_cnt_400 > 0)
    assert (rtt_cnt_400 * 2 > rtt_cnt_200)