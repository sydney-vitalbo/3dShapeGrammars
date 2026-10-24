// Camera.h
// Sydney Vitalbo

#ifndef SCENE_CAMERA_H
#define SCENE_CAMERA_H

namespace sg {

//https://gamedev.stackexchange.com/questions/20758/how-can-i-orbit-a-camera-about-its-target-point

/**
 * Arcball camera that looks at a target point
 *
 *   - yaw    : rotation around the world Y axis
 *   - pitch  : rotation above/below the horizon, clamped so never flip over the poles
 *   - distance: how far the eye sits from the target (zoom)
 */
class Camera {
public:
    Camera();

    //interactive controls (called by SceneView::handle)--------------

    /**
     * Rotate around the target
     * dYawDegrees: horizontal drag, in degrees
     * dPitchDegrees: vertical drag, in degrees (clamped near poles)
     */
    void orbit(float dYawDegrees, float dPitchDegrees);

    /**
     * Zoom toward/away from the target
     * factor: >1 zooms out, <1 zooms in
     */
    void zoom(float factor);

    //Shift the target in the camera's right/up plane
    void pan(float dRight, float dUp);

    //Get current eye position in world space (computed from the spherical state)
    void eyePosition(float out[3]) const;


    /**
     * Load this camera as the projection matrix
     * Call with the current viewport aspect ratio whenever the window resizes
     */
    void applyProjection(float aspect) const;

    /**
     * Load this camera as the modelview (view) matrix
     * Call at the start of each frame before drawing the scene.
     */
    void applyView() const;

private:
    float target[3];  // the point the camera orbits and looks at
    float yaw;
    float pitch;
    float distance; 

    float fovYDegrees; // vertical field of view
    float nearPlane;
    float farPlane;
};

} // namespace sg

#endif // SCENE_CAMERA_H
