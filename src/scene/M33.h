#pragma once

#include <ostream>
#include <istream>

using namespace std;

#include "V3.h"

class M33 {
public:
	V3 rows[3];
	M33() {};
	M33(float diagonal);
	M33(V3 r1, V3 r2, V3 r3);
	M33(V3 axis, float alpha);
	V3& operator[](int i);
	V3 GetColumn(int i);
	void SetColumn(int i, V3 v);
	M33 operator+(M33 m);
	M33 operator-(M33 m);
	M33 operator*(float c);
	M33 Transpose();
	float GetDeterminant();
	M33 Invert();
	V3 operator*(V3 v);
	M33 operator*(M33 m);
	void M33::SetRotY(float alpha);
	friend ostream& operator<<(ostream& os, M33 m);
	friend istream& operator>>(istream& is, M33 m);
};
