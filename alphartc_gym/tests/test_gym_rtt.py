#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym

import os


def cal_pkt_transmit_time_ms(header_bytes, payload_bytes, bw_kps):
    return (header_bytes + payload_bytes) * 8 / bw_kps


def test_simple_instance():
    total_stats = []
    g = gym.Gym()
    trace_path = os.path.join(
        os.path.dirname(__file__),
        "data",
        "trace_rtt.json")
    g.reset(trace_path=trace_path)

    while True:
        stats, done = g.step(1000)
        if not done:
            total_stats += stats
        else:
            break
    assert(total_stats)
    
    wait_sendings = []
    for i in range(len(total_stats)):
        
        if total_stats[i]["send_time_ms"] // 200 != total_stats[i]["arrival_time_ms"] // 200:
            continue
        while (len(wait_sendings) and wait_sendings[0]["arrival_time_ms"] <= total_stats[i]["send_time_ms"]):
            wait_sendings.pop(0)

        queue_delay = sum([item["transmission_time"] for item in wait_sendings])
        bw_kps = 1e3
        delay = 50/2 if (total_stats[i]["send_time_ms"] // 200) % 2 == 0 else 20/2

        transmission_time = cal_pkt_transmit_time_ms(total_stats[i]["header_length"], total_stats[i]["payload_size"], bw_kps)

        next_time = total_stats[i]["send_time_ms"] + transmission_time + queue_delay + delay
        wait_sendings.append({
            "arrival_time_ms" : total_stats[i]["arrival_time_ms"],
            "transmission_time" : transmission_time
        })

        assert(abs(next_time - total_stats[i]["arrival_time_ms"]) <= 1)

