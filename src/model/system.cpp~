#include "model/system.hpp"

#include <algorithm>

System::System()
{
}

System::System(size_t _maxSize, std::vector<Hypothesis> _hypotheses) :
	maxSize(_maxSize),
	hypotheses(_hypotheses)
{
}

System::System(FILE* in)
{
	int size, max_size;
	std::fscanf(in, "%i %i", &max_size, &size);
	maxSize = max_size;
	for (int i = 0; i < size; i++)
		hypotheses.push_back(Hypothesis(in));
}

size_t System::write(FILE* out) const
{
	size_t bytes;
	bytes += std::fprintf(out, "%i %i\n", maxSize, hypotheses.size());
	for (auto h : hypotheses)
		bytes += h.write(out);
	bytes += std::fprintf(out, "\n");
	fflush(out);
	return bytes;
}

Matrix System::mode()
{
	if (hypotheses.size() == 0) return Matrix();

	auto best = hypotheses.at(0);
	for (auto h : hypotheses)
		if (h.weight() > best.weight())
			best = h;

	return best.mode();
}

void System::addVariance(size_t idx, float value)
{
	for (auto h : hypotheses)
		h.addVariance(idx, value);
}

void System::add(Evidence evidence)
{
	std::vector<Hypothesis> newHypotheses;

	for (auto h : hypotheses)
	{
		auto a = h;
		a.scale(.8);
		a.add(evidence);
		newHypotheses.push_back(a);

		auto b = h;
		b.scale(.2);
		newHypotheses.push_back(b);
	}

	if (newHypotheses.size() > maxSize)
	{
		std::sort(newHypotheses.begin(), newHypotheses.end(), [](const Hypothesis& a, const Hypothesis& b){ return a.weight() > b.weight(); });
		newHypotheses.erase(newHypotheses.begin() + maxSize, newHypotheses.end());

		auto scalar = 1 / newHypotheses.at(0).weight();
		for (Hypothesis& h : newHypotheses)
			h.scale(scalar);
	}

	hypotheses = newHypotheses;
}

