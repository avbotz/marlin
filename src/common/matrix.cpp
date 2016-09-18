#include "common/matrix.hpp"

#include <stdexcept>
#include <cstring>
#include <cmath>
#include <algorithm>

Matrix::Matrix() :
	m_rows(0),
	m_cols(0)
{
}

Matrix::Matrix(const Matrix& a) :
	m_rows(a.rows()),
	m_cols(a.cols())
{
	std::memcpy(m_data, a.m_data, size() * sizeof(float));
}

Matrix::Matrix(std::initializer_list<float> values) :
	m_cols(1),
	m_rows(0)
{
	for (auto v : values)
		m_data[m_rows++] = v;

}

Matrix::Matrix(std::initializer_list<Matrix> rows) :
	m_cols(0),
	m_rows(0)
{
	for (auto r : rows) m_cols = std::max(m_cols, r.size());

	for (auto r : rows)
	{
		for (size_t i = 0; i < r.size(); i++)
			set(m_rows, i, r.get(i));

		m_rows++;
	}
}

// <size> <element 1> <element 2> ...
Matrix::Matrix(FILE* in)
{
	int rows, cols;
	std::fscanf(in, " %i %i", &rows, &cols);
	m_rows = rows;
	m_cols = cols;
	for (int i = 0; i < m_rows * m_cols; i++)
		std::fscanf(in, " %f", &m_data[i]);
}

size_t Matrix::write(FILE* out) const
{
	size_t bytes = 0;
	bytes += std::fprintf(out, "%zu %zu ", m_rows, m_cols);
	for (int i = 0; i < m_rows * m_cols; i++)
		bytes += std::fprintf(out, "%f ", m_data[i]);
	bytes += std::fprintf(out, "\n");
	fflush(out);
	return bytes;
}

size_t Matrix::rows() const
{
	return m_rows;
}

size_t Matrix::cols() const
{
	return m_cols;
}

size_t Matrix::size() const
{
	return m_rows * m_cols;
}

float Matrix::get(size_t i, size_t j) const
{
	return m_data[i * m_cols + j];
}

float Matrix::get(size_t i) const
{
	return m_data[i];
}

float Matrix::set(size_t i, size_t j, float value)
{
	return m_data[i * m_cols + j] = value;
}

float Matrix::set(size_t i, float value)
{
	return m_data[i] = value;
}

float Matrix::magnitude() const
{
	float sum = 0;
	for (size_t i = 0; i < size(); i++)
		sum += std::pow(get(i), 2);
	return std::sqrt(sum);
}

Matrix Matrix::operator*(const Matrix& a) const
{
	if (cols() != a.rows()) throw std::runtime_error("incompatible size");

	Matrix res;
	res.m_rows = rows();
	res.m_cols = a.cols();

	for (int i = 0; i < res.rows(); i++)
	{
		for (int j = 0; j < res.cols(); j++)
		{
			res.m_data[i * res.cols() + j] = 0;
			for (int k = 0; k < cols(); k++)
				res.m_data[i * res.cols() + j] += get(i, k) * a.get(k, j);
		}
	}

	return res;
}

Matrix Matrix::operator*=(const Matrix& a)
{
	return *this = *this * a;
}

Matrix operator+(const Matrix& a, const Matrix& b)
{
	Matrix res = a;
	res += b;
	return res;
}

Matrix Matrix::operator+=(const Matrix& a)
{
	if (size() != a.size()) throw std::runtime_error("incompatible size");
	for (int i = 0; i < size(); i++)
		m_data[i] += a.m_data[i];
	return *this;
}

Matrix operator-(const Matrix& a, const Matrix& b)
{
	Matrix res = a;
	res -= b;
	return res;
}

Matrix Matrix::operator-=(const Matrix& a)
{
	if (size() != a.size()) throw std::runtime_error("incompatible size");
	for (int i = 0; i < size(); i++)
		m_data[i] -= a.get(i);
	return *this;
}

