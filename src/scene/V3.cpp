
#include "V3.h"
#include "M33.h"

using namespace std;

#include <ostream>
#include <istream>
#include <stdio.h>
#include <cmath>

V3::V3(float x, float y, float z) {
	xyz[0] = x;
	xyz[1] = y;
	xyz[2] = z;
}

void V3::Rewrite(float x, float y, float z) {
	xyz[0] = x;
	xyz[1] = y;
	xyz[2] = z;
}

void V3::Rewrite(V3 new_v) {
	xyz[0] = new_v[0];
	xyz[1] = new_v[1];
	xyz[2] = new_v[2];
}

float& V3::operator[](int index) {
	return xyz[index];
}

istream& operator>>(istream& is, V3& v) {
	is >> v[0];
	is >> v[1];
	is >> v[2];
	return is;
}

ostream& operator<<(ostream& os, V3 v) {
	os << v[0] << " ";
	os << v[1] << " ";
	os << v[2] << endl;
	return os;
}

V3 V3::operator+(V3 v1) {
	V3 ret;
	V3& v0 = *this;
	ret[0] = v0[0] + v1[0];
	ret[1] = v0[1] + v1[1];
	ret[2] = v0[2] + v1[2];
	return ret;
}

V3 V3::operator-(V3 v1) {
	V3& v0 = *this;
	return v0 + (v1 * -1.0f);
}

V3 V3::operator*(float c) {
	V3 ret;
	V3& v0 = *this;
	ret[0] = v0[0] * c;
	ret[1] = v0[1] * c;
	ret[2] = v0[2] * c;
	return ret;
}

V3 V3::operator/(float c) {
	V3& v0 = *this;
	return v0 * (1.0f / c);
}

float V3::operator*(V3 v1) {
	V3& v0 = *this;
	float ret = v0[0] * v1[0]
		+ v0[1] * v1[1]
		+ v0[2] * v1[2];
	return ret;
}

/* 
 * Evaluate v0 cross v1
 */

V3 V3::operator^(V3 v1) {
	V3& v0 = *this;
	V3 result(v0[1] * v1[2] - v0[2] * v1[1],
		v0[2] * v1[0] - v0[0] * v1[2],
		v0[0] * v1[1] - v0[1] * v1[0]
	);
	return result;
}

float V3::Length() {
	V3 v = *this;
	float l = std::sqrt(
		v[0] * v[0] +
		v[1] * v[1] +
		v[2] * v[2]
	);
	return l;
}

V3 V3::Normalize() {
	float l = Length();
	V3 v = *this;
	V3 ret;
	if (l > 0) {
		ret = v / l;
	}
	return ret;
}

V3 V3::RotateDirection(V3 direction, float alpha) {
	M33 rotationMatrix(direction, alpha);
	return rotationMatrix* (*this);
}

/*
 * Rotates the current V3 object
 * and returns the current V3 object
 */

V3 V3::RotateOnAxis(V3 origin, V3 axis, float alpha) {
	// set auxiliary axis
	V3 x(1, 0, 0);
	V3 y(0, 1, 0);
	V3 aux = std::abs(axis * x) < std::abs(axis * y) ? x : y;

	// find other 2 axes of the coordinate system
	V3 u = axis.Normalize(); 
	V3 v = (aux ^ u).Normalize(); 
	V3 w = (u ^ v).Normalize();   

	// transform P
	M33 uvwSystem(u, v, w);
	V3& Pt = *this;
	Pt = uvwSystem * (Pt - origin);
	
	// rotate in the u-v plane
	// about the input axis
	Pt = RotateDirection(axis, alpha);

	// Convert back to the original coordinate system

	// Invert the system matrix
	M33 inverseMatrix = uvwSystem.Invert();

	// undo the coordinate system change
	Pt = inverseMatrix * Pt + origin;
	return Pt;
}

void V3::SetFromColor(unsigned int color) {
	unsigned char* rgb = (unsigned char*)&color;
	V3& v = *this;
	v[0] = ((float)rgb[0]) / 255.0f;
	v[1] = ((float)rgb[1]) / 255.0f;
	v[2] = ((float)rgb[2]) / 255.0f;
}

unsigned int V3::GetColor() {
	unsigned int ret;
	unsigned int rgb[3];
	V3& v = *this;
	for (int ci = 0; ci < 3; ci++) {
		float fc = v[ci];
		if (fc < 0.0f)
			fc = 0.0f;
		if (fc > 1.0f)
			fc = 1.0f;
		rgb[ci] = (unsigned int)(255.0f * fc);
	}
	// little endian since stored as ABGR
	ret = 0xFF000000 + rgb[2] * 256 * 256 + rgb[1] * 256 + rgb[0];

	return ret;
}

V3 V3::LightThisColor(V3 lv, float ka, V3 normal) {
	V3& colorv = *this;
	float kd = lv * normal;
	kd = (kd < 0.0f) ? 0.0f : kd;
	V3 ret = colorv * (ka + (1.0f - ka) * kd);
	return ret;
}

void AABB::AddPoint(V3 newPoint) {

	for (int i = 0; i < 3; i++) {
		if (newPoint[i] < minC[i])
			minC[i] = newPoint[i];
		if (newPoint[i] > maxC[i])
			maxC[i] = newPoint[i];
	}
}
