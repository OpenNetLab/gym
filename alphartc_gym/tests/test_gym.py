#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from alphartc_gym import gym

import os
import glob


def test_basic():
    total_stats = []
    g = gym.Gym("test_gym")
    g.reset()
    while True:
        stats, done = g.step(1000)
        if not done:
            total_stats += stats
        else:
            break
    assert(total_stats)
    for stats in total_stats:
        assert(isinstance(stats, dict))

def test_multiple_instances():
    total_stats = []
    g1 = gym.Gym()
    g2 = gym.Gym()
    g1.reset()
    g2.reset()
    while True:
        stats, done = g1.step(1000)
        if not done:
            total_stats += stats
        else:
            break
    while True:
        stats, done = g2.step(1000)
        if not done:
            total_stats += stats
        else:
            break
    assert(total_stats)
    for stats in total_stats:
        assert(isinstance(stats, dict))

def test_trace():
    total_stats = []
    trace_files = os.path.join(os.path.dirname(__file__), "data", "*.json")
    for trace_file in glob.glob(trace_files):
        g = gym.Gym("test_gym")
        g.reset(trace_path=trace_file, report_interval_ms=60, duration_time_ms=0)
        while True:
            stats, done = g.step(1000)
            if  not done:
                total_stats += stats
            else:
                break
        assert(total_stats)
        for stats in total_stats:
            assert(isinstance(stats, dict))
