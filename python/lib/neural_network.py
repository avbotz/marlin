import tensorflow as tf
import numpy as np

class NeuralNetwork(object):
	def __init__(self, numInputs, numFeatures, numOutputs):
		self._session = tf.Session()
		self._inputs  = inputs  = tf.placeholder(tf.float32)
		self._targets = targets = tf.placeholder(tf.float32)

		def weight_variable(shape): return tf.Variable(tf.truncated_normal(shape, stddev=.1))
		W1 = weight_variable([numFeatures, numInputs])
		b1 = weight_variable([numFeatures])
		W2 = weight_variable([numOutputs, numFeatures])
		b2 = weight_variable([numOutputs])
		self._weights = weights = [W1, b1, W2, b2]
		self.session().run(tf.initialize_variables(weights))

		self._features = features = tf.sigmoid(tf.transpose(tf.matmul(W1, tf.transpose(inputs)))   + tf.reshape(b1, [1, numFeatures]))
		self._outputs  = outputs  = tf.sigmoid(tf.transpose(tf.matmul(W2, tf.transpose(features))) + tf.reshape(b2, [1, numOutputs]))

		self._loss = loss = -tf.reduce_sum(targets*tf.log(outputs + 1e-9) + (1-targets)*tf.log(1-outputs + 1e-9)) / tf.to_float(tf.shape(self._inputs)[0])

		opt = tf.train.GradientDescentOptimizer(learning_rate=1)
		grads = opt.compute_gradients(loss, self.weights())

		self._train_op = train_op = opt.apply_gradients(grads)

	def apply(self, inputs):
		return self.session().run(self.outputs(), {self._inputs: inputs})

	def evaluate(self, inputs, targets):
		return self.session().run(self.loss(), {self._inputs: inputs, self.targets(): targets})

	def train(self, inputs, targets):
		self.session().run(self.train_op(), {self.inputs(): inputs, self.targets(): targets})

	def load(self, filename):
		self.session().run([w.assign(t[1]) for w, t in zip(self.weights(), sorted(np.load(filename).items()))])

	def save(self, filename):
		np.savez(filename, *self.session().run(self.weights()))

	def session(self):  return self._session
	def inputs(self):   return self._inputs
	def targets(self):  return self._targets
	def features(self): return self._features
	def outputs(self):  return self._outputs
	def weights(self):  return self._weights
	def loss(self):     return self._loss
	def train_op(self):    return self._train_op

