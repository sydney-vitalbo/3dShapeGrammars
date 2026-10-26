
#pragma once

#include <ostream>
#include <istream>
#include <cfloat>

# define M_PI   (3.14159265358979323846)  /* pi */

using namespace std;

class M33;

class V3 {
public:
	float xyz[3];
	V3() {};
	V3(float x, float y, float z);
	void Rewrite(float x, float y, float z);
	void Rewrite(V3 new_v);
	float& operator[](int i);
	friend istream& operator>>(istream& is, V3& v);
	friend ostream& operator<<(ostream& os, V3 v);
	V3 operator+(V3 v1);
	V3 operator-(V3 v1);
	V3 operator*(float c);
	V3 operator/(float c);
	float operator*(V3 v1);
	V3 operator^(V3 v1);
	float Length();
	V3 Normalize();
	V3 RotateDirection(V3 direction, float alpha);
	V3 RotateOnAxis(V3 origin, V3 direction, float alpha);
	void SetFromColor(unsigned int color);
	unsigned int GetColor();
	V3 LightThisColor(V3 lv, float ka, V3 normal);
};

class AABB {
public:
	V3 minC, maxC;
	AABB() : minC(FLT_MAX, FLT_MAX, FLT_MAX),
		maxC(-FLT_MAX, -FLT_MAX, -FLT_MAX) {};
	void AddPoint(V3 newPoint);
};
