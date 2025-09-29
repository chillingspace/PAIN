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

	#endif

	}
	void RendererLayer::onUpdate(AppTiming timing)
	{
	#ifdef PN_PLATFORM_ANDROID
			if (renderer) {
				renderer->Render();
			}
	#else

	#endif


	}
	void RendererLayer::onEvent(Event::Event& e)
	{
	}
}