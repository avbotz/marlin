#include "vision/neural_network.hpp"

#include <cmath>

NeuralNetwork::NeuralNetwork(const Matrix& W1, const Matrix& b1, const Matrix& W2, const Matrix& b2) :
	m_W1(W1),
	m_b1(b1),
	m_W2(W2),
	m_b2(b2)
{
}

Matrix NeuralNetwork::apply(const Matrix& inputs) const
{
	auto sigmoid = [](float x)
	{
		return static_cast<float>(1 / (1 + std::exp(-x)));
	};

	auto features = m_W1 * inputs + m_b1;
	for (size_t i = 0; i < features.size(); i++)
		features.set(i, sigmoid(features.get(i)));

	auto outputs = m_W2 * features + m_b2;
	for (size_t i = 0; i < outputs.size(); i++)
		outputs.set(i, sigmoid(outputs.get(i)));

	return outputs;
}

