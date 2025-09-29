#include "pch.h"
#include "RendererLayer.h"
#include "./CoreSystems/Events/GLFW/KeyEvents.h"
#include "./CoreSystems/Events/GLFW/MouseEvents.h"
#include "./CoreSystems/Events/GLFW/WindowEvents.h"
#include "./CoreSystems/Renderer/Windows/WindowsRenderer.h"

namespace PAIN {

	void RendererLayer::onAttach()
	{

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
#endif

	}
	void RendererLayer::onUpdate(float dt)
	{
#ifdef PN_PLATFORM_ANDROID
		if (renderer) {
			renderer->Render();
		}
#else
		if (w_renderer) {
			w_renderer->Render();
		}
#endif

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
	}
	void RendererLayer::onEvent(Event::Event& e)
	{
		Event::Dispatcher dispatcher(e);

		dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {

			switch (e.getKeyCode())
			{
			case GLFW_KEY_W:
				W_KEYDOWN = true;
				break;
			case GLFW_KEY_A:
				A_KEYDOWN = true;
				break;
			case GLFW_KEY_S:
				S_KEYDOWN = true;
				break;
			case GLFW_KEY_D:
				D_KEYDOWN = true;
				break;
			default:
				break;
			}


			return false;
			});

		dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {
			//PN_CORE_INFO(e.toString());
			switch (e.getKeyCode())
			{
			case GLFW_KEY_W:
				W_KEYDOWN = false;
				break;
			case GLFW_KEY_A:
				A_KEYDOWN = false;
				break;
			case GLFW_KEY_S:
				S_KEYDOWN = false;
				break;
			case GLFW_KEY_D:
				D_KEYDOWN = false;
				break;
			default:
				break;
			}
			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
			//PN_CORE_INFO(e.toString());

			if (e.getBtnCode() == GLFW_MOUSE_BUTTON_LEFT) {
				mouseButtonDown = true;
			}

			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {

			if (e.getBtnCode() == GLFW_MOUSE_BUTTON_LEFT) {
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
	}
}