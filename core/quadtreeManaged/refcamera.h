#ifndef REFCAMERA_H
#define REFCAMERA_H

#include "camera.h"
#include "shader.h"

class refCamera : public Camera
{
public:

    Camera& reference;
    refCamera(Camera& ref_cam) : Camera(ref_cam), reference(ref_cam) {}
    ~refCamera(){}

    // sync to reference camera's status
    Camera& get_reference() const
    {
        return this->reference;
    }
    void set_reference(Camera& cam)
    {
        reference = cam;
    }
    void sync_rotation()
    {
        Front = reference.Front;
        Up = reference.Up;
        Right = reference.Right;
        rotation = reference.rotation;
    }
    void sync_position()
    {
        Position = reference.Position;
    }
    void sync_frustrum()
    {
        Aspect = reference.Aspect;
        Near = reference.Near;
        Far = reference.Far;

        Zoom = reference.Zoom;
    }
    void render_frustrum() const;
    void draw_frustrum() const;
    void draw_frustrum_gui();
    void gui_interface();

    //Static member
    static Shader shader;
    static bool drawFrustrum;

};


#endif
