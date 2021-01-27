#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import zmq
import json

__ZMQ_TYPE__ = "ipc://"
__ZMQ_PATH__ = "/tmp/"
__ZMQ_PREFIX__ = __ZMQ_TYPE__ + __ZMQ_PATH__
__GYM_EXIT_FLAG__ = b"Bye"

class GymConnector(object):
    def __init__(self, gym_id = "gym"):
        self.gym_id = gym_id
        self.zmq_ctx = zmq.Context()
        self.zmq_sock = self.zmq_ctx.socket(zmq.REQ)
        self.zmq_sock.connect(__ZMQ_PREFIX__ + self.gym_id)

    def step(self, bandwidth):
        self.zmq_sock.send_string(str(int(bandwidth)))
        rep = self.zmq_sock.recv()
        if rep == __GYM_EXIT_FLAG__:
            return None
        return json.loads(rep)

    def __del__(self):
        self.zmq_sock.disconnect(__ZMQ_PREFIX__ + self.gym_id)
