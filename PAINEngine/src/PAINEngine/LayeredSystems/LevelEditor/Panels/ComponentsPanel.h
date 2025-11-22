#pragma once
#ifdef _DEBUG
#ifndef PAIN_EDITOR_COMPONENTS_PANEL_HPP
#define PAIN_EDITOR_COMPONENTS_PANEL_HPP

#include "Panels.h"
#include "Utility/ECSUtility.h"

namespace PAIN {
	namespace Editor {

		namespace Panel {
			class EntityPanel;
			class ResourcePanel;

			class ComponentsPanel : public IPanel {
			public:
				ComponentsPanel();
				~ComponentsPanel() override = default;

				void nextWindowSettings() override;   // default

				void onAttach() override;
				void onUpdate(AppTiming timing) override;             // draw inside panel window

				static constexpr const char* getStaticName() { return "##ComponentsPanel"; }


				//Set comp removal string reference
				void setCompStringRef(std::string const& to_set);

				//Add component UI function (by type-derived key)
				template<typename T>
				void registerCompUIFunc(std::function<void(ComponentsPanel&, T&)> comp_func) {
					if (comps_ui.find(ECS::Utility::convertTypeString(typeid(T).name())) != comps_ui.end()) {
						throw std::runtime_error("Component UI function already registered");
					}

					comps_ui.emplace(ECS::Utility::convertTypeString(typeid(T).name()), [comp_func](ComponentsPanel& comp_panel, void* comp) { comp_func(comp_panel, *static_cast<T*>(comp)); });
				}

				// overload (by explicit ECS key)
				template<typename T>
				void registerCompUIFunc(const std::string& key,
					std::function<void(ComponentsPanel&, T&)> comp_func) {
					if (comps_ui.count(key)) {
						throw std::runtime_error("Component UI function already registered for key: " + key);
					}
					comps_ui.emplace(key, [comp_func](ComponentsPanel& comp_panel, void* comp) {
						comp_func(comp_panel, *static_cast<T*>(comp));
						});
				}

				void renderEntityComponents(entt::entity entity);

				//Expose services
				std::shared_ptr<Services> services;

			private:
				std::string comp_string_ref;

				//Component setting error message ( Usage: Editing error popup message )
				std::shared_ptr<std::string> error_msg;

				//Component setting success message ( Usage: Editing success popup message )
				std::shared_ptr<std::string> success_msg;

				std::function<void(std::any const&)> addComponentPopUp(std::string const& popup_id);

				std::function<void(std::any const&)> removeComponentPopUp(std::string const& popup_id);

				std::function<void(std::any const&)> addRigidBodyConfigPopUp(std::string const& popup_id);

				bool should_open_remove_popup = false;

				//Components to UI Function map
				std::unordered_map<std::string, std::function<void(ComponentsPanel&, void*)>> comps_ui;

				std::weak_ptr<EntityPanel> entities_panel;
				std::weak_ptr<ResourcePanel> resources_panel;

				// Transform tracking removed - now handled as static in .cpp
			};


		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
