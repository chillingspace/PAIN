#include "sCameraController.h"


namespace PAIN {
	void sCameraController::onDetach() {}

	void sCameraController::onAttach()
	{
		//get scene
		m_Scene = services->get<Scene>();

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
#ifndef PN_PLATFORM_ANDROID
		Event::Dispatcher dispatcher(e);

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

#ifdef PN_PLATFORM_WINDOWS
			switch (e.getKeyCode()) {
			case PAIN_KEY_TAB:
				move_mode = static_cast<MOVE_MODES>((move_mode + 1) % NUM_MOVE_MODES);
				break;
			case PAIN_KEY_O:
				camera->move_mode = static_cast<Camera::MOVE_MODES>((camera->move_mode + 1) % Camera::MOVE_MODES::NUM_MOVE_MODES);
				break;
			default:
				break;
			}
#endif
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
#endif
	}

}
