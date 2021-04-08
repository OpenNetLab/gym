#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym
import os


def test_simple_instance():
    total_stats = []
    g = gym.Gym()
    trace_path = os.path.join(
        os.path.dirname(__file__),
        "data",
        "trace_loss.json")
    g.reset(trace_path=trace_path)

    while True:
        stats, done = g.step(1000000)
        if not done:
            total_stats += stats
        else:
            break
    assert(total_stats)
    
    now_total = 0
    now_loss = 0
    now_status = 0
    mp_src_sep = {}
    for i in range(len(total_stats)):
        ssrc = total_stats[i]["ssrc"]
        if ssrc not in mp_src_sep:
            mp_src_sep[ssrc] = 1
        # calculate loss packet
        if mp_src_sep[ssrc] + 1 < total_stats[i]["sequence_number"]:
            assert now_status == 1
            while mp_src_sep[ssrc] + 1 < total_stats[i]["sequence_number"]:
                now_total += 1
                now_loss += 1
                mp_src_sep[ssrc] += 1
        # enter a new interval
        if total_stats[i]["send_time_ms"] // 200 == total_stats[i]["arrival_time_ms"] // 200:
            flag_loss = total_stats[i]["send_time_ms"] // 200 % 2
            if flag_loss == 0:
                if now_status == 1 and now_total >= 5:
                    assert now_loss > 0
                elif now_status == 0:
                    assert now_loss == 0
                now_total = 0
                now_loss = 0
                mp_src_sep[ssrc] = total_stats[i]["sequence_number"]
            now_status = flag_loss
        else:
            now_status = 1
        
        mp_src_sep[ssrc] = total_stats[i]["sequence_number"]
        now_total += 1 
