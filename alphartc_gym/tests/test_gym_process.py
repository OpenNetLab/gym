#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import gym_process
import gym_connect

import zmq

import os
import pytest

@pytest.mark.timeout(1.5)
def test_base():
    proc = gym_process.GymProcess(
        gym_id="test_gym",
        report_interval_ms=60,
        duration_time_ms=1000)
    conn = gym_connect.GymConnector(gym_id = "test_gym")
    all_stats = []
    while True:
        stats = conn.step(1e9)
        if stats == None:
            break
        all_stats += stats
    # stats shouldn't be empty
    assert(all_stats)
    for stats in all_stats:
        assert(isinstance(stats, dict))
    assert(proc.wait() == 0)

@pytest.mark.timeout(200)
def test_trace():
    trace_path = os.path.join(
        os.path.dirname(__file__),
        "data",
        "trace_example.json")
    proc = gym_process.GymProcess(
        gym_id="test_gym",
        trace_path=trace_path,
        report_interval_ms=60,
        duration_time_ms=0)
    conn = gym_connect.GymConnector(gym_id = "test_gym")
    all_stats = []
    while True:
        stats = conn.step(1e9)
        if stats == None:
            break
        all_stats += stats
    # stats shouldn't be empty
    assert(all_stats)
    for stats in all_stats:
        assert(isinstance(stats, dict))
    assert(proc.wait() == 0)
