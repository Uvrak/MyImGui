// Application.cpp
#include "Application.h"
#include "WorldSettings.h"

#include <MyImGui.h>

#include <SDL3/SDL.h>
#include <stdexcept>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <array>

namespace ow3d
{
    Application::Application() = default;
    Application::~Application() = default;

    bool Application::initialize(
        WorldManager::TileWorldInitializer setupWorld,
        const std::function<void(Renderer&)>& setupRenderer,
        WorldManager::WorldFactory worldFactory)
    {
        if (!m_window.create("OpenWorld3D", 1280, 720, true))
            return false;

        if (!m_renderer.initialize())
            return false;

        if (setupRenderer)
            setupRenderer(m_renderer);

        m_worldManager.setTileWorldInitializer(std::move(setupWorld));
        m_worldManager.setTileWorldFactory(std::move(worldFactory));

        int viewportWidth = m_renderer.viewportWidth();
        int viewportHeight = m_renderer.viewportHeight();

        int worldMode = 0;

        m_window.loadState(
            "window.cfg",
            viewportWidth,
            viewportHeight,
            worldMode
        );

        m_renderer.resizeViewport(
            viewportWidth,
            viewportHeight
        );

        m_worldManager.setMode(
            worldMode == 1
            ? WorldMode::TileBased
            : WorldMode::Normal
        );

        if (!SDL_SetWindowRelativeMouseMode(m_window.handle(), false))
        {
            SDL_Log("Could not disable relative mouse mode: %s", SDL_GetError());
        }

        if (!MyImGui::initialize(m_window.handle()))
            return false;

        return true;
    }

    void Application::run()
    {
        if (!m_capturePrefix.empty())
        {
            // Deterministic captures exercise the real shaders, textures and meshes.
            m_renderer.resizeViewport(1600, 1000);
            for (int frame = 0; frame < (m_captureUiTest ? 150 : m_captureHoverTest ? 3 : 2); ++frame)
            {
                m_renderer.setRenderTime(m_captureHoverTest || frame == 0 ? 0.5f : 2.5f);
                if (m_captureHoverTest)
                {
                    m_renderer.updateSceneHover(0.98f, 0.98f, true);
                    if (m_renderer.sceneRoofHidden()) throw std::runtime_error("Roof hides outside house.");
                    const glm::vec4 projected = m_renderer.camera().projectionMatrix() *
                        m_renderer.camera().viewMatrix() * glm::vec4(m_renderer.sceneCenter(), 1.0f);
                    m_renderer.updateSceneHover(projected.x / projected.w, projected.y / projected.w, frame == 1);
                    if (m_renderer.sceneRoofHidden() != (frame == 1))
                        throw std::runtime_error("Roof hover capture check failed.");
                    if (frame == 1)
                    {
                        m_renderer.updateSceneHover(projected.x / projected.w, projected.y / projected.w, true);
                        if (!m_renderer.sceneRoofHidden()) throw std::runtime_error("Roof hover flickers.");
                    }
                }
                if (m_renderer.character().loaded()) {
                    auto& actor=m_renderer.character();
                    // Keep the selected preview gait/distance, advance its cycle only.
                    actor.advancePreview(frame == 0 ? .1f : .4f);
                    actor.updateCamera(m_renderer.camera());
                }
                if(m_captureUiTest) {
                    // The test pauses between events to render screenshots. Allow
                    // that overhead; normal interactive double-click timing is unchanged.
                    ImGui::GetIO().MouseDoubleClickTime=2.f;
                    ImGui::GetIO().MouseSingleClickDelay=2.1f;
                    MyImGui::beginFrame();
                    ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({1200,650});
                    ImGui::Begin("Outfit input check",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
                    if(m_uiDraw)m_uiDraw(0,0,1200,650);
                    ImGui::End();MyImGui::endFrame();
                }
                m_worldManager.update(0.0f);
                m_renderer.beginFrame();
                m_worldManager.render(m_renderer);
                if (m_worldManager.mode() == WorldMode::TileBased) { m_renderer.drawSceneObjects();m_renderer.drawCharacter();m_renderer.drawSceneGlass(); }
                const char* hoverNames[] = { "-roof-visible.bmp", "-roof-hidden.bmp", "-roof-restored.bmp" };
                const auto path = m_capturePrefix + (m_captureHoverTest ? hoverNames[frame] : (frame == 0 ? "-t0.bmp" : "-t1.bmp"));
                if (!m_renderer.saveScreenshot(path.c_str()))
                    throw std::runtime_error("Could not write capture: " + path);
                if (m_renderer.character().loaded()) SDL_Log("Character capture: LOD %d, gait %d", m_renderer.character().lod(), m_renderer.character().gait());
                m_renderer.endFrame();
            }
            m_renderer.setRenderTime(-1.0f);
            return;
        }
        // Present interactive windows only after loading; captures remain hidden
        // from creation and never briefly take focus from the desktop/game.
        SDL_ShowWindow(m_window.handle());
        SDL_RaiseWindow(m_window.handle());
        SDL_Event event{};

        Uint64 previousTicks = SDL_GetTicks();

        bool mouseCaptured = false;
        float cameraGrabX=0,cameraGrabY=0;
        auto releaseCameraMouse=[&](bool restorePointer){
            if(!mouseCaptured)return;
            mouseCaptured=false;
            SDL_SetWindowRelativeMouseMode(m_window.handle(),false);
            SDL_SetWindowMouseGrab(m_window.handle(),false);
            SDL_CaptureMouse(false);
            if(restorePointer && SDL_GetKeyboardFocus()==m_window.handle()){
                int width=0,height=0;SDL_GetWindowSize(m_window.handle(),&width,&height);
                SDL_WarpMouseInWindow(m_window.handle(),std::clamp(cameraGrabX,1.f,float(std::max(1,width-2))),
                    std::clamp(cameraGrabY,1.f,float(std::max(1,height-2))));
            }
            if(m_renderer.character().follow)m_renderer.character().resetCameraTilt();
        };
        const char* profilePath=SDL_getenv("U7R_FRAME_PROFILE");
        std::vector<std::array<double,8>> frameSamples;unsigned profileFrames=0;
        std::array<Uint64,9> profileTicks{};
        auto profileMark=[&](int i){if(profilePath)profileTicks[i]=SDL_GetTicksNS();};

        while (m_running)
        {
            profileMark(0);
            const Uint64 currentTicks = SDL_GetTicks();

            const float deltaTime =
                static_cast<float>(currentTicks - previousTicks) / 1000.0f;

            previousTicks = currentTicks;

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_MOUSE_WHEEL && mouseCaptured &&
                    event.wheel.windowID == SDL_GetWindowID(m_window.handle()) &&
                    (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK))
                {
                    const float steps = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                        ? -event.wheel.y : event.wheel.y;
                    if (m_renderer.character().loaded() && m_renderer.character().follow) m_renderer.character().zoom(steps);
                    else m_renderer.camera().zoomFree(steps);
                    // A camera zoom gesture must not also scroll the UI underneath.
                    continue;
                }
                MyImGui::processEvent(event);

                if (event.type == SDL_EVENT_QUIT)
                {
                    m_running = false;
                }

                const auto windowId=SDL_GetWindowID(m_window.handle());
                const bool releaseButton=event.type==SDL_EVENT_MOUSE_BUTTON_UP &&
                    event.button.windowID==windowId && event.button.button==SDL_BUTTON_RIGHT;
                const bool escape=event.type==SDL_EVENT_KEY_DOWN && event.key.windowID==windowId &&
                    event.key.scancode==SDL_SCANCODE_ESCAPE;
                const bool lostFocus=(event.type==SDL_EVENT_WINDOW_FOCUS_LOST || event.type==SDL_EVENT_WINDOW_MINIMIZED) &&
                    event.window.windowID==windowId;
                if(releaseButton || escape || lostFocus || event.type==SDL_EVENT_QUIT)
                    releaseCameraMouse(!lostFocus && event.type!=SDL_EVENT_QUIT);

            }

            // Recover even if an OS menu/focus change swallowed the button-up event.
            if(mouseCaptured && (SDL_GetKeyboardFocus()!=m_window.handle() ||
               !(SDL_GetMouseState(nullptr,nullptr)&SDL_BUTTON_RMASK)))
                releaseCameraMouse(SDL_GetKeyboardFocus()==m_window.handle());

            m_window.saveState(
                "window.cfg",
                m_renderer.viewportWidth(),
                m_renderer.viewportHeight(),
                m_worldManager.mode() == WorldMode::TileBased ? 1 : 0
            );

            m_renderer.updateOpenings(deltaTime);
            const bool* keys = SDL_GetKeyboardState(nullptr);
            const auto mouseButtons = SDL_GetMouseState(nullptr, nullptr);
            constexpr auto walkButtons = SDL_BUTTON_LMASK | SDL_BUTTON_RMASK;
            const bool mouseWalking = mouseCaptured && (mouseButtons & walkButtons) == walkButtons;

            constexpr float moveSpeed = 2.5f;
            constexpr float turnSpeed = 90.0f;

            const bool playerControl = m_renderer.character().loaded() && m_renderer.character().follow && m_worldManager.mode() == WorldMode::TileBased;
            if (playerControl) {
                float dx=0, dy=0;
                if (mouseCaptured) SDL_GetRelativeMouseState(&dx, &dy);
                const bool keyboard = mouseCaptured && !ImGui::GetIO().WantTextInput;
                if (keyboard && keys[SDL_SCANCODE_LEFT]) dx -= turnSpeed * deltaTime / .15f;
                if (keyboard && keys[SDL_SCANCODE_RIGHT]) dx += turnSpeed * deltaTime / .15f;
                m_renderer.character().update(deltaTime, mouseWalking || (keyboard && keys[SDL_SCANCODE_W]), dx, dy);
                m_renderer.character().updateCamera(m_renderer.camera(),deltaTime);
            } else {
            if (m_renderer.character().loaded()) m_renderer.character().update(deltaTime, false, 0, 0);
            // Vor / zurück
            if (!mouseWalking && (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]))
            {
                m_renderer.camera().moveForward(
                    moveSpeed * deltaTime
                );
            }

            if (!mouseWalking && (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]))
            {
                m_renderer.camera().moveForward(
                    -moveSpeed * deltaTime
                );
            }

            // Drehen per Tastatur
            if (keys[SDL_SCANCODE_LEFT])
            {
                m_renderer.camera().rotateYaw(
                    -turnSpeed * deltaTime
                );
            }

            if (keys[SDL_SCANCODE_RIGHT])
            {
                m_renderer.camera().rotateYaw(
                    turnSpeed * deltaTime
                );
            }

            // Seitlich bewegen
            if (keys[SDL_SCANCODE_A])
            {
                m_renderer.camera().moveRight(
                    -moveSpeed * deltaTime
                );
            }

            if (keys[SDL_SCANCODE_D])
            {
                m_renderer.camera().moveRight(
                    moveSpeed * deltaTime
                );
            }

            // Hoch / runter
            if (!mouseWalking && keys[SDL_SCANCODE_Q])
            {
                m_renderer.camera().moveUp(
                    moveSpeed * deltaTime
                );
            }

            if (!mouseWalking && keys[SDL_SCANCODE_E])
            {
                m_renderer.camera().moveUp(
                    -moveSpeed * deltaTime
                );
            }

            // Maus
            if (mouseCaptured)
            {
                float mouseX = 0.0f;
                float mouseY = 0.0f;

                SDL_GetRelativeMouseState(&mouseX, &mouseY);

                constexpr float mouseSensitivity = 0.15f;

                m_renderer.camera().rotateYaw(
                    mouseX * mouseSensitivity
                );

                m_renderer.camera().rotatePitch(
                    -mouseY * mouseSensitivity
                );
            }

            if (mouseWalking)
                m_renderer.camera().walkForward(moveSpeed * std::min(deltaTime, 0.05f));
            }

            profileMark(1);
            m_worldManager.update(deltaTime);

            m_renderer.beginFrame();

            profileMark(2);
            MyImGui::beginFrame();

            MyImGui::beginDockspace();

            m_mainMenu.draw(
                m_window,
                m_resolutionWindow,
                m_renderer
            );

            m_resolutionWindow.draw(
                m_renderer
            );

            if (m_settingsVisible) {
            if (ImGui::Begin("Settings",&m_settingsVisible))
            {
                ImGui::Text("OpenWorld3D Settings");
                if(m_settingsDraw){
                    ImGui::PushID("WorldSettings");
                    m_settingsDraw();
                    ImGui::PopID();
                }
                if (m_renderer.character().loaded()) {
                    auto& actor=m_renderer.character();
                    ImGui::Checkbox("Sir Canegm folgen", &actor.follow);
                    ImGui::Text("Mittelklick im Bild: Folgen / freie Kamera");
                    if(ImGui::Checkbox("Draufsicht", &actor.topDown))actor.setTopDownView(actor.topDown,m_renderer.camera());
                    if(!actor.follow)ImGui::Text("Freie Kamera: links ziehen | RMB: drehen | Mausrad: Zoom");
                    else ImGui::Text("RMB: umsehen | RMB + LMB: gehen | Mausrad: Abstand");
                    ImGui::Text("Maus vor: schnell gehen / laufen / schnell laufen");
                }
            bool tileBased =
                (m_worldManager.mode() == WorldMode::TileBased);

            if (ImGui::Checkbox("Tile Based World", &tileBased))
            {
                m_worldManager.setMode(
                    tileBased
                    ? WorldMode::TileBased
                    : WorldMode::Normal
                );
            }

            ImGui::Text(
                "Current World Mode: %s",
                m_worldManager.mode() == WorldMode::TileBased
                ? "Tile Based"
                : "Normal"
            );

            }
            ImGui::End();
            }

            static MyImGui::FloatingWindow viewportWindow("Viewport");

            const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(
                ImVec2(mainViewport->WorkPos.x + 20.0f, mainViewport->WorkPos.y + 150.0f),
                ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(
                ImVec2(mainViewport->WorkSize.x * 0.8f, mainViewport->WorkSize.y * 0.7f),
                ImGuiCond_FirstUseEver);
            m_renderer.updateSceneHover(0.0f, 0.0f, false);
            if (viewportWindow.begin())
            {
                const ImVec2 availableSize =
                    ImGui::GetContentRegionAvail();

                const float renderAspect =
                    static_cast<float>(m_renderer.viewportWidth()) /
                    static_cast<float>(m_renderer.viewportHeight());

                float imageWidth = availableSize.x;
                float imageHeight = imageWidth / renderAspect;

                if (imageHeight > availableSize.y)
                {
                    imageHeight = availableSize.y;
                    imageWidth = imageHeight * renderAspect;
                }

                const float offsetX =
                    (availableSize.x - imageWidth) * 0.5f;

                const float offsetY =
                    (availableSize.y - imageHeight) * 0.5f;

                if (offsetX > 0.0f)
                    ImGui::SetCursorPosX(
                        ImGui::GetCursorPosX() + offsetX
                    );

                if (offsetY > 0.0f)
                    ImGui::SetCursorPosY(
                        ImGui::GetCursorPosY() + offsetY
                    );

                ImGui::Image(
                    static_cast<ImTextureID>(
                        m_renderer.colorTexture()
                        ),
                    ImVec2(imageWidth, imageHeight),
                    ImVec2(0.0f, 1.0f),
                    ImVec2(1.0f, 0.0f)
                );
                const bool imageHovered = ImGui::IsItemHovered();
                const ImVec2 corner = ImGui::GetItemRectMin();
                bool overlayHovered = m_uiDraw && m_uiDraw(corner.x, corner.y, corner.x+imageWidth, corner.y+imageHeight);
                if (m_framerateVisible) {
                    // Use ImGui's rolling frame-rate average; the readout has no render queries.
                    char label[32];std::snprintf(label,sizeof(label),"%.0f FPS",ImGui::GetIO().Framerate);
                    const ImVec2 pos(corner.x + imageWidth - 110, corner.y + 8);
                    const ImVec2 textSize=ImGui::CalcTextSize(label);
                    auto* draw=ImGui::GetWindowDrawList();
                    draw->AddRectFilled(pos,ImVec2(pos.x+102,pos.y+26),ImGui::GetColorU32(ImGuiCol_Button),ImGui::GetStyle().FrameRounding);
                    draw->AddText(ImVec2(pos.x+(102-textSize.x)*.5f,pos.y+(26-textSize.y)*.5f),ImGui::GetColorU32(ImGuiCol_Text),label);
                } else if (m_renderer.character().loaded()) {
                    ImGui::SetCursorScreenPos(ImVec2(corner.x + imageWidth - 110, corner.y + 8));
                    auto& actor=m_renderer.character();
                    if (ImGui::Button(actor.topDown ? "Schraegansicht" : "Draufsicht", ImVec2(102, 26))) actor.setTopDownView(!actor.topDown,m_renderer.camera());
                    overlayHovered = overlayHovered || ImGui::IsItemHovered();
                }
                if(!mouseCaptured && imageHovered && !overlayHovered && !ImGui::GetDragDropPayload() &&
                    !ImGui::IsAnyItemActive() && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
                    auto& actor=m_renderer.character();
                    if(actor.loaded()){
                        actor.follow=!actor.follow;
                        if(actor.follow)actor.updateCamera(m_renderer.camera());
                    }
                }
                const bool freeCamera=!m_renderer.character().follow || !m_renderer.character().loaded();
                if(freeCamera && !mouseCaptured && !overlayHovered && !ImGui::GetDragDropPayload()){
                    // Start on empty viewport space; overlays and clickable objects retain their clicks.
                    const auto savedCursor=ImGui::GetCursorScreenPos();
                    ImGui::SetCursorScreenPos(corner);
                    ImGui::InvisibleButton("Free camera pan",{imageWidth,imageHeight},ImGuiButtonFlags_MouseButtonLeft);
                    if(ImGui::IsItemHovered() || ImGui::IsItemActive())ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                    if(ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left,3.f)){
                        const auto delta=ImGui::GetIO().MouseDelta;
                        m_renderer.camera().panPixels(delta.x,delta.y,imageHeight);
                    }
                    if(ImGui::IsItemHovered() && !ImGui::IsItemActive())ImGui::SetTooltip("Links ziehen: Ansicht verschieben | Mittelklick: Sir Canegm folgen");
                    ImGui::SetCursorScreenPos(savedCursor);
                }
                // Uncaptured wheel input uses this frame's image and overlay hit tests.
                // Captured wheel events were already consumed above, so never zoom twice.
                if (!mouseCaptured && imageHovered && !overlayHovered &&
                    !ImGui::IsAnyItemActive() && ImGui::GetIO().MouseWheel != 0.0f)
                {
                    const float steps = ImGui::GetIO().MouseWheel;
                    auto& actor = m_renderer.character();
                    if (actor.loaded() && actor.follow)
                    {
                        actor.zoom(steps);
                        actor.updateCamera(m_renderer.camera());
                    }
                    else m_renderer.camera().zoomFree(steps);
                }
                if (!mouseCaptured && !overlayHovered && imageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Right) && SDL_GetKeyboardFocus()==m_window.handle())
                {
                    SDL_GetMouseState(&cameraGrabX,&cameraGrabY);
                    if (SDL_SetWindowRelativeMouseMode(m_window.handle(), true))
                    {
                        SDL_SetWindowMouseGrab(m_window.handle(),true);
                        if(m_renderer.character().follow && m_viewportControlActivated)m_viewportControlActivated();
                        mouseCaptured = true;
                        // Discard motion collected while the pointer was freely moving.
                        SDL_GetRelativeMouseState(nullptr, nullptr);
                    }
                }
                if (!mouseCaptured && m_worldManager.mode() == WorldMode::TileBased &&
                    !overlayHovered && imageWidth > 0.0f && imageHeight > 0.0f && imageHovered)
                {
                    const ImVec2 mouse = ImGui::GetMousePos();
                    const float ndcX = 2.0f * (mouse.x - corner.x) / imageWidth - 1.0f;
                    const float ndcY = 1.0f - 2.0f * (mouse.y - corner.y) / imageHeight;
                    m_renderer.updateSceneHover(ndcX, ndcY, true);
                    if(!ImGui::GetDragDropPayload() && !ImGui::IsAnyItemActive()) {
                        int opening=m_renderer.pickOpening(ndcX,ndcY);
                        if(opening>=0) {
                            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                            ImGui::SetTooltip("%s",m_renderer.openings().hint(opening));
                            // Claim the click so it cannot drag the floating viewport window.
                            const auto savedCursor=ImGui::GetCursorScreenPos();
                            ImGui::SetCursorScreenPos(corner);
                            ImGui::InvisibleButton("House opening interaction",{imageWidth,imageHeight});
                            if(ImGui::IsItemClicked(ImGuiMouseButton_Left))m_renderer.openings().toggle(opening);
                            ImGui::SetCursorScreenPos(savedCursor);
                        }
                    }
                }
            }

            viewportWindow.end();
            if (m_windowDraw) m_windowDraw();

            profileMark(3);
            // Use this frame's image rectangle and pointer position, including letterboxing.
            m_worldManager.render(m_renderer);
            profileMark(4);
            if (m_worldManager.mode() == WorldMode::TileBased) { m_renderer.drawSceneObjects();profileMark(5);m_renderer.drawCharacter();m_renderer.drawSceneGlass(); }
            else profileMark(5);

            profileMark(6);
            m_renderer.endFrame();

            MyImGui::endFrame();
            profileMark(7);

            m_window.swapBuffers();
            profileMark(8);
            if(profilePath){
                if(++profileFrames>60){std::array<double,8> row{};for(int i=0;i<8;++i)row[i]=double(profileTicks[i+1]-profileTicks[i])/1e6;frameSamples.push_back(row);}
                if(profileFrames==240){
                    std::ofstream out(profilePath);out<<"input_actor,world_update,ui,world_render,objects,character,ui_render,present (milliseconds)\n";
                    for(const auto& row:frameSamples){for(double value:row)out<<value<<',';out<<'\n';}
                    profilePath=nullptr;
                }
            }
        }
    }
}
