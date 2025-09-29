#include "pch.h"
#include "RendererLayer.h"
#include "./CoreSystems/Events/GLFW/KeyEvents.h"
#include "./CoreSystems/Events/GLFW/MouseEvents.h"
#include "./CoreSystems/Events/GLFW/WindowEvents.h"
#include "./CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "./Applications/Application.h"

#include "./CoreSystems/Renderer/Light.h"
#include "./CoreSystems/Renderer/Material.h"

//For imgui viewport
#include "./LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "./LayeredSystems/LevelEditor/Editor.h"

#ifndef PN_PLATFORM_ANDROID
#include <GLFW/glfw3.h>
#endif

namespace PAIN {

	void RendererLayer::onAttach() {

#ifdef PN_PLATFORM_ANDROID
		renderer = std::make_unique<AndroidRenderer>();
		if (renderer) {
			renderer->Init();
		}
#else
		w_renderer = std::make_unique<WindowsRenderer>();
		if (w_renderer) {
			w_renderer->Init();
		}

		// Create framebuffer for ImGui viewport
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// Create color texture
		glGenTextures(1, &fboTexture);
		glBindTexture(GL_TEXTURE_2D, fboTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbWidth, fbHeight, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, fboTexture, 0);

		// Create depth-stencil buffer
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
			GL_RENDERBUFFER, rbo);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			PN_CORE_ERROR("Framebuffer is not complete!");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif


	}

	void RendererLayer::onUpdate(AppTiming timing) {
		const float dt = timing.dt;

#ifdef PN_PLATFORM_ANDROID
		if (renderer) {
			renderer->Render();
		}
#else
		if (w_renderer) {
			auto editor = services->get<Editor::Editor>();
			bool editor_visible = editor && editor->isVisible();

			if (editor_visible) {
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glViewport(0, 0, fbWidth, fbHeight);
			}
			else {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// Match viewport to window size
				auto window = services->get<Window::Window>();
				int winWidth, winHeight;
				glfwGetFramebufferSize((GLFWwindow*)window->getNativeWindow(), &winWidth, &winHeight);
				glViewport(0, 0, winWidth, winHeight);

			}

			// Clear + render
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			w_renderer->Render();

			glBindFramebuffer(GL_FRAMEBUFFER, 0); // reset
		}
		switch (move_mode) {
		case CAMERA:
			if (Camera::get().move_mode == Camera::MOVE_MODES::ORBIT_ORIGIN) {
				Camera::get().forward = -glm::normalize(Camera::get().pos); // always look at origin
			}

			if (W_KEYDOWN) {
				glm::vec3 offset = Camera::get().forward * Camera::get().speed;
				offset *= dt;
				Camera::get().pos += offset;
			}
			if (S_KEYDOWN) {
				glm::vec3 offset = Camera::get().forward * Camera::get().speed;
				offset *= dt;
				Camera::get().pos -= offset;
			}
			if (A_KEYDOWN) {
				glm::vec3 offset = glm::normalize(glm::cross(Camera::get().forward, Camera::get().up)) * Camera::get().speed;
				offset *= dt;
				Camera::get().pos -= offset;
			}
			if (D_KEYDOWN) {
				glm::vec3 offset = glm::normalize(glm::cross(Camera::get().forward, Camera::get().up)) * Camera::get().speed;
				offset *= dt;
				Camera::get().pos += offset;
			}
			if (SPACE_KEYDOWN) {
				glm::vec3 offset = Camera::get().up * Camera::get().speed;
				offset *= dt;
				Camera::get().pos += offset;
			}
			if (LCTRL_KEYDOWN) {
				glm::vec3 offset = Camera::get().up * Camera::get().speed;
				offset *= dt;
				Camera::get().pos -= offset;
			}
			break;
		case LIGHT:
			static constexpr float light_speed = 15.f;

			glm::vec3 forward, right, up;

			if (light.move_mode == Light::FREE) {
				forward = glm::vec3(0.f, 0.f, -1.f);
				right = glm::vec3(1.f, 0.f, 0.f);
				up = glm::vec3(0.f, 1.f, 0.f);
			}
			else if (light.move_mode == Light::ORBIT_ORIGIN) {
				forward = -glm::normalize(light.position);
				right = glm::normalize(glm::cross(forward, glm::vec3(0.f, 1.f, 0.f)));
				up = glm::normalize(glm::cross(right, forward));
			}

			if (W_KEYDOWN) {
				light.position += forward * light_speed * dt;
			}
			if (S_KEYDOWN) {
				light.position += -forward * light_speed * dt;
			}
			if (A_KEYDOWN) {
				light.position += -right * light_speed * dt;
			}
			if (D_KEYDOWN) {
				light.position += right * light_speed * dt;
			}
			if (SPACE_KEYDOWN) {
				light.position += up * light_speed * dt;
			}
			if (LCTRL_KEYDOWN) {
				light.position += -up * light_speed * dt;
			}
			break;
		}

		if (mouseButtonDown && xOffset != 0.f) {
			// transformation matrix(rotate)
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-Camera::get().sensitivity * xOffset), Camera::get().up);
			Camera::get().forward = glm::vec3(rot * glm::vec4(Camera::get().forward, 0.f));
		}
		if (mouseButtonDown && yOffset != 0.f) {
			// transformation matrix(rotate)
			const glm::vec3 right = -glm::normalize(glm::cross(Camera::get().forward, Camera::get().up));
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), glm::radians(-Camera::get().sensitivity * yOffset), right);
			Camera::get().forward = glm::vec3(rot * glm::vec4(Camera::get().forward, 0.f));
		}
		xOffset = 0.f;
		yOffset = 0.f;
#endif
	}
	void RendererLayer::onEvent(Event::Event& e) {
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
			//PN_CORE_INFO(e.toString());
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
				Camera::get().move_mode = static_cast<Camera::MOVE_MODES>((Camera::get().move_mode + 1) % Camera::MOVE_MODES::NUM_MOVE_MODES);
				light.move_mode = static_cast<Light::MOVE_MODES>((light.move_mode + 1) % Light::MOVE_MODES::NUM_MOVE_MODES);
				break;
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
#endif
	}
}