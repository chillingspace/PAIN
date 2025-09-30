#include "RendererLayer.h"

namespace PAIN {

	void RendererLayer::onAttach()
	{
		
	#ifdef PN_PLATFORM_ANDROID
		renderer = std::make_unique<AndroidRenderer>();
	#else
		renderer = std::make_unique<WindowsRenderer>();
	#endif

		if (renderer) {
			renderer->Init();
		}

	}
	void RendererLayer::onUpdate(AppTiming timing)
	{
		if (renderer) {
			renderer->Render();
		}
	}
	void RendererLayer::onEvent(Event::Event& e)
	{
	}
}