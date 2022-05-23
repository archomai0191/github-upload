#pragma once
class Matrix
{
	float matrix[16];
	float* getPtrForEdit() { return matrix; }
public:
	const float* getPtr() { return matrix; }
	static Matrix transl(float x, float y, float z); //�������
	static Matrix scale(float sx, float sy, float sz); //�������
	static Matrix rotate_X(float a); //������� ������ OX
	static Matrix rotate_Y(float a); //������� ������ OY
	static Matrix rotate_Z(float a); //������� ������ OZ
	static Matrix perspective(float near, float far, float left, float right, float top, float bottom); //������������� ��������

	Matrix operator* (const Matrix& mat); //��������� ������
};