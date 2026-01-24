#include "pch.h"
#include "sCameraController.h"


namespace PAIN {
    MobileMoveAxes g_MobileMoveAxes;
    MobileLookDelta g_MobileLookDelta;

    void sCameraController::onDetach() {}

    void sCameraController::onAttach()
    {
        //get scene
        m_Scene = services->get<Scene::SceneManager>();
    }

#ifdef PN_PLATFORM_ANDROID
    // ANDROID ONLY
    void sCameraController::beginTouchControls(int pointerId, float x, float y) {
#ifdef _DEBUG
        auto editor = services->get<Editor::Editor>();
        bool hasEditor = (editor != nullptr);
#else
        bool hasEditor = false;
#endif

        float screen_center = 0.f;
        if (hasEditor) {
#ifdef _DEBUG
            if (editor->isVisible()) {
                if (!vp_hovered) return;

                float relativeX = x - m_vpPosX;
                float relativeY = y - m_vpPosY;

                if (relativeX < 0 || relativeX > m_vpWidth ||
                    relativeY < 0 || relativeY > m_vpHeight) {
                    return;  // Touch outside viewport
                }
                screen_center = m_vpWidth / 2.f;

                if (relativeX >= screen_center) {
                    if (m_touchLooking) return;

                    m_touchLooking = true;
                    m_touchPointerId = pointerId;
                    m_touchLastX = x;
                    m_touchLastY = y;
                    mouseButtonDown = true; // reuse existing yaw/pitch code path
                }
                if (relativeX < screen_center) {
                    if (m_move.active) return;

                    m_move.active = true;
                    m_move.id = pointerId;
                    m_move.start_x = x;
                    m_move.start_y = y;
                    m_move.last_x = x;
                    m_move.last_y = y;
                }
            }
            else
#endif
            {
                // Editor hidden or release mode
                screen_center = m_surfaceWidth / 2.f;

                if (x >= screen_center) {
                    if (m_touchLooking) return;

                    m_touchLooking = true;
                    m_touchPointerId = pointerId;
                    m_touchLastX = x;
                    m_touchLastY = y;
                    mouseButtonDown = true;
                }
                if (x < screen_center) {
                    if (m_move.active) return;

                    m_move.active = true;
                    m_move.id = pointerId;
                    m_move.start_x = x;
                    m_move.start_y = y;
                    m_move.last_x = x;
                    m_move.last_y = y;
                }
            }
        }
        else {
            // No editor (release mode)
            screen_center = m_surfaceWidth / 2.f;

            if (x >= screen_center) {
                if (m_touchLooking) return;

                m_touchLooking = true;
                m_touchPointerId = pointerId;
                m_touchLastX = x;
                m_touchLastY = y;
                mouseButtonDown = true;
            }
            if (x < screen_center) {
                if (m_move.active) return;

                m_move.active = true;
                m_move.id = pointerId;
                m_move.start_x = x;
                m_move.start_y = y;
                m_move.last_x = x;
                m_move.last_y = y;
            }
        }
    }

    // ANDROID ONLY
    void sCameraController::updateTouchControls(int pointerId, float x, float y) {
        if (m_touchLooking && pointerId == m_touchPointerId) {
            float dx = (x - m_touchLastX);
            float dy = (m_touchLastY - y);

            xOffset += dx;
            yOffset += dy;
            m_touchLastX = x;
            m_touchLastY = y;

            g_MobileLookDelta.dx += dx;
            g_MobileLookDelta.dy += dy;
        }
        if (m_move.active && pointerId == m_move.id) {
            // acts as a jpy stick
            // Compute vector from stick center to current
            float dx = x - m_move.start_x;
            float dy = y - m_move.start_y;

            // if the touch is back at the origin don't move
            float len = sqrtf(dx * dx + dy * dy);
            if (len < m_moveDeadzonePx) {
                dx = 0.f; dy = 0.f; len = 0.f;
            }

            // Clamp to radius
            if (len > m_moveRadiusPx && len > 0.f) {
                float s = m_moveRadiusPx / len;
                dx *= s; dy *= s;
                len = m_moveRadiusPx;
            }

            // Normalize to [-1,1] stick space
            float nx = (m_moveRadiusPx > 0.f) ? (dx / m_moveRadiusPx) : 0.f;
            float ny = (m_moveRadiusPx > 0.f) ? (dy / m_moveRadiusPx) : 0.f;

            // Store last for completeness (if you later want relative gestures)
            m_move.last_x = x;
            m_move.last_y = y;

            // Save the normalized vector for onUpdate movement this frame.
            m_cachedMoveX = nx;
            m_cachedMoveY = ny;

            // for lua
            g_MobileMoveAxes.x = -nx;
            g_MobileMoveAxes.y = ny;
        }
    }

    // ANDROID ONLY
    void sCameraController::endTouchControls(int pointerId) {
        if (m_touchLooking && pointerId == m_touchPointerId) {
            m_touchLooking = false;
            m_touchPointerId = -1;
            mouseButtonDown = false;
        }
        if (m_move.active && pointerId == m_move.id) {
            m_move = MoveStick{};
            m_cachedMoveX = 0.f;
            m_cachedMoveY = 0.f;

            g_MobileMoveAxes.x = 0.f;
            g_MobileMoveAxes.y = 0.f;
        }
    }
#endif

    void sCameraController::onUpdate(AppTiming timing)
    {
        if (!m_Scene) {
            PN_CORE_ERROR("UNABLE TO GET SCENE MANAGER");
            return;
        };

        const float dt = timing.unscaled_dt;

#ifdef _DEBUG
        auto editor = services->get<Editor::Editor>();
        bool editor_visible = editor->isVisible();

        static bool lastEditorVisible = !editor_visible; // Force update on first frame

        if (lastEditorVisible != editor_visible) {
            if (editor_visible) {
                PN_CORE_INFO("EDITOR");
                m_Scene->SetEditorCamera();
            }
            else {
                PN_CORE_INFO("GAME");
                m_Scene->SetGameCamera();
            }
            lastEditorVisible = editor_visible;
        }
#endif 

        camera = m_Scene->GetActiveCamera();
        if (!camera) return;



        // -------- GAME MODE --------
#ifdef _DEBUG
        bool gameMode = !editor_visible;
#else
    // No editor in release, always game mode
        bool gameMode = true;
#endif

#ifdef PN_PLATFORM_WINDOWS
        // Disable cursor (WINDOWS ONLY)
        auto window = services->get<Window::Window>();

        if (gameMode) {
            window->setCursorMode(b_hide_mouse);

        }
        else {
            window->setCursorMode(false);
        }
#endif

        if (gameMode) {
            // active camera is driven by lua (thirdPersonCamera.lua) in game mode
            xOffset = 0.f;
            yOffset = 0.f;
            return;
        }


        // -------- EDITOR CAMERA --------

        // Move left/right based on cross product of forward and up vectors
        glm::vec3 right = glm::normalize(glm::cross(camera->forward, camera->up));

        // Android editor camera
        if (m_move.active && camera) {
            // Project forward to XZ like you already do
            static glm::mat4 mmtx = glm::scale(glm::mat4(1.f), glm::vec3(1, 0, 1));
            glm::vec3 fwd = glm::normalize(glm::vec3(mmtx * glm::vec4(camera->forward, 1.f)));
            /* glm::vec3 right = glm::normalize(glm::cross(camera->forward, camera->up));*/

            // Map: up on stick (negative ny) -> forward, right on stick (positive nx) -> strafe right
            float speed = camera->speed * m_moveScale;
            camera->pos += (-m_cachedMoveY) * fwd * speed * dt;
            camera->pos += (m_cachedMoveX)*right * speed * dt;
        }

        float speedMultiplier = 1.0f;
        if (LSHIFT_KEYDOWN) {
            speedMultiplier = 2.0f; // FAST
        }
        else if (LCTRL_KEYDOWN) {
            speedMultiplier = 0.5f; // SLOW
        }

        float currentMoveSpeed = camera->speed * speedMultiplier * dt;

        switch (move_mode) {
        case FREE_FLY:
            // Move forward/backward directly in camera forward direction
            if (W_KEYDOWN) {
                glm::vec3 offset = camera->forward * currentMoveSpeed;
                camera->pos += offset;
            }
            if (S_KEYDOWN) {
                glm::vec3 offset = camera->forward * currentMoveSpeed;
                camera->pos -= offset;
            }
            if (A_KEYDOWN) {
                camera->pos -= right * currentMoveSpeed;
            }
            if (D_KEYDOWN) {
                camera->pos += right * currentMoveSpeed;
            }

            // Move up/down along camera up vector
            if (E_KEYDOWN) {
                camera->pos += camera->up * currentMoveSpeed;
            }
            if (Q_KEYDOWN) {
                camera->pos -= camera->up * currentMoveSpeed;
            }


            break;

        case FIRST_PERSON:
            static glm::mat4 mmtx = glm::scale(glm::mat4(1.f), glm::vec3(1, 0, 1));
            if (W_KEYDOWN) {
                glm::vec3 offset = glm::vec3(mmtx * glm::vec4(camera->forward, 1.f)) * currentMoveSpeed;
                camera->pos += offset;
            }
            if (S_KEYDOWN) {
                glm::vec3 offset = glm::vec3(mmtx * glm::vec4(camera->forward, 1.f)) * currentMoveSpeed;
                camera->pos -= offset;
            }
            if (A_KEYDOWN) {
                glm::vec3 offset = glm::normalize(glm::cross(camera->forward, camera->up)) * currentMoveSpeed;
                camera->pos -= offset;
            }
            if (D_KEYDOWN) {
                glm::vec3 offset = glm::normalize(glm::cross(camera->forward, camera->up)) * currentMoveSpeed;
                camera->pos += offset;
            }
            //if (SPACE_KEYDOWN) {
            //    glm::vec3 offset = camera->up * currentMoveSpeed;
            //    camera->pos += offset;
            //}
            //if (LCTRL_KEYDOWN) {
            //    glm::vec3 offset = camera->up * currentMoveSpeed;
            //    camera->pos -= offset;
            //}
            break;

        }

        if (mouseButtonDown && xOffset != 0.f) {
            // transformation matrix(rotate)
            const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-camera->sensitivity * xOffset), camera->up);
            camera->forward = glm::normalize(glm::vec3(rot * glm::vec4(camera->forward, 0.f)));
        }

        if (mouseButtonDown && yOffset != 0.f) {
            // transformation matrix(rotate)
            const glm::vec3 right = -glm::normalize(glm::cross(camera->forward, camera->up));
            const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-camera->sensitivity * yOffset), right);
            camera->forward = glm::normalize(glm::vec3(rot * glm::vec4(camera->forward, 0.f)));
        }
        xOffset = 0.f;
        yOffset = 0.f;
    }

    void sCameraController::onEvent(Event::Event& e) {
        Event::Dispatcher dispatcher(e);

#ifdef PN_PLATFORM_WINDOWS

#ifdef _DEBUG
        // ===== DEBUG MODE: Check editor visibility =====
        auto editor = services->get<Editor::Editor>();
        bool editorIsVisible = editor && editor->isVisible();
#else
        // ===== RELEASE MODE: No editor exists, always allow controls =====
        bool editorIsVisible = false;
#endif

        // ===== CAMERA CONTROLS: Active in Release OR when Editor is hidden in Debug =====
        if (!editorIsVisible) {
            dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {

                if (e.getKeyCode() == PAIN_KEY_Q && LCTRL_KEYDOWN) {
                    //PN_CORE_INFO("Ctrl+Q pressed - Quitting application");
                    g_shouldQuitApplication = true; // If you have this global flag
                    return true; // Event handled
                }

                switch (e.getKeyCode()) {
                case PAIN_KEY_W:
                    W_KEYDOWN = true;
                    break;
                case PAIN_KEY_A:
                    A_KEYDOWN = true;
                    break;
                case PAIN_KEY_S:
                    S_KEYDOWN = true;
                    break;
                case PAIN_KEY_D:
                    D_KEYDOWN = true;
                    break;
                case PAIN_KEY_E:
                    E_KEYDOWN = true;
                    break;
                case PAIN_KEY_Q:
                    Q_KEYDOWN = true;
                    break;
                case PAIN_KEY_ESCAPE:
                    ESC_KEYDOWN = true;
                    break;
                case PAIN_KEY_LEFT_SHIFT:
                    LSHIFT_KEYDOWN = true;
                    break;
                case PAIN_KEY_LEFT_CONTROL:
                    LCTRL_KEYDOWN = true;
                    break;
                default:
                    break;
                }
                return false;
                });

            dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {
                switch (e.getKeyCode()) {
                case PAIN_KEY_W:
                    W_KEYDOWN = false;
                    break;
                case PAIN_KEY_A:
                    A_KEYDOWN = false;
                    break;
                case PAIN_KEY_S:
                    S_KEYDOWN = false;
                    break;
                case PAIN_KEY_D:
                    D_KEYDOWN = false;
                    break;
                case PAIN_KEY_E:
                    E_KEYDOWN = false;
                    break;
                case PAIN_KEY_Q:
                    Q_KEYDOWN = false;
                    break;
                case PAIN_KEY_LEFT_SHIFT:
                    LSHIFT_KEYDOWN = false;
                    break;
                case PAIN_KEY_LEFT_CONTROL:
                    LCTRL_KEYDOWN = false;
                    break;
                case PAIN_KEY_ESCAPE:
                    ESC_KEYDOWN = false;
                    break;
                default:
                    break;
                }
                return false;
                });

            // ===== MOUSE CONTROL FOR CAMERA =====
            dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
                // Right mouse button (button code 1)
                if (e.getBtnCode() == 1) {
                    mouseButtonDown = true;
                }
                return false;
                });

            dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {
                if (e.getBtnCode() == 1) {
                    mouseButtonDown = false;
                }
                return false;
                });

            dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
                if (mouseButtonDown) {
                    glm::vec2 currentPos = e.getWindowPos();

                    xOffset += (currentPos.x - m_mouseLastX);
                    yOffset += (m_mouseLastY - currentPos.y);

                    m_mouseLastX = currentPos.x;
                    m_mouseLastY = currentPos.y;
                }
                else {
                    // Update last position even when not dragging so the first drag is smooth
                    glm::vec2 currentPos = e.getWindowPos();
                    m_mouseLastX = currentPos.x;
                    m_mouseLastY = currentPos.y;
                }
                return false;
                });
        }

        // ===== BOTH MODES: Camera mode switching and audio mute =====
        dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {
            auto& gs = GraphicsSettings::get();
            auto window = services->get<Window::Window>();

            switch (e.getKeyCode()) {

            case PAIN_KEY_M:
            {
                m_isMuted = !m_isMuted;
                auto audio = services->get<Audio::Audio>();
                if (audio) {
                    audio->setMuteAll(m_isMuted);
                    if (m_isMuted) {
                        PN_CORE_INFO("Audio Muted");
                    }
                    else {
                        PN_CORE_INFO("Audio Unmuted");
                    }
                }
                break;
            }
            case PAIN_KEY_ESCAPE:
            {

                //auto window = services->get<Window::Window>();
                //if (window) {
                //    b_hide_mouse = !b_hide_mouse;
                //    window->setCursorMode(true);
                //    PN_CORE_INFO("Cursor Mode: {}", b_hide_mouse ? "Hidden" : "Visible");
                //}
                //break;
            }

            case PAIN_KEY_EQUAL: // '+' key in many layouts (with shift)
            case PAIN_KEY_KP_ADD: // Numpad +
                if (camera) {
                    camera->speed += 0.5f; // Increase speed by 0.5 (adjust as needed)
                    if (camera->speed > 20.f) camera->speed = 20.f; // Clamp max speed
                }
                PN_CORE_INFO("CAMERA SPEED +0.5f, SPEED: {}", camera->speed);
                break;

            case PAIN_KEY_MINUS: // '-' key
            case PAIN_KEY_KP_SUBTRACT: // Numpad -
                if (camera) {
                    camera->speed -= 0.5f; // Decrease speed by 0.5
                    if (camera->speed < 0.1f) camera->speed = 0.1f; // Clamp min speed
                }
                PN_CORE_INFO("CAMERA SPEED -0.5f, SPEED: {}", camera->speed);
                break;

            case PAIN_KEY_I:
                gs.interpolate_animation = !gs.interpolate_animation;
                PN_CORE_INFO("Toggled interpolate_animation: {}", gs.interpolate_animation);
                break;

            case PAIN_KEY_G:
                gs.draw_floor = !gs.draw_floor;
                PN_CORE_INFO("Toggled draw_floor: {}", gs.draw_floor);
                break;

            case PAIN_KEY_F3:
                gs.DEBUG_USE_DIFFUSE_MAP = !gs.DEBUG_USE_DIFFUSE_MAP;
                PN_CORE_INFO("Toggled DEBUG_USE_DIFFUSE_MAP: {}", gs.DEBUG_USE_DIFFUSE_MAP);
                break;
            case PAIN_KEY_F4:
                gs.DEBUG_USE_AO_MAP = !gs.DEBUG_USE_AO_MAP;
                PN_CORE_INFO("Toggled DEBUG_USE_AO_MAP: {}", gs.DEBUG_USE_AO_MAP);
                break;
            case PAIN_KEY_F5:
                gs.DEBUG_USE_NORMAL_MAP = !gs.DEBUG_USE_NORMAL_MAP;
                PN_CORE_INFO("Toggled DEBUG_USE_NORMAL_MAP: {}", gs.DEBUG_USE_NORMAL_MAP);
                break;
            case PAIN_KEY_F6:
                gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP = !gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP;
                PN_CORE_INFO("Toggled DEBUG_USE_ROUGHNESSMETALLIC_MAP: {}", gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP);
                break;
            case PAIN_KEY_F7:
                gs.DEBUG_USE_EMISSION_MAP = !gs.DEBUG_USE_EMISSION_MAP;
                PN_CORE_INFO("Toggled DEBUG_USE_EMISSION_MAP: {}", gs.DEBUG_USE_EMISSION_MAP);
                break;
            case PAIN_KEY_F8:
                gs.DEBUG_PBR_MAP_TYPE = (GraphicsSettings::DEBUG_PBR_MAP_TYPES)((gs.DEBUG_PBR_MAP_TYPE + 1) % GraphicsSettings::DEBUG_PBR_MAP_TYPES::NUM_PBR_MAP_TYPES);
                PN_CORE_INFO("Toggled DEBUG_PBR_MAP_TYPE: {}", (int)gs.DEBUG_PBR_MAP_TYPE);
                break;
            case PAIN_KEY_F9:
                gs.world_light = !gs.world_light;
                PN_CORE_INFO("Toggled world_light: {}", gs.world_light);
                break;
            case PAIN_KEY_F10:
                gs.ibl = !gs.ibl;
                PN_CORE_INFO("Toggled IBL: {}", gs.ibl);
                break;


            //case PAIN_KEY_G:
            //    gs.gamma_correction = !gs.gamma_correction;
            //    PN_CORE_INFO("Toggled gamma_correction: {}", gs.gamma_correction);
            //    break;

            //case PAIN_KEY_T:
            //    gs.tone_mapping_mode = (GraphicsSettings::TONE_MAPPING_TYPES)(((int)gs.tone_mapping_mode + 1) % (int)GraphicsSettings::TONE_MAPPING_TYPES::NUM_TONE_MAPPING_TYPES);
            //    PN_CORE_INFO("Tone mapping mode: {}", (int)gs.tone_mapping_mode);
            //    break;

            default:
                break;
            }
            return false;
            });

        dispatcher.Dispatch<Event::WindowFocused>([&](Event::WindowFocused& e) -> bool {
            PN_CORE_INFO(e.toString());
            return false;
            });

#else
        // ===== ANDROID: Touch events remain unchanged =====
        dispatcher.Dispatch<Event::SurfaceChanged>([&](Event::SurfaceChanged& e) -> bool {
            m_surfaceWidth = e.getWidth();
            m_surfaceHeight = e.getHeight();
            return false;
            });

        dispatcher.Dispatch<Event::TouchDown>([&](Event::TouchDown& e) -> bool {
            beginTouchControls(e.getPointerId(), e.getX(), e.getY());
            return false;
            });

        dispatcher.Dispatch<Event::TouchMove>([&](Event::TouchMove& e) -> bool {
            updateTouchControls(e.getPointerId(), e.getX(), e.getY());
            return false;
            });

        dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
            endTouchControls(e.getPointerId());
            return false;
            });

        dispatcher.Dispatch<Event::TouchCancel>([&](Event::TouchCancel& e) -> bool {
            endTouchControls(e.getPointerId());
            return false;
            });
#endif
    }

    glm::mat4 sCameraController::getViewMatrix() const
    {
        if (camera) {
            return camera->view();
        }
        return glm::mat4(1.0f);
    }
    glm::mat4 sCameraController::getProjectionMatrix() const
    {
        if (camera) {
            return camera->projection();
        }
        return glm::mat4(1.0f);
    }
}
