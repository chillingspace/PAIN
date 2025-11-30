#pragma once

#ifdef _DEBUG
#ifndef TOOLS_PANELS_HPP
#define TOOLS_PANELS_HPP

#include "Panels.h"
#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Windows/Window.h"
#include "ECS/Controller.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {
			class EntityPanel;
			class ComponentsPanel;

			class Tools : public IPanel {
			private:

				//popup
				std::function<void(std::any const&)> createNewScenePopUp(std::string const& popup_id);
				std::function<void(std::any const&)> saveAsPopUp(std::string const& popup_id);
				std::function<void(std::any const&)> settingsPopUp(std::string const& popup_id);
				std::function<void(std::any const&)> unsavedChangesPopUp(std::string const& popup_id);
				std::function<void(std::any const&)> unsavedScenePopUp(std::string const& popup_id);

				std::function<void(std::any const&)> addComponentPopUp(std::string const& popup_id);

				std::weak_ptr<EntityPanel> entities_panel;
				std::weak_ptr<ComponentsPanel> components_panel;

			public:
				Tools();
				~Tools() override = default;

				void nextWindowSettings() override;


				void onAttach() override;

				void onUpdate(AppTiming timing) override;

				void onEvent(Event::Event& event) override;
			};
		}
	}
}

#endif
#endif