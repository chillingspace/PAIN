#include "RendererLayer.h"

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
	void RendererLayer::onUpdate()
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
	}
}