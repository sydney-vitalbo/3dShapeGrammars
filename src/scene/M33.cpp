#include "M33.h"
#include <cmath>
#include <math.h>

M33::M33(V3 r1, V3 r2, V3 r3) {
    rows[0] = r1;
    rows[1] = r2;
    rows[2] = r3;
}

M33::M33(float diagonal) {
    rows[0] = V3(diagonal, 0.0f, 0.0f);
    rows[1] = V3(0.0f, diagonal, 0.0f);
    rows[2] = V3(0.0f, 0.0f, diagonal);
}

/*
 * axis is a direction vector of the axis to rotate on
 * set rows to rotation matrix of rotation
 * by alpha degrees
 */

M33::M33(V3 axis, float alpha) {
    alpha = alpha * M_PI / 180;
    axis = axis.Normalize();
    float sinA = std::sin(alpha);
    float oneMinusCosA = 1.0f - std::cos(alpha);

    M33 K(
        V3(0, -axis[2], axis[1]),
        V3(axis[2], 0, -axis[0]),
        V3(-axis[1], axis[0], 0)
    );

    M33 I(1.0f);
    M33 K2 = K * K;

    for (int i = 0; i < 3; i++) {
        rows[i] = I[i]
            + (K[i] * sinA)
            + (K2[i]) * oneMinusCosA;
    }

}

V3& M33::operator[](int i) {
    return rows[i];
}

V3 M33::GetColumn(int i) {
    return V3(rows[0][i], rows[1][i], rows[2][i]);
}

void M33::SetColumn(int i, V3 v) {
    rows[0][i] = v[0];
    rows[1][i] = v[1];
    rows[2][i] = v[2];
}

M33 M33::operator+(M33 m) {
    return M33(rows[0] + m.rows[0], rows[1] + m.rows[1], rows[2] + m.rows[2]);
}

M33 M33::operator-(M33 m) {
    return M33(rows[0] - m.rows[0], rows[1] - m.rows[1], rows[2] - m.rows[2]);
}

M33 M33::operator*(float c) {
    return M33(rows[0] * c, rows[1] * c, rows[2] * c);
}

M33 M33::Transpose() {
    M33 m = *this;
    M33 transpose(
        m.GetColumn(0), m.GetColumn(1), m.GetColumn(2)
    );
    return transpose;
}

float M33::GetDeterminant() {

    float det = rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1]) -
                rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0]) +
                rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0]);
    return det;
}

M33 M33::Invert() {
    // Calculate the determinant

    float det = GetDeterminant();

    if (std::abs(det) < 1e-6) {
        return M33(0.0f);
    }

    V3 a = GetColumn(0), b = GetColumn(1), c = GetColumn(2);
    V3 _a = b ^ c; _a = _a * (1.0f / (a * _a));
    V3 _b = c ^ a; _b = _b * (1.0f / (b * _b));
    V3 _c = a ^ b; _c = _c * (1.0f / (c * _c));
    M33 inverseMatrix(_a, _b, _c);
    return inverseMatrix;
}

V3 M33::operator*(V3 v) {
    V3 result = V3(
        rows[0] * v,
        rows[1] * v,
        rows[2] * v
    );
    return result;
}

M33 M33::operator*(M33 m) {
    M33 result;
    M33 m0 = *this;
    for (int col = 0; col < 3; col++) {
        V3 column = m0 * m.GetColumn(col);
        result.SetColumn(col, column);
    }
    return result;
}


ostream& operator<<(ostream& os, M33 m) {
    os << m[0] << m[1] << m[2];
    return os;
}

istream& operator>>(istream& is, M33& m) {
    is >> m[0] >> m[1] >> m[2];
    return is;
}

void M33::SetRotY(float alpha) {
    float alphaRad = M_PI / 180.0f * alpha;
    M33& m = *this;
    m[0] = V3(cosf(alphaRad), 0.0f, sinf(alphaRad));
    m[1] = V3(0.0f, 1.0f, 0.0f);
    m[2] = V3(-sinf(alphaRad), 0.0f, cosf(alphaRad));
}