﻿
#define GLEW_STATIC 1
#include <GL/glew.h>
// ici w dans wglew est pour Windows
#include "GL/wglew.h"

#include "Application.h"
#include <cstdint>
#include <iostream>
#include "../common/Texture.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "stb_image.h"

bool Application::initialize()
{
    GLenum error = glewInit();
    if (error != GLEW_OK)
        return false;

    // on utilise un texture manager afin de ne pas recharger une texture deja en memoire
    // de meme on va definir une ou plusieurs textures par defaut
    Texture::SetupManager();
    // 

    m_opaqueShader.LoadVertexShader("../../../../OpenLightShadow/shaders/opaque.vs.glsl");
    m_opaqueShader.LoadFragmentShader("../../../../OpenLightShadow/shaders/opaque.fs.glsl");
    m_opaqueShader.Create();

    m_FBOShader.LoadVertexShader("../../../../OpenLightShadow/shaders/RenderTexture.vs.glsl");
    m_FBOShader.LoadFragmentShader("../../../../OpenLightShadow/shaders/RenderTexture.fs.glsl");
    m_FBOShader.Create();

    {
        Mesh object;
        Mesh::ParseObj(&object, "data/lightning/lightning_obj.obj");

        Mesh::ParseObj(&object, "data/Chess_Knights.obj");
        m_objects.push_back(object);

        Mesh::ParseObj(&object, "data/plane.obj");
        m_objects.push_back(object);

    }

    uint32_t program = m_opaqueShader.GetProgram();
    for (Mesh& obj : m_objects) {
        obj.Setup(program);
    }

    // UBO
    glGenBuffers(1, &m_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
    // pas obligatoire de preallouer la zone memoire mais preferable
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_UBO);

    // binding de l'UBO et du Block dans le shader (à faire pour chaque shader !)    
    int32_t MatBlockIndex = glGetUniformBlockIndex(program, "Matrices");
    // note: le '0' (zero) ici correspond au meme '0' 
    // de glBindBufferBase() lors de l'initialisation
    glUniformBlockBinding(program, MatBlockIndex, 0);


    // data pour le projecteur 
    {
        //m_projTextureID = Texture::LoadTexture("batman_logo.png");

        glGenBuffers(1, &m_lightUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, m_lightUBO);
        // pas obligatoire de preallouer la zone memoire mais preferable
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_lightUBO);

        // binding de l'UBO et du Block dans le shader (à faire pour chaque shader !)    
        int32_t MatBlockIndex = glGetUniformBlockIndex(program, "LightMatrices");
        // note: le '1' ici correspond au meme '1' 
        // de glBindBufferBase() lors de l'initialisation
        glUniformBlockBinding(program, MatBlockIndex, 1);
    }


    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);


    glGenTextures(1, &m_projTextureID);

    glBindTexture(GL_TEXTURE_2D, m_projTextureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT,
        GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, m_projTextureID, 0);

    glDrawBuffer(GL_NONE);

    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer error: " << fboStatus << std::endl;



    //load variables

    ColorSpecular = new float[3] {0, 0, 0};
    ColorAmbiante = new float[3] {0, 0, 0};
    ColorDiffuse = new float[3] {0, 0, 0};
    Roughness = .5;

    Metatlic = false;

    CameraAngle = 0;
    Reflectance = 0;


    return true;
}

void Application::deinitialize()
{
    glDeleteBuffers(1, &m_UBO);

    glDeleteBuffers(1, &m_lightUBO);

    m_opaqueShader.Destroy();

    delete[] ColorSpecular;
    delete[] ColorAmbiante;
    delete[] ColorDiffuse;

    Texture::PurgeTextures();
}

void Application::update() {


    ImGui::Begin("View");

    ImGui::SliderFloat("Camera Angle", &CameraAngle, -180.0f, 180.0f);
    ImGui::End();

    ImGui::Begin("Material");

    ImGui::ColorEdit4("Color Ambiante", ColorAmbiante, ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Color Diffuse", ColorDiffuse, ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit4("Color Specular", ColorSpecular, ImGuiColorEditFlags_NoInputs);
    ImGui::SliderFloat("Roughness", &Roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Reflectance", &Reflectance, 0.0f, 1.0f);

    ImGui::Checkbox("Mettalic", &Metatlic);

    ImGui::End();
}

void Application::renderScene()
{


    glm::vec3 cameraPosition = glm::vec3({ 0.f, 1.f, 25.f });

    glm::vec3 light_position = { 10.f, 20.f, 30 };

    uint32_t program = m_FBOShader.GetProgram();
    glUseProgram(program);

    {
        glm::vec3 up{ 0.0f, 1.f, 0.f };
       
        glm::mat4 matrices[2];
        glm::mat4 lightMatrices[2];

        matrices[0] = glm::lookAt(light_position,glm::vec3(0,0,0), glm::vec3(0,1,0));
        matrices[1] = glm::ortho<float>(-35, 35, -35, 35, .01f, 75.0f);

        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, matrices, GL_STATIC_DRAW);
         
    }



    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    render( program);
    glCullFace(GL_FRONT);
    
    program = m_opaqueShader.GetProgram();
    glUseProgram(program);
    
    {
        glm::vec3 up{ 0.0f, 1.f, 0.f };
        glm::mat4 matrices[2];
        glm::mat4 lightMatrices[2];

        matrices[0] = glm::translate(glm::mat4(1.f), -cameraPosition);
        matrices[0] *= glm::rotate(glm::mat4(1.0f), 20.0f * (glm::pi<float>() / 180.0f), glm::vec3(1.0f, 0.0, 0.0));
        matrices[0] *= glm::rotate(glm::mat4(1.0f), CameraAngle*(glm::pi<float>()/180.0f), up);

        matrices[1] = glm::perspectiveFov(glm::radians(45.f), (float)m_width, (float)m_height, 0.1f, 1000.f);

        glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, matrices, GL_STATIC_DRAW);

        lightMatrices[0] = glm::lookAt(light_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        lightMatrices[1] = glm::ortho<float>(-35, 35, -35, 35, .01f, 75.0f);

        glBindBuffer(GL_UNIFORM_BUFFER, m_lightUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, lightMatrices, GL_STATIC_DRAW);

        uint32_t lightD = glGetUniformLocation(program, "u_lightDirection");
        glUniform3fv(lightD, 1,glm::value_ptr(light_position));

        uint32_t colorSpecular = glGetUniformLocation(program, "u_SpecularColor");
        glUniform3fv(colorSpecular, 1, ColorSpecular);

        uint32_t colorAmbiante = glGetUniformLocation(program, "u_AmbianteColor");
        glUniform3fv(colorAmbiante, 1, ColorAmbiante);
        
        uint32_t colorDiffuse = glGetUniformLocation(program, "u_DiffuseColor");
        glUniform3fv(colorDiffuse, 1, ColorDiffuse);
        
        int32_t RoughnessLocation = glGetUniformLocation(program, "u_Roughness");
        glUniform1f(RoughnessLocation, Roughness);

        int32_t Reflectancelocation = glGetUniformLocation(program, "u_Reflectance");
        glUniform1f(Reflectancelocation, Reflectance);


        int32_t MetalnessLocation = glGetUniformLocation(program, "u_Metalness");
        glUniform1i(MetalnessLocation, Metatlic);

    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);
    render(program);
}

void Application::render( uint32_t program)
{
   
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.973f, 0.514f, 0.475f, 1.f);

    // comme on va afficher de la 3D on efface le depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // backface culling et activation du z-buffer
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

   
    glm::vec3 light_position = { 10.f, 20.f, 50.f };

    glm::mat4 worldMatrix = glm::mat4(1.0);


    //
    // 
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_projTextureID);
    int32_t PROJTEXTURE = glGetUniformLocation(program, "u_ProjSampler");
    glUniform1i(PROJTEXTURE, 1);
    //
    // --------------------------------------------

    uint32_t WM = glGetUniformLocation(program, "u_WorldMatrix");
    glUniformMatrix4fv(WM, 1, false, glm::value_ptr(worldMatrix));

    for (auto& obj : m_objects)
    {
        for (auto& submesh : obj.meshes)
        {
            const Material& mat = submesh.materialId > -1 ? obj.materials[submesh.materialId] : Material::defaultMaterial;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);

            // bind implicitement les VBO et IBO rattaches, ainsi que les definitions d'attributs
            glBindVertexArray(submesh.VAO);
            // dessine les triangles
            glDrawElements(GL_TRIANGLES, submesh.indicesCount, GL_UNSIGNED_INT, 0);

        }
    }
}