#include "RendererLayer.h"
#include "./CoreSystems/Events/GLFW/KeyEvents.h"
#include "./CoreSystems/Events/GLFW/MouseEvents.h"
#include "./CoreSystems/Events/GLFW/WindowEvents.h"

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


	}
	void RendererLayer::onEvent(Event::Event& e)
	{
		Event::Dispatcher dispatcher(e);

		dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {
			//if (e.getKeyCode() == GLFW_KEY_W) {
			//	Camera::get().pos += Camera::get().target * Camera::get().speed;
			//}

			PN_CORE_INFO(e.toString());

			return false;
			});

		dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
			PN_CORE_INFO(e.toString());

			return false;
			});

		dispatcher.Dispatch<Event::WindowFocused>([&](Event::WindowFocused& e) -> bool {
			PN_CORE_INFO(e.toString());
			return false;
			});
	}
}