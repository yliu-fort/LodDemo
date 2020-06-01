#ifndef MODEL_H
#define MODEL_H

#include <memory>
//GLAD
#include <glad/glad.h>

//GLFW
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

#include "cmake_source_dir.h"
#include "shader.h"
#include "camera.h"
#include "mesh.h"
#include "texture_utility.h"

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct directLighting
{
    glm::vec3 direction = glm::vec3(0,-1,0);
    glm::vec3 ambient = glm::vec3(0);
    glm::vec3 diffuse = glm::vec3(1);
    glm::vec3 specular = glm::vec3(0);

    void setLighting(Shader& shader)
    {
        shader.setVec3("dirLight.direction", direction);
        shader.setVec3("dirLight.ambient", ambient);
        shader.setVec3("dirLight.diffuse", diffuse);
        shader.setVec3("dirLight.specular", specular);
    }

    void setLighting(Shader& shader, uint id)
    {
        shader.setVec3("dirLightArray[" + std::to_string(id) + "].direction", direction);
        shader.setVec3("dirLightArray[" + std::to_string(id) + "].ambient", ambient);
        shader.setVec3("dirLightArray[" + std::to_string(id) + "].diffuse", diffuse);
        shader.setVec3("dirLightArray[" + std::to_string(id) + "].specular", specular);
    }
};

enum RENDER_TYPE
{
    NORMAL=0,
    WIREFRAME=1,
    POINT=2
};

class Model
{
public:
  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<std::vector<Shader>> shaders;
  std::vector<Texture> textures;
  glm::mat4 model;

  Model(std::shared_ptr<Mesh> mesh_ptr, std::shared_ptr<std::vector<Shader>> shader_ptr)
      : mesh(mesh_ptr), shaders(shader_ptr), model(glm::mat4(1))
  {}

  // render the mesh
  void draw(const Camera& camera, RENDER_TYPE rt=NORMAL, directLighting* dirLightingSource = NULL)
  {
      // use shader
      (*shaders)[rt].use();

      // bind appropriate textures
      unsigned int diffuseNr  = 1;
      unsigned int specularNr = 1;
      unsigned int normalNr   = 1;
      unsigned int heightNr   = 1;
      for(unsigned int i = 0; i < textures.size(); i++)
      {
          glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
          // retrieve texture number (the N in diffuse_textureN)
          std::string number;
          std::string name = textures[i].type;
          if(name == "texture_diffuse")
              number = std::to_string(diffuseNr++);
          else if(name == "texture_specular")
              number = std::to_string(specularNr++); // transfer unsigned int to stream
          else if(name == "texture_normal")
              number = std::to_string(normalNr++); // transfer unsigned int to stream
           else if(name == "texture_height")
              number = std::to_string(heightNr++); // transfer unsigned int to stream

          // now set the sampler to the correct texture unit
          glUniform1i(glGetUniformLocation((*shaders)[rt].ID, (name + number).c_str()), i);
          // and finally bind the texture
          glBindTexture(GL_TEXTURE_2D, textures[i].id);
      }

      // Mode
      if(rt == WIREFRAME)
      {
          model = glm::scale(model,glm::vec3(1.01f));
          glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
      }

      // Lighting
      // directional light
      directLighting tmpDirLighting; tmpDirLighting.setLighting((*shaders)[rt]);
      if(dirLightingSource) { dirLightingSource->setLighting((*shaders)[rt]); }

      // View
      (*shaders)[rt].setVec3("viewPos",camera.Position);
      (*shaders)[rt].setMat4("projection_view",camera.GetFrustumMatrix());
      (*shaders)[rt].setMat4("model",model);

      // draw mesh
      if(rt == POINT)
          mesh->dummyDraw();
      else
          mesh->draw();

      if(rt == WIREFRAME)
      {
          glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
      }

      // always good practice to set everything back to defaults once configured.
      glActiveTexture(GL_TEXTURE0);
      //glUseProgram(0);
  }

};

// Derived clas -> astrophysical body
// Consider adding reference body
// only need to store relative location, orientation
class Planet : public Model
{
public:
    glm::vec3 location; // scaled with radius
    double scale;
    double radius = 6378100; // meter
    double atmosphereHeight = 90000.0; // meter
    directLighting* dirLighting;

    Planet(std::shared_ptr<Mesh> mesh_ptr, std::shared_ptr<std::vector<Shader>> shader_ptr)
        : Model(mesh_ptr,shader_ptr), dirLighting(NULL), location(glm::vec3(0,0,0))
    {
        std::string path = FP("../../resources/textures/earth");

        try
        {
            Texture tex;
            tex.id = loadTexture("8k_earth_daymap.jpg", path,false);
            tex.type = "texture_diffuse";
            tex.path =path;
            textures.push_back(tex);
        }
        catch (...)
        {
            std::cout << "ERROR::PLANET::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }

        try
        {
            Texture tex;
            tex.id = loadTiffTexture("8k_earth_specular_map.tif", path,false);
            tex.type = "texture_specular";
            tex.path =path;
            textures.push_back(tex);

        }
        catch (...)
        {
            std::cout << "ERROR::PLANET::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }

        try
        {
            Texture tex;
            tex.id = loadTiffTexture("8k_earth_normal_map.tif", path,false);
            tex.type = "texture_normal";
            tex.path =path;
            textures.push_back(tex);

        }
        catch (...)
        {
            std::cout << "ERROR::PLANET::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }

        try
        {
            Texture tex;
            tex.id = loadTexture("8k_earth_nightmap.jpg", path,false);
            tex.type = "texture_diffuse";
            tex.path =path;
            textures.push_back(tex);
        }
        catch (...)
        {
            std::cout << "ERROR::PLANET::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
    }

    void setDirLighting(directLighting* dirLight)
    {
        dirLighting = dirLight;
    }

    void bounce(Camera& camera)
    {
        // Caution: 1.0 is assumed scaled size
        if(glm::length(camera.Position) < (1.0f + 1.0f/radius))
        {
            // Reprojection
            camera.Position = (1.0f + 1.0f/(float)radius)*glm::normalize(camera.Position);
        }
    }

    // If in atomosphere, switch to horizontal camera
    void cameraRedirectionInAtomsphere(Camera& camera, double real_distance)
    {
        if((real_distance - radius) < atmosphereHeight)
        {
            camera.setReference(camera.Position);
            //std::cout << "Enter Atmosphere\n";
        }
    }


    void draw(Camera& camera, RENDER_TYPE rt=NORMAL)
    {

        //bounce(camera);
        //cameraRedirectionInAtomsphere(camera);
        //((Model*)this)->draw(camera, rt, dirLighting);

        Camera refCam(camera);

        // Compute current view position
        scale = 1.0/radius; // 1km / radius

        // Log-space scaling, 1e-1 ~ 1e100 meter range
        // For blinking issue distant stars will be put at around 1e95

        refCam.Near = 1.0f*scale; // m
        refCam.Far = 1.1f*radius; //m

        // non-scale position
        refCam.Position = location - camera.Position;
        bounce(refCam);

        // Compute current visual and real distance
        double current_vis_distance = glm::length(refCam.Position);
        double current_real_distance = current_vis_distance/scale; // ground distance


        // If too far
        if(current_vis_distance > 1.1f)
        {
            refCam.Near = 0.05f; // 0.1*radius
            refCam.Far = scale*radius/1.5e-3; // visible limit

            // scaled position
            //viewPos = location - refCam.Position;

            // Compute current visual and real distance
            current_vis_distance = scale * current_real_distance;
            current_real_distance = current_vis_distance/scale;

        }

        // Near: manual clipping, Only draw points for distant stars
        bool visible = (radius/current_real_distance > 1.5e-3);

        // Move model
        if(visible)
        {
            model = glm::translate(glm::mat4(1), refCam.Position);

            // transfer lighting dir

            // Scale camera offset
            refCam.Position = glm::vec3(0,0,0);

            //std::cout << "Real distance to EARTH SURFACE = " << current_real_distance - radius << "m\n";

            // Set property for camera, not copy
            cameraRedirectionInAtomsphere(camera, current_real_distance);

            ((Model*)this)->draw(refCam, rt, dirLighting);
        }
    }
};

// Derived clas -> astrophysical body
class Star : public Model
{
public:

    glm::vec3 location;
    double radius = 6.955e8; // meter
    double distance = 1.496e11;
    glm::vec3 albedo = glm::vec3(1.0);

    directLighting lightingSource;

    Star(std::shared_ptr<Mesh> mesh_ptr, std::shared_ptr<std::vector<Shader>> shader_ptr)
        : Model(mesh_ptr,shader_ptr){}

    Star(const char* name, std::shared_ptr<Mesh> mesh_ptr, std::shared_ptr<std::vector<Shader>> shader_ptr)
        : Model(mesh_ptr,shader_ptr)
    {
        std::string path = FP("../../resources/textures");

        try
        {
            Texture tex;
            tex.id = loadTexture(name, path,false);
            tex.type = "texture_diffuse";
            tex.path =path;
            textures.push_back(tex);
        }
        catch (...)
        {
            std::cout << "ERROR::STAR::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
    }

    void setRandom()
    {
        // set astro properties
        // compute scales
        radius = 1e8 + rand()/double(RAND_MAX)* 1e9;
        distance = 1e11 + rand()/double(RAND_MAX) * 1e30;

        float s = rand()/double(RAND_MAX)*2-1;
        float t = rand()/double(RAND_MAX)*2-1;
        float r = rand()/double(RAND_MAX)*2-1;
        setLocation(glm::vec3(s,t,r));

        albedo.x = rand()/double(RAND_MAX);
        albedo.y = rand()/double(RAND_MAX);
        albedo.z = rand()/double(RAND_MAX);

    }
    // Note: this location implied a reference coordinate located in (0,0,0)
    // Care must be taken when switching to another reference system
    void setLocation(glm::vec3 dir = glm::vec3(-0.25f,-0.23f,-1.2f))
    {
        // initialize log-space location
        location = glm::normalize(-dir) ;// glm::vec3(log(distance));

        // This implied that object receive light is located in origin
        // of reference coordinate. Object and lighting source must
        // stay in the same reference system.
        lightingSource.direction = -location;

    }

    void setLighting()
    {
        lightingSource.ambient = glm::vec3( 0.05f, 0.05f, 0.05f);
        lightingSource.diffuse = glm::vec3( 10.5f, 10.5f, 10.5f);
        lightingSource.specular= glm::vec3( 0.10f, 0.15f, 0.10f);
    }

    friend void bindLighting(Star& src, Planet& dest, glm::vec3 reference = glm::vec3(0))
    {
        dest.setDirLighting(&src.lightingSource);
    }

    void draw(const Camera& camera, RENDER_TYPE rt=NORMAL)
    {
        Camera refCam(camera);
        float neardist = 1000.0f;

        // Log-space scaling, 1e-1 ~ 1e100 meter range
        // For blinking issue distant stars will be put at around 1e95
        refCam.Near = 0.1f;
        refCam.Far = 100.0f*neardist;

        // Compute current view position
        glm::vec3 viewPos = location - refCam.Position/glm::vec3(distance);
        double current_real_distance = glm::length(viewPos)*distance;
        double current_vis_distance = log(current_real_distance)*neardist;
        double current_scale = current_vis_distance/current_real_distance;

        // Near: manual clipping, Only draw points for distant stars
        bool visible = (radius/current_real_distance > 1.5e-3);

        // Move model
        if(visible)
        {
            model = glm::translate(glm::mat4(1), glm::normalize(viewPos)*glm::vec3(current_vis_distance));
            model = glm::scale(model,glm::vec3(radius*current_scale));
        }else
        {
            // Put the model to 95% distance to suppress blinking artifact
            model = glm::translate(glm::mat4(1), glm::normalize(viewPos)*glm::vec3(0.95f*refCam.Far));
        }

        // Scale camera offset
        refCam.Position = glm::vec3(0,0,0);

        //std::cout << "Visual angle = " << radius/current_real_distance << "\n";

        if(visible)
        {
            (*shaders)[rt].use();
            (*shaders)[rt].setVec3("albedo",albedo);
            ((Model*)this)->draw(refCam, rt);
        }else
        {
            ((Model*)this)->draw(refCam, POINT);
        }


    }
};

#endif
