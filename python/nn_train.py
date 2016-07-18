#!/usr/bin/env python3

import sys

import numpy as np

from lib.neural_network import NeuralNetwork

import nn_training_data as td

filename = sys.argv[1]

numInputs = 3
numFeatures = 4
numOutputs = 1

positive = td.orange_bin_cover
negative = td.white_bin_border + td.black_bin_background + td.yellow_bin_silhouette + td.green_ground

inputs = np.array(positive + negative, dtype=np.float32) / 256
targets = np.array([[1]]*len(positive) + [[0]]*len(negative), dtype=np.float32)

nn = NeuralNetwork(numInputs, numFeatures, numOutputs)

nn.load(filename)

i = 0
while True:
	nn.train(inputs, targets)

	if i % 256 == 0:
		print("{} {}".format(int(i/256), nn.evaluate(inputs, targets)))
	if i % (256**2) == 0:
		nn.save(filename)
		print("{}".format(nn.session().run(nn.weights())))
	i += 1

