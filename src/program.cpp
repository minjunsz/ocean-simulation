// BSD 3 - Clause License
//
// Copyright(c) 2020, Aaron Hornby
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//     OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

/*
 * Sample boilerplate taken from CPSC453 at the University of Calgary
 * Main class for user-defined program
 * Created on: Sep 10, 2018
 * Author: John Hall
 * Modifications by: Aaron Hornby (10176084)
 */

#include "program.h"

#include <fstream>
#include <iostream>
#include <string>

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>
#include <imgui/examples/imgui_impl_opengl3.h>

// remember: include glad before GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "input-handler.h"
#include "mesh-object.h"
#include "object-loader.h"
#include "render-engine.h"

namespace wave_tool
{
    Program::Program() {}

    Program::~Program() {}

    std::shared_ptr<RenderEngine> Program::getRenderEngine() const
    {
        return m_renderEngine;
    }

    bool Program::start()
    {
        if (!setupWindow())
            return false;

        m_renderEngine = std::make_shared<RenderEngine>(m_window);

        initScene();

        // image.Initialize();
        // do a bunch of raytracing into texture
        // image.SaveToFile("image.png"); // no need to put in loop since we dont update image

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        // render loop
        while (!glfwWindowShouldClose(m_window))
        {
            // handle inputs
            glfwPollEvents();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            buildUI();

            // rendering...
            ImGui::Render();
            // image.Render();
            m_renderEngine->render(m_skyboxStars, m_skysphere, m_skyboxClouds, m_waterGrid, m_meshObjects);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(m_window);
        }

        return cleanup();
    }

    // TODO: look at Dear ImGui demo code and expand this to be better organized
    void Program::buildUI()
    {
        // start Dear ImGui frame...
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(1024.0f, 64.0f), ImVec2(1024.0f, 336.0f));

        ImGui::Begin("SETTINGS");

        ImGui::Separator();

        // reference: https://github.com/ocornut/imgui/blob/cc0d4e346a3e4a5408c85c7e6bf0df5e1307bb2d/examples/example_marmalade/main.cpp#L93
        ImGui::Text("AVG. FRAMETIME (VSYNC ON) - %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Separator();

        // reference: https://www.glfw.org/docs/latest/group__keys.html
        // reference: https://github.com/ocornut/imgui/blob/7b3d379819c487f0d5323ed7bd7c9109a2bc1d76/imgui_demo.cpp#L154-L167
        if (ImGui::TreeNode("CONTROLS"))
        {
            ImGui::Separator();
            if (ImGui::TreeNode("INTERNAL"))
            {
                ImGui::Separator();
                ImGui::BulletText("A - move left");
                ImGui::BulletText("D - move right");
                ImGui::BulletText("E - move up");
                ImGui::BulletText("Q - move down");
                ImGui::BulletText("S - move back");
                ImGui::BulletText("W - move forward");
                ImGui::BulletText("ESCAPE - exit app");
                ImGui::BulletText("LEFT_CLICK + DRAG - rotate camera");
                ImGui::BulletText("SCROLL - zoom");
                ImGui::Separator();
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("UI"))
            {
                ImGui::Separator();
                ImGui::BulletText("CTRL + LEFT_CLICK - convert widget into an input box");
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("note: this feature is only available on some widgets, and may be overridden if nonsensical inputs are entered.");
                ImGui::Separator();
                ImGui::TreePop();
            }
            ImGui::Separator();
            ImGui::TreePop();
        }

        ImGui::Separator();

        const float MIN_TIME = 7.0f;
        const float MAX_TIME = 17.0f;
        if (ImGui::SliderFloat("TIME OF DAY (HOURS)", &m_renderEngine->timeOfDayInHours, MIN_TIME, MAX_TIME))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            m_renderEngine->timeOfDayInHours = glm::clamp(m_renderEngine->timeOfDayInHours, MIN_TIME, MAX_TIME);
        }

        ImGui::SameLine();

        if (ImGui::Button("TOGGLE ANIMATION##0"))
        {
            m_renderEngine->isAnimatingTimeOfDay = !m_renderEngine->isAnimatingTimeOfDay;
        }
        if (m_renderEngine->isAnimatingTimeOfDay && m_renderEngine->animationSpeedTimeOfDayInSecondsPerHour > 0.0f)
        {
            float const deltaTimeInSeconds = 1.0f / ImGui::GetIO().Framerate;
            float const deltaTimeOfDayInHours = (1.0f / m_renderEngine->animationSpeedTimeOfDayInSecondsPerHour) * deltaTimeInSeconds;
            m_renderEngine->timeOfDayInHours += deltaTimeOfDayInHours;
            m_renderEngine->timeOfDayInHours = MIN_TIME + glm::mod(m_renderEngine->timeOfDayInHours - MIN_TIME, MAX_TIME - MIN_TIME);
        }

        if (ImGui::SliderFloat("CLOUD PROPORTION", &m_renderEngine->cloudProportion, 0.0f, 0.3f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            m_renderEngine->cloudProportion = glm::clamp(m_renderEngine->cloudProportion, 0.0f, 0.3f);
        }

        static int GerstnerWaveCount = 4;
        if (ImGui::InputInt("Gerstner Wave Count", &GerstnerWaveCount, 1, 1))
        {
            GerstnerWaveCount = glm::clamp(GerstnerWaveCount, 1, 4);
        }
        ImGui::SameLine();
        if (ImGui::Button("Generate"))
        {
            for (int i = 0; i < geometry::GerstnerWave::MAX_COUNT; ++i)
            {
                if (i < GerstnerWaveCount)
                {
                    float A, w, phi, steep;
                    glm::vec2 dir;
                    A = getRandomFloat(0.05f, 0.15f);   // amplitude: 0.05 ~ 0.15
                    w = getRandomFloat(0.5f, 2.0f);     // frequency: 0.5 ~ 2.0
                    phi = getRandomFloat(0.3f, 1.0f);   // phase: 0.3 ~ 1.0
                    steep = getRandomFloat(0.0f, 1.0f); // steepness: 0.0 ~ 1.0
                    dir = getRandomDirection();
                    m_renderEngine->gerstnerWaves.at(i) = nullptr;
                    m_renderEngine->gerstnerWaves.at(i) = std::make_shared<geometry::GerstnerWave>(A, w, phi, steep, dir);
                }
                else
                    m_renderEngine->gerstnerWaves.at(i) = nullptr;
            }
        }

        ImGui::Separator();

        ImGui::Text("WATER-GRID POLYGON MODE:");
        ImGui::SameLine();
        if (ImGui::Button("FULL##4"))
            m_waterGrid->m_polygonMode = PolygonMode::FILL;
        ImGui::SameLine();
        if (ImGui::Button("WIREFRAME##4"))
            m_waterGrid->m_polygonMode = PolygonMode::LINE;
        ImGui::Separator();

        if (!m_renderEngine->gerstnerWaves.empty())
        {
            if (ImGui::TreeNode("GERSTNER WAVES"))
            {
                ImGui::Separator();
                for (unsigned int i = 0; i < m_renderEngine->gerstnerWaves.size(); ++i)
                {
                    if (nullptr == m_renderEngine->gerstnerWaves.at(i))
                        continue;

                    if (ImGui::TreeNode(std::string{"wave" + std::to_string(i)}.c_str()))
                    {
                        ImGui::Separator();
                        if (ImGui::SliderFloat(std::string{"Amplitude##" + std::to_string(i)}.c_str(), &m_renderEngine->gerstnerWaves.at(i)->amplitude_A, 0.0f, 1.0f))
                        {
                            // force-clamp (handle CTRL + LEFT_CLICK)
                            if (m_renderEngine->gerstnerWaves.at(i)->amplitude_A < 0.0f)
                                m_renderEngine->gerstnerWaves.at(i)->amplitude_A = 0.0f;
                        }
                        if (ImGui::SliderFloat(std::string{"Frequency##" + std::to_string(i)}.c_str(), &m_renderEngine->gerstnerWaves.at(i)->frequency_w, 0.0f, 1.0f))
                        {
                            // force-clamp (handle CTRL + LEFT_CLICK)
                            if (m_renderEngine->gerstnerWaves.at(i)->frequency_w < 0.0f)
                                m_renderEngine->gerstnerWaves.at(i)->frequency_w = 0.0f;
                        }
                        if (ImGui::SliderFloat(std::string{"Phase Constant (~Speed)##" + std::to_string(i)}.c_str(), &m_renderEngine->gerstnerWaves.at(i)->phaseConstant_phi, 0.0f, 10.0f))
                        {
                            // force-clamp (handle CTRL + LEFT_CLICK)
                            if (m_renderEngine->gerstnerWaves.at(i)->phaseConstant_phi < 0.0f)
                                m_renderEngine->gerstnerWaves.at(i)->phaseConstant_phi = 0.0f;
                        }
                        if (ImGui::SliderFloat(std::string{"Steepness##" + std::to_string(i)}.c_str(), &m_renderEngine->gerstnerWaves.at(i)->steepness_Q, 0.0f, 1.0f))
                        {
                            // force-clamp (handle CTRL + LEFT_CLICK)
                            m_renderEngine->gerstnerWaves.at(i)->steepness_Q = glm::clamp(m_renderEngine->gerstnerWaves.at(i)->steepness_Q, 0.0f, 1.0f);
                        }
                        if (ImGui::SliderFloat2(std::string{"XZ-Direction##" + std::to_string(i)}.c_str(), (float *)&m_renderEngine->gerstnerWaves.at(i)->xzDirection_D, -1.0f, 1.0f))
                        {
                            // force-clamp (handle CTRL + LEFT_CLICK)
                            m_renderEngine->gerstnerWaves.at(i)->xzDirection_D = glm::clamp(m_renderEngine->gerstnerWaves.at(i)->xzDirection_D, glm::vec2{-1.0f, -1.0f}, glm::vec2{1.0f, 1.0f});
                            // TODO: use an epsilon???
                            //  we can't normalize the 0 vector, so reset to a dummy
                            if (0.0f == glm::length(m_renderEngine->gerstnerWaves.at(i)->xzDirection_D))
                                m_renderEngine->gerstnerWaves.at(i)->xzDirection_D = glm::vec2{0.0f, 1.0f};
                            else
                                m_renderEngine->gerstnerWaves.at(i)->xzDirection_D = glm::normalize(m_renderEngine->gerstnerWaves.at(i)->xzDirection_D);
                        }
                        ImGui::Separator();
                        ImGui::TreePop();
                    }
                }
                ImGui::Separator();
                ImGui::TreePop();
            }
            ImGui::Separator();
        }

        if (ImGui::SliderFloat("WATER BUMP ROUGHNESS", &m_renderEngine->heightmapSampleScale, 0.0f, 1.0f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            if (m_renderEngine->heightmapSampleScale < 0.0f)
                m_renderEngine->heightmapSampleScale = 0.0f;
        }

        if (ImGui::SliderFloat("WATER BUMP STEEPNESS", &m_renderEngine->heightmapDisplacementScale, 0.0f, 1.0f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            if (m_renderEngine->heightmapDisplacementScale < 0.0f)
                m_renderEngine->heightmapDisplacementScale = 0.0f;
        }

        if (ImGui::SliderFloat("VERTICAL-BOUNCE-WAVE AMPLITUDE", &m_renderEngine->verticalBounceWaveAmplitude, 0.0f, 1.0f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            if (m_renderEngine->verticalBounceWaveAmplitude < 0.0f)
                m_renderEngine->verticalBounceWaveAmplitude = 0.0f;
        }

        if (ImGui::SliderFloat("VERTICAL-BOUNCE-WAVE PHASE", &m_renderEngine->verticalBounceWavePhase, 0.0f, 1.0f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            m_renderEngine->verticalBounceWavePhase = glm::clamp(m_renderEngine->verticalBounceWavePhase, 0.0f, 1.0f);
        }

        // TODO: once i figure out why the Gerstner waves decay over time, change the max here to an appropriate value (and maybe clamp or mod???)
        //       maybe there is a looping here that I can take advantage of
        // if (ImGui::SliderFloat("WAVE-ANIMATION TIME (s)", &m_renderEngine->waveAnimationTimeInSeconds, 0.0f, std::numeric_limits<float>::max())) {
        if (ImGui::SliderFloat("WAVE-ANIMATION TIME (s)", &m_renderEngine->waveAnimationTimeInSeconds, 0.0f, 3600.0f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            if (m_renderEngine->waveAnimationTimeInSeconds < 0.0f)
                m_renderEngine->waveAnimationTimeInSeconds = 0.0f;
        }

        if (ImGui::Button("TOGGLE ANIMATION##1"))
        {
            m_renderEngine->isAnimatingWaves = !m_renderEngine->isAnimatingWaves;
        }
        ImGui::SameLine();

        ImGui::PushItemWidth(300.0f);
        if (ImGui::SliderFloat("ANIMATION SPEED (VERTICAL-BOUNCE-WAVE PERIOD (s))", &m_renderEngine->animationSpeedVerticalBounceWavePhasePeriodInSeconds, 0.0f, 60.0f))
        {
            // force-clamp (handle CTRL + LEFT_CLICK)
            if (m_renderEngine->animationSpeedVerticalBounceWavePhasePeriodInSeconds < 0.0f)
                m_renderEngine->animationSpeedVerticalBounceWavePhasePeriodInSeconds = 0.0f;
        }
        ImGui::PopItemWidth();

        // TODO: refactor this into its own function somewhere else...
        // TODO: check if this ImGui framerate is applicable here (or is it an average of several frames???)
        if (m_renderEngine->isAnimatingWaves)
        {
            float const deltaTimeInSeconds = 1.0f / ImGui::GetIO().Framerate;

            m_renderEngine->waveAnimationTimeInSeconds += deltaTimeInSeconds;
            // handle overflow...
            if (m_renderEngine->waveAnimationTimeInSeconds < 0.0f)
                m_renderEngine->waveAnimationTimeInSeconds = 0.0f;

            if (m_renderEngine->animationSpeedVerticalBounceWavePhasePeriodInSeconds > 0.0f)
            {
                float const deltaVerticalBounceWavePhase = (1.0f / m_renderEngine->animationSpeedVerticalBounceWavePhasePeriodInSeconds) * deltaTimeInSeconds;
                m_renderEngine->verticalBounceWavePhase += deltaVerticalBounceWavePhase;
                m_renderEngine->verticalBounceWavePhase = glm::mod(m_renderEngine->verticalBounceWavePhase, 1.0f);
            }
        }

        // if (ImGui::SliderFloat("TINT DEPTH THRESHOLD", &m_renderEngine->tintDeltaDepthThreshold, 0.0f, 1.0f))
        // {
        //     // force-clamp (handle CTRL + LEFT_CLICK)
        //     m_renderEngine->tintDeltaDepthThreshold = glm::clamp(m_renderEngine->tintDeltaDepthThreshold, 0.0f, 1.0f);
        // }

        // if (ImGui::SliderFloat("WATER CLARITY", &m_renderEngine->waterClarity, 0.0f, 1.0f))
        // {
        //     // force-clamp (handle CTRL + LEFT_CLICK)
        //     m_renderEngine->waterClarity = glm::clamp(m_renderEngine->waterClarity, 0.0f, 1.0f);
        // }

        // if (ImGui::SliderFloat("SOFT-EDGES DEPTH THRESHOLD", &m_renderEngine->softEdgesDeltaDepthThreshold, 0.0f, 1.0f))
        // {
        //     // force-clamp (handle CTRL + LEFT_CLICK)
        //     m_renderEngine->softEdgesDeltaDepthThreshold = glm::clamp(m_renderEngine->softEdgesDeltaDepthThreshold, 0.0f, 1.0f);
        // }
        ImGui::Separator();

        ImGui::End();

        ImGui::EndFrame();
    }

    bool Program::cleanup()
    {
        // Dear ImGui cleanup...
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // glfw cleanup...
        if (nullptr != m_window)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
        return true;
    }

    // NOTE: this method should only be called ONCE at start
    void Program::initScene()
    {
        // CREATE THE 3 PLANES...

        // draw a symmetrical grid for each cartesian plane...

        // TODO: should probably change this to read from variable in render engine that could store the far clip-plane distance
        // NOTE: compare this to far clipping plane distance of 100
        // NOTE: all these should be the same
        int const maxX{100};
        int const maxY{100};
        int const maxZ{100};
        // NOTE: any change here should be reflected in the ImGui notice
        int const deltaX{1};
        int const deltaY{1};
        int const deltaZ{1};

        // TODO: is it possible to mirror the skybox textures on loading them in (since we are inside the cube), but keeping the proper orientation???
        //  hard-coded skyboxes...

        // this will hold the skybox geometry (cube) and star skybox cubemap
        m_skyboxStars = ObjectLoader::createTriMeshObject("../../assets/models/imports/cube.obj", true, true);
        if (nullptr != m_skyboxStars)
        {
            m_skyboxStars->textureID = m_renderEngine->loadCubemap({"../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-stars-2048/right.png",
                                                                    "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-stars-2048/left.png",
                                                                    "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-stars-2048/top.png",
                                                                    "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-stars-2048/bottom.png",
                                                                    "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-stars-2048/front.png",
                                                                    "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-stars-2048/back.png"});

            if (nullptr != m_skyboxStars)
            {
                m_skyboxStars->shaderProgramID = m_renderEngine->getSkyboxStarsProgram();
                m_renderEngine->assignBuffers(*m_skyboxStars);
            }
        }

        // skysphere...
        m_skysphere = ObjectLoader::createTriMeshObject("../../assets/models/imports/icosphere.obj", true, true);
        if (nullptr != m_skysphere)
        {
            m_skysphere->textureID = m_renderEngine->load1DTexture("../../assets/textures/sky-gradient.png");
            // fallback #1 (no skysphere)
            if (0 == m_skysphere->textureID)
                m_skysphere = nullptr;
            if (nullptr != m_skysphere)
            {
                m_skysphere->shaderProgramID = m_renderEngine->getSkysphereProgram();
                m_renderEngine->assignBuffers(*m_skysphere);
            }
        }

        // this will hold the skybox geometry (cube) and cloud skybox cubemap
        m_skyboxClouds = ObjectLoader::createTriMeshObject("../../assets/models/imports/cube.obj", true, true);
        if (nullptr != m_skyboxClouds)
        {
            m_skyboxClouds->textureID = m_renderEngine->loadCubemap({"../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-nebulae-2048/DaylightBox_Right.bmp",
                                                                     "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-nebulae-2048/DaylightBox_Left.bmp",
                                                                     "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-nebulae-2048/DaylightBox_Top.bmp",
                                                                     "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-nebulae-2048/DaylightBox_Bottom.bmp",
                                                                     "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-nebulae-2048/DaylightBox_Front.bmp",
                                                                     "../../assets/textures/skyboxes/wwwtyro-space-3d/2drp4i9sx0lc-nebulae-2048/DaylightBox_Back.bmp"});

            m_skyboxClouds->shaderProgramID = m_renderEngine->getSkyboxCloudsProgram();
            m_renderEngine->assignBuffers(*m_skyboxClouds);
        }

        m_waterGrid = std::make_shared<MeshObject>();
        // m_waterGrid->m_polygonMode = PolygonMode::POINT; //NOTE: doing this atm makes a cool pixel art world
        // TEMP: hacking some indices together to draw grid as tri-mesh (should move this to MeshObject in the future)...
        // TODO: explain this better in the future (with diagrams)

        // first, store indices into a length * length square grid
        // NOTE: atm, this must match the constant of the same name in render-engine.cpp, but will be changed in the future
        GLuint const GRID_LENGTH = 513;
        // NOTE: must use vector instead of array to handle larger grid lengths
        //  zero-fill
        std::vector<std::vector<GLuint>> gridIndices(GRID_LENGTH, std::vector<GLuint>(GRID_LENGTH, 0));
        // now fill with the proper indices in the same layout that shader expects
        // not that it really matters, but visualize it as [row][col] = [0][0] as the bottom-left element of 2D array
        GLuint counterIndex = 0;
        for (GLuint row = 0; row < GRID_LENGTH; ++row)
        {
            for (GLuint col = 0; col < GRID_LENGTH; ++col)
            {
                gridIndices.at(row).at(col) = counterIndex;
                ++counterIndex;
            }
        }

        // now using the vertex indices in this format, we can easily tesselate this grid into triangles as so...
        // TODO: draw a diagram comment here to better explain this
        for (GLuint row = 0; row < GRID_LENGTH - 1; ++row)
        {
            for (GLuint col = 0; col < GRID_LENGTH - 1; ++col)
            {
                // make 2 triangles (thus a square) from each of these indices acting as the bottom-left corner
                // ensures that the winding of all triangles is counter-clockwise

                m_waterGrid->drawFaces.push_back(gridIndices.at(row).at(col));
                m_waterGrid->drawFaces.push_back(gridIndices.at(row + 1).at(col + 1));
                m_waterGrid->drawFaces.push_back(gridIndices.at(row + 1).at(col));

                m_waterGrid->drawFaces.push_back(gridIndices.at(row).at(col));
                m_waterGrid->drawFaces.push_back(gridIndices.at(row).at(col + 1));
                m_waterGrid->drawFaces.push_back(gridIndices.at(row + 1).at(col + 1));
            }
        }

        m_waterGrid->textureID = m_renderEngine->load2DTexture("../../assets/textures/noise/waves/waves3/00.png"); // WARNING: THIS MAY HAVE TO BE CHANGED TO LOAD IN SPECIFICALLY WITH 8-bits (or may work, but should be optimized)
        // fallback #1 (no water grid)
        if (0 == m_waterGrid->textureID)
            m_waterGrid = nullptr;
        if (nullptr != m_waterGrid)
        {
            m_waterGrid->shaderProgramID = m_renderEngine->getWaterGridProgram();
            m_renderEngine->assignBuffers(*m_waterGrid);
        }
    }

    void Program::queryGLVersion()
    {
        // query OpenGL version and renderer information
        std::string const GLV = reinterpret_cast<char const *>(glGetString(GL_VERSION));
        std::string const GLSLV = reinterpret_cast<char const *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
        std::string const GLR = reinterpret_cast<char const *>(glGetString(GL_RENDERER));

        std::cout << "OpenGL [ " << GLV << " ] " << "with GLSL [ " << GLSLV << " ] " << "on renderer [ " << GLR << " ]" << std::endl;
    }

    bool Program::setupWindow()
    {
        // initialize the GLFW windowing system
        if (!glfwInit())
        {
            std::cout << "ERROR: GLFW failed to initialize, TERMINATING..." << std::endl;
            return false;
        }

        // set the custom error callback function
        // errors will be printed to the console
        glfwSetErrorCallback(errorCallback);

        // attempt to create a window with an OpenGL 4.1 core profile context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // reference: https://stackoverflow.com/questions/42848322/what-does-my-choice-of-glfw-samples-actually-do
        // glfwWindowHint(GLFW_SAMPLES, 4);
        // glEnable(GL_MULTISAMPLE);
        int const WIDTH = 1024;
        int const HEIGHT = 1024;
        m_window = glfwCreateWindow(WIDTH, HEIGHT, "WaveTool", nullptr, nullptr);
        if (!m_window)
        {
            std::cout << "ERROR: Program failed to create GLFW window, TERMINATING..." << std::endl;
            glfwTerminate();
            return false;
        }

        // so that we can access this program object in callbacks...
        glfwSetWindowUserPointer(m_window, this);

        // set callbacks...
        glfwSetCursorPosCallback(m_window, InputHandler::motion);
        // glfwSetKeyCallback(m_window, keyCallback);
        glfwSetKeyCallback(m_window, InputHandler::key);
        glfwSetMouseButtonCallback(m_window, InputHandler::mouse);
        glfwSetScrollCallback(m_window, InputHandler::scroll);
        // glfwSetWindowSizeCallback(m_window, windowSizeCallback);
        glfwSetWindowSizeCallback(m_window, InputHandler::reshape);

        // bring the new window to the foreground (not strictly necessary but convenient)
        glfwMakeContextCurrent(m_window);
        // enable VSync
        glfwSwapInterval(0);

        // reference: https://www.khronos.org/opengl/wiki/OpenGL_Loading_Library#glad_.28Multi-Language_GL.2FGLES.2FEGL.2FGLX.2FWGL_Loader-Generator.29
        // glad uses GLFW loader to find appropriate OpenGL config (load OpenGL functions) for your system
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "ERROR: Failed to initialize OpenGL context, TERMINATING..." << std::endl;
            glfwTerminate();
            return false;
        }

        // reference: https://blog.conan.io/2019/06/26/An-introduction-to-the-Dear-ImGui-library.html
        // setup Dear ImGui context...
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        // setup platform/renderer bindings...
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        char const *glsl_version = "#version 410 core";
        // NOTE: must init glad before this call to not get an exception
        //  reference: https://stackoverflow.com/questions/48582444/imgui-with-the-glad-opengl-loader-throws-segmentation-fault-core-dumped
        ImGui_ImplOpenGL3_Init(glsl_version);
        // set UI style...
        ImGui::StyleColorsDark();

        // query and print out information about our OpenGL environment
        queryGLVersion();

        return true;
    }

    // 랜덤 생성을 위한 함수
    glm::vec2 Program::getRandomDirection()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

        glm::vec2 dir(dis(gen), dis(gen));
        return glm::normalize(dir);
    }

    float Program::getRandomFloat(float min, float max)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    }

    void errorCallback(int error, char const *description)
    {
        std::cout << "GLFW ERROR: " << error << ":" << std::endl;
        std::cout << description << std::endl;
    }

    /*
        void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
            Program *program = (Program*)glfwGetWindowUserPointer(window);

            // key codes are often prefixed with GLFW_KEY_ and can be found on the GLFW website
            if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
                glfwSetWindowShouldClose(window, GL_TRUE);
            }
        }
    */

    /*
        void windowSizeCallback(GLFWwindow *window, int width, int height) {
            glViewport(0, 0, width, height);
        }
    */
}
