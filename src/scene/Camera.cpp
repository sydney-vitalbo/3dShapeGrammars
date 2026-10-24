//Camera.cpp
//Sydney Vitalbo

#include "scene/Camera.h"

#include <cmath>
#include <algorithm>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

namespace sg {

// anonymous namespace for this file only
namespace {
constexpr float kPi = 3.14159265358979323846f;

//converts: radians = degrees * kDeg2Rad.
constexpr float kDeg2Rad = kPi / 180.0f;
} // namespace

//constructor picks starting values so the scene is framed nicely
Camera::Camera()
    : target{0.0f, 1.0f, 0.0f}, // look slightly above ground so buildings sit in view
      yaw(35.0f),
      pitch(25.0f),
      distance(12.0f),
      fovYDegrees(45.0f),
      nearPlane(0.1f),
      farPlane(500.0f) {}

void Camera::orbit(float dYawDegrees, float dPitchDegrees) {
    yaw += dYawDegrees;

    //clamp pitch so we approach but never reach straight up/down
    pitch += dPitchDegrees;
    pitch = std::clamp(pitch, -89.0f, 89.0f);
}

void Camera::zoom(float factor) {
    //multiply by a factor (e.g. 0.9 = closer, 1.1 = farther)
    distance *= factor;
    //keeps distance within [min, max] so cannot go to infinity or go inside target
    distance = std::clamp(distance, 0.5f, 200.0f);
}

void Camera::pan(float dRight, float dUp) {
    //slides the point we're looking at sideways/up
    // 1. figure out which world direction that currently is (changes as we rotate)
    // 2. rebuild the camera's own axes: forward (eye->target), right, and up
    float eye[3];
    eyePosition(eye);

    float forward[3] = {target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
    float flen = std::sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
    if (flen > 1e-6f) { forward[0] /= flen; forward[1] /= flen; forward[2] /= flen; }

    const float worldUp[3] = {0.0f, 1.0f, 0.0f};
    // right = forward x worldUp
    float right[3] = {
        forward[1]*worldUp[2] - forward[2]*worldUp[1],
        forward[2]*worldUp[0] - forward[0]*worldUp[2],
        forward[0]*worldUp[1] - forward[1]*worldUp[0]
    };
    float rlen = std::sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
    if (rlen > 1e-6f) { right[0] /= rlen; right[1] /= rlen; right[2] /= rlen; }
    // up = right x forward
    float up[3] = {
        right[1]*forward[2] - right[2]*forward[1],
        right[2]*forward[0] - right[0]*forward[2],
        right[0]*forward[1] - right[1]*forward[0]
    };

    //scale pan speed with distance so it feels consistent at any zoom level
    float s = distance * 0.001f;
    for (int i = 0; i < 3; ++i) {
        target[i] += (-dRight * right[i] + dUp * up[i]) * s * 1000.0f * 0.001f;
    }
}

void Camera::eyePosition(float out[3]) const {
    //convert our stored angles+distance (spherical coords) into an actual 3D point (cartesian coords) for the eye

    float y = yaw * kDeg2Rad;   // to radians for sin/cos
    float p = pitch * kDeg2Rad;

    float cosP = std::cos(p);
    out[0] = target[0] + distance * cosP * std::sin(y); // x
    out[1] = target[1] + distance * std::sin(p);        // y (height)
    out[2] = target[2] + distance * cosP * std::cos(y); // z
}

void Camera::applyProjection(float aspect) const {
    if (aspect <= 0.0f) aspect = 1.0f; // guard against a zero-height viewport

    // switch to projection matrix mode
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); // ...reset it to the identity matrix
    
    //set up a perspective view:
    //   fovYDegrees: vertical field of view
    //   aspect: window width / height
    //   near / far: only things between these two distances are drawn
    gluPerspective(fovYDegrees, aspect, nearPlane, farPlane);

    //switch back to the modelview matrix (used for drawing geometry)
    glMatrixMode(GL_MODELVIEW);
}

void Camera::applyView() const {
    //view matrix positions and aims the camera
    float eye[3];
    eyePosition(eye);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // gluLookAt produces the view matrix:
    //   eye: where the camera is
    //   center: the target point
    //   up: which way is "up"
    //after this call, anything we draw is positioned relative to the camera!
    gluLookAt(eye[0], eye[1], eye[2],
              target[0], target[1], target[2],
              0.0f, 1.0f, 0.0f);
}

} // namespace sg
