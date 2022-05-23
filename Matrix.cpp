#include "Matrix.h"
#include <cmath>
Matrix Matrix::transl(float x, float y, float z)
{
	Matrix res;
	float* arr= res.getPtrForEdit();

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (i == j) arr[4 * i + j] = 1.0f;
			else if (j == 3) arr[4 * i + j] = i == 0 ? x : i == 1 ? y : z;
			else arr[4 * i + j] = 0.0f;
		}
	return res;
}

Matrix Matrix::scale(float sx, float sy, float sz)
{
	Matrix res;
	float* arr = res.getPtrForEdit();

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (i != j) arr[4 * i + j] = 0.0f;
			else if (j == 0) arr[4 * i + j] = sx;
			else if (j == 1) arr[4 * i + j] = sy;
			else if (j == 2) arr[4 * i + j] = sz;
			else arr[4 * i + j] = 1.0f;
		}
	return res;
}

Matrix Matrix::rotate_X(float a)
{
	Matrix res;
	float* arr = res.getPtrForEdit();

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (i == j) arr[4 * i + j] = i < 3 && i > 0 ? cos(a) : 1.0f;
			else if (i == 3 - j) arr[4 * i + j] = i < 3 && i > 0 ? sin(a) * (i == 2 ? 1: -1) : 0.0f;
			else arr[4 * i + j] = 0.0f;
		}
	return res;
}

Matrix Matrix::rotate_Y(float a)
{
	Matrix res;
	float* arr = res.getPtrForEdit();

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (i == j) arr[4 * i + j] = i % 2 == 0 ? cos(a) : 1.0f;
			else if (i == 2 - j) arr[4 * i + j] = i % 2 == 0 ? sin(a) * (i == 2 ? 1 : -1) : 0.0f;
			else arr[4 * i + j] = 0.0f;
		}
	return res;
}

Matrix Matrix::rotate_Z(float a)
{
	Matrix res;
	float* arr = res.getPtrForEdit();

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			if (i == j) arr[4 * i + j] = i < 2 ? cos(a) : 1.0f;
			else if (i == 1 - j) arr[4 * i + j] = i < 2 ? sin(a) * (i == 0 ? 1 : -1) : 0.0f;
			else arr[4 * i + j] = 0.0f;
		}
	return res;
}

Matrix Matrix::operator*(const Matrix& mat)
{
	Matrix res;
	float* arr = res.getPtrForEdit();
	const float* operand = mat.matrix;

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
		{
			arr[4 * i + j] = 0.0f;
			for (int k = 0; k < 4; k++) arr[4 * i + j] += matrix[4 * i + k] * operand[4 * k + j];
		}
	return res;
}

Matrix Matrix::perspective(float near, float far, float left, float right, float top, float bottom)
{
	Matrix res;
	float* arr = res.getPtrForEdit();

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++) arr[4 * i + j] = 0.0f;
	arr[0] = 2 * near / (right - left);
	arr[2] = (right + left) / (right - left);
	arr[5] = 2 * near / (top - bottom);
	arr[6] = (top + bottom) / (top - bottom);
	arr[10] = -(far + near) / (far - near);
	arr[11] = -2 * far * near / (far - near);
	arr[14] = -1;
	return res;
}