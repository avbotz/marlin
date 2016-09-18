#include "model/evidence.hpp"

Evidence::Evidence()
{
}

Evidence::Evidence(const std::vector<Variable>& _variables) :
	variables(_variables)
{
}

Evidence::Evidence(FILE* in)
{
	int size;
	fscanf(in, " %i", &size);
	for (int i = 0; i < size; i++)
	{
		Variable v;
		fscanf(in, " %i %f %f", &v.index, &v.value, &v.variance);
		variables.push_back(v);
	}
}

size_t Evidence::write(FILE* out) const
{
	size_t bytes = 0;
	bytes += fprintf(out, "%zu ", variables.size());
	for (auto v : variables)
		bytes += fprintf(out, "%i %f %f ", v.index, v.value, v.variance);
	bytes += fprintf(out, "\n");
	fflush(out);
	return bytes;
}

