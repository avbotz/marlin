#ifndef MODEL_HYPOTHESIS_HPP
#define MODEL_HYPOTHESIS_HPP

#include <cstdio>
#include <vector>

#include "common/matrix.hpp"
#include "model/evidence.hpp"

class Hypothesis
{
public:
	Hypothesis();
	Hypothesis(FILE*);

	size_t write(FILE*) const;

	float weight() const;
	Matrix mode() const;

	void add(Evidence);
	void scale(float);

private:
	Matrix m_values;
	Matrix m_variance;

	float m_weight;
};

#endif

