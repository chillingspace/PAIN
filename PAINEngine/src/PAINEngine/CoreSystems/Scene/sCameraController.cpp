#include "sCameraController.h"


namespace PAIN {
	void sCameraController::onDetach() {}

	void sCameraController::onAttach()
	{
		//get scene
		m_Scene = services->get<Scene>();

	}

    void sCameraController::beginTouchLook(int pointerId, float x, float y) {

        float screen_center = m_surfaceWidth / 2.f;
		if (x >= screen_center){
            if (m_touchLooking) return;

            m_touchLooking = true;
            m_touchPointerId = pointerId;
            m_touchLastX = x;
            m_touchLastY = y;
            mouseButtonDown = true; // reuse existing yaw/pitch code path
        }
        if (x < screen_center){
            if (m_move.active) return;

            m_move.active = true;
            m_move.id = pointerId;
            m_move.start_x = x;
            m_move.start_y = y;
            m_move.last_x = x;
            m_move.last_y = y;
        }

    }

    void sCameraController::updateTouchLook(int pointerId, float x, float y) {
        if (m_touchLooking && pointerId == m_touchPointerId) {
            // Match desktop: yOffset = lastY - y so drag up looks up
            xOffset += (x - m_touchLastX);
            yOffset += (m_touchLastY - y);
            m_touchLastX = x;
            m_touchLastY = y;
        }
        if (m_move.active && pointerId == m_move.id){
            // Compute vector from stick center to current
            float dx = x - m_move.start_x;
            float dy = y - m_move.start_y;

            // Deadzone
            float len = sqrtf(dx*dx + dy*dy);
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
            // Easiest: directly move position here using dt and speed.
            // But since dt is only available in onUpdate, cache nx/ny in members or
            // move here and pass dt from your input path. Simpler: store nx/ny now:
            m_cachedMoveX = nx;
            m_cachedMoveY = ny;
        }

    }

    void sCameraController::endTouchLook(int pointerId) {
        if (m_touchLooking && pointerId == m_touchPointerId) {
            m_touchLooking = false;
            m_touchPointerId = -1;
            mouseButtonDown = false;
        }
        if (m_move.active && pointerId == m_move.id) {
            m_move = MoveStick{};
            m_cachedMoveX = 0.f;
            m_cachedMoveY = 0.f;
        }
    }
	void sCameraController::onUpdate(AppTiming timing)
	{
		const float dt = timing.dt;
		camera = m_Scene->GetActiveCamera();

		switch (move_mode) {
		case CAMERA:
			if (camera->move_mode == Camera::MOVE_MODES::ORBIT_ORIGIN) {
				// spherical
				float radius = glm::length(camera->pos);
				float theta = atan2(camera->pos.z, camera->pos.x);
				float phi = acos(camera->pos.y / radius);

				if (W_KEYDOWN) radius -= camera->speed * dt;
				if (S_KEYDOWN) radius += camera->speed * dt;
				if (A_KEYDOWN) theta += 1.5f * dt;
				if (D_KEYDOWN) theta -= 1.5f * dt;
				if (SPACE_KEYDOWN) phi -= 1.5f * dt;
				if (LCTRL_KEYDOWN) phi += 1.5f * dt;

				// clamp phi
				phi = glm::clamp(phi, 0.01f, glm::pi<float>() - 0.01f);

				// cartesian
				camera->pos.x = radius * sin(phi) * cos(theta);
				camera->pos.y = radius * cos(phi);
				camera->pos.z = radius * sin(phi) * sin(theta);

				// look at origin
				camera->forward = -glm::normalize(camera->pos);
			}

		case NUM_MOVE_MODES:

			static glm::mat4 mmtx = glm::scale(glm::mat4(1.f), glm::vec3(1, 0, 1));
			if (W_KEYDOWN) {
				glm::vec3 offset = glm::vec3(mmtx * glm::vec4(camera->forward, 1.f)) * camera->speed * dt;
				camera->pos += offset;
			}
			if (S_KEYDOWN) {
				glm::vec3 offset = glm::vec3(mmtx * glm::vec4(camera->forward, 1.f)) * camera->speed * dt;
				camera->pos -= offset;
			}
			if (A_KEYDOWN) {
				glm::vec3 offset = glm::normalize(glm::cross(camera->forward, camera->up)) * camera->speed * dt;
				camera->pos -= offset;
			}
			if (D_KEYDOWN) {
				glm::vec3 offset = glm::normalize(glm::cross(camera->forward, camera->up)) * camera->speed * dt;
				camera->pos += offset;
			}
			if (SPACE_KEYDOWN) {
				glm::vec3 offset = camera->up * camera->speed * dt;
				camera->pos += offset;
			}
			if (LCTRL_KEYDOWN) {
				glm::vec3 offset = camera->up * camera->speed * dt;
				camera->pos -= offset;
			}

			break;
		}

        if (m_move.active && camera) {
            // Project forward to XZ like you already do
            static glm::mat4 mmtx = glm::scale(glm::mat4(1.f), glm::vec3(1, 0, 1));
            glm::vec3 fwd   = glm::normalize(glm::vec3(mmtx * glm::vec4(camera->forward, 1.f)));
            glm::vec3 right = glm::normalize(glm::cross(camera->forward, camera->up));

            // Map: up on stick (negative ny) -> forward, right on stick (positive nx) -> strafe right
            float speed = camera->speed * m_moveScale;
            camera->pos += (-m_cachedMoveY) * fwd   * speed * dt;
            camera->pos += ( m_cachedMoveX) * right * speed * dt;
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
		dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {

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
			case PAIN_KEY_SPACE:
				SPACE_KEYDOWN = true;
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
			case PAIN_KEY_SPACE:
				SPACE_KEYDOWN = false;
				break;
			case PAIN_KEY_LEFT_CONTROL:
				LCTRL_KEYDOWN = false;
				break;
			default:
				break;
			}
			return false;
			});

		dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {

			switch (e.getKeyCode()) {
			case PAIN_KEY_TAB:
				move_mode = static_cast<MOVE_MODES>((move_mode + 1) % NUM_MOVE_MODES);
				break;
			case PAIN_KEY_O:
				camera->move_mode = static_cast<Camera::MOVE_MODES>((camera->move_mode + 1) % Camera::MOVE_MODES::NUM_MOVE_MODES);
				break;
			case PAIN_KEY_M:
			{
				m_isMuted = !m_isMuted; // Toggle the state
				auto audio = services->get<Audio::Audio>();
				if (audio) {
					audio->setMuteAll(m_isMuted); // Use the new mute function
					if (m_isMuted) {
						PN_CORE_INFO("Audio Muted");
					}
					else {
						PN_CORE_INFO("Audio Unmuted");
					}
				}
				break;
			}
			default:
				break;
			}
			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
			//PN_CORE_INFO(e.toString());

			if (e.getBtnCode() == PAIN_MOUSE_BUTTON_LEFT) {
				mouseButtonDown = true;
			}

			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {

			if (e.getBtnCode() == PAIN_MOUSE_BUTTON_LEFT) {
				mouseButtonDown = false;
			}

			return false;
			});

		dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
			static float lastX = 0.0f;
			static float lastY = 0.0f;

			xOffset = e.getWindowPos().x - lastX;
			yOffset = lastY - e.getWindowPos().y; // reversed since y-coordinates go from bottom to top


			lastX = e.getWindowPos().x;
			lastY = e.getWindowPos().y;

			return false;
			});

		dispatcher.Dispatch<Event::WindowFocused>([&](Event::WindowFocused& e) -> bool {
			PN_CORE_INFO(e.toString());
			return false;
			});
#else
        dispatcher.Dispatch<Event::TouchDown>([&](Event::TouchDown& e) -> bool {
            beginTouchLook(e.getPointerId(), e.getX(), e.getY());
            return true;
        });
        dispatcher.Dispatch<Event::TouchMove>([&](Event::TouchMove& e) -> bool {
            updateTouchLook(e.getPointerId(), e.getX(), e.getY());
            return true;
        });
        dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
            endTouchLook(e.getPointerId());
            return true;
        });
        dispatcher.Dispatch<Event::TouchCancel>([&](Event::TouchCancel& e) -> bool {
            endTouchLook(e.getPointerId());
            return true;
        });

		dispatcher.Dispatch<Event::SurfaceChanged>([&](Event::SurfaceChanged& e) -> bool {
            m_surfaceWidth = e.getWidth();
            m_surfaceHeight = e.getHeight();
			return true;
		});


#endif
	}

}
