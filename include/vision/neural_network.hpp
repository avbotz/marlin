#ifndef VISION_NEURAL_NETWORK_HPP
#define VISION_NEURAL_NETWORK_HPP

#include "common/matrix.hpp"

class NeuralNetwork
{
public:
	NeuralNetwork(const Matrix&, const Matrix&, const Matrix&, const Matrix&);
	Matrix apply(const Matrix&) const;

private:
	const Matrix m_W1;
	const Matrix m_b1;
	const Matrix m_W2;
	const Matrix m_b2;
};

#endif

