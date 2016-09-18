#!/usr/bin/env python3

import sys

import numpy as np

from lib.neural_network import NeuralNetwork
from lib import image as im

filename = sys.argv[1]

nn = NeuralNetwork(3, 4, 1)
nn.load(filename)

while True:
	imgI = im.read(sys.stdin.buffer)[:,:,[2,1,0]]
	img_shape = imgI.shape

	inputs = imgI.reshape([-1, 3]).astype(np.float32) / 256

	outputs = nn.apply(inputs)

	imgO = (np.concatenate([outputs]*3, 1).reshape(img_shape) * 255).astype(np.uint8)

	im.write(sys.stdout.buffer.raw, imgO)
#	im.write(sys.stdout.buffer.raw, np.concatenate([imgI[:,:,[2,1,0]], imgO]))

