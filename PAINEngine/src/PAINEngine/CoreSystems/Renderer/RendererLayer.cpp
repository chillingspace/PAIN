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

			return false;
			});

		dispatcher.Dispatch<Event::WindowFocused>([&](Event::WindowFocused& e) -> bool {
			PN_CORE_INFO(e.toString());
			return false;
			});
	}
}