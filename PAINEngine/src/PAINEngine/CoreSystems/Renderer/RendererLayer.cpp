#include "RendererLayer.h"

#ifdef PN_PLATFORM_ANDROID
#include "Android/AndroidRenderer.h"
#else

#endif

namespace PAIN {
	PAIN::RendererLayer::~RendererLayer()
	{
	}
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
	void RendererLayer::onUpdate()
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