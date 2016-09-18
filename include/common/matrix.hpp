#ifndef COMMON_MATRIX_HPP
#define COMMON_MATRIX_HPP

#include <cstddef>
#include <cstdio>
#include <utility>
#include <initializer_list>

class Matrix
{
public:
	Matrix();
	Matrix(const Matrix&);
	Matrix(std::initializer_list<float>);
	Matrix(std::initializer_list<Matrix>);
	Matrix(size_t, size_t);

	Matrix(FILE*);
	size_t write(FILE*) const;

	size_t rows() const;
	size_t cols() const;
	size_t size() const;

	float get(size_t, size_t) const;
	float get(size_t) const;

	float set(size_t, size_t, float);
	float set(size_t, float);

	float magnitude() const;

	Matrix operator*(const Matrix&) const;
	Matrix operator*=(const Matrix&);
	Matrix operator+=(const Matrix&);
	Matrix operator-=(const Matrix&);

private:
	size_t m_rows, m_cols;
	float m_data[256];
};

Matrix operator+(const Matrix&, const Matrix&);
Matrix operator-(const Matrix&, const Matrix&);

#endif

