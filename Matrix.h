#pragma once
class Matrix
{
	float matrix[16];
	float* getPtrForEdit() { return matrix; }
public:
	const float* getPtr() { return matrix; }
	static Matrix transl(float x, float y, float z); //перенос
	static Matrix scale(float sx, float sy, float sz); //масштаб
	static Matrix rotate_X(float a); //поворот вокруг OX
	static Matrix rotate_Y(float a); //поворот вокруг OY
	static Matrix rotate_Z(float a); //поворот вокруг OZ
	static Matrix perspective(float near, float far, float left, float right, float top, float bottom); //перспективная проекция

	Matrix operator* (const Matrix& mat); //умножение матриц
};