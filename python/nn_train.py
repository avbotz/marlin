#!/usr/bin/env python3

import sys

import numpy as np

from lib.neural_network import NeuralNetwork

import nn_training_data as td

flipped = [ td.orange_avhs_pool_bin_cover, td.avhs_pool_floor, td.wall_behind_torpedo ]

for l in flipped:
	for row in l:
		a = row[0]
		row[0] = row[2]
		row[2] = a

filename = sys.argv[1]

numInputs = 3
numFeatures = 4
numOutputs = 1

#positive = td.orange_torpedo_cover * 10
positive = td.orange_bin_cover_2016
#negative = td.yellow_torpedo_board + td.blue_water + td.white_glare + td.white_wall + td.green_wall + td.wall_behind_torpedo + td.avhs_pool_floor
negative = td.bin_yellow_torp_2016 + td.bin_background_2016

inputs = np.array(positive + negative, dtype=np.float32) / 256
targets = np.array([[1]]*len(positive) + [[0]]*len(negative), dtype=np.float32)

nn = NeuralNetwork(numInputs, numFeatures, numOutputs)

#nn.load(filename)

i = 0
while True:
	nn.train(inputs, targets)

	if i % (256**1) == 0:
		print("{} {}".format(int(i/256), nn.evaluate(inputs, targets)))
	if i % (256**1 * 1) == 0:
		nn.save(filename)
		print("{}".format(nn.session().run(nn.weights())))
	i += 1

