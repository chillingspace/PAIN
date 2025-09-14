#include "pch.h"

#include "AppLayer.h"

namespace PAIN {

	void AppLayerStack::dispatchToLayers(Event::Event& e) {
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			if (e.checkHandled()) break;
		}
	}

	void AppLayerStack::dispatchToLayersReversed(Event::Event& e) {
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			if (e.checkHandled()) break;
		}
	}

	void AppLayerStack::addLayer(std::shared_ptr<AppLayer> layer) {
		layer_stack.push_back(layer);
	}

	void AppLayerStack::onUpdate() {
		//Iterate through all systems
		for (auto& layer : layer_stack) {
			layer->onUpdate();
		}
	}

	bool AppLayerStack::checkAppRunning() const {
		return b_app_running;
	}

	void AppLayerStack::terminateApp() {
		b_app_running = false;
	}
}
