#pragma once

#ifdef _DEBUG
#ifndef PANELS_HPP
#define PANELS_HPP

#include "../Command.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "imnodes.h"

#include "Applications/AppSystem.h"

// Forward declare the real Editor in its namespace:
namespace PAIN { namespace Editor { class Editor; } }

namespace PAIN {
	namespace Editor {
		namespace Panel {

			//Different popup types
			enum class PopUpTypes { Regular, Modal };

			//Popup 
			struct PopUp {
				std::string id;
				PopUpTypes type;
				ImGuiWindowFlags flags = 0;
				std::function<void(bool&)> func;
			};

			//Interace panel
			class IPanel {
			private:

				//Friend class
				friend class PAIN::Editor::Editor;

				//Popup queue
				std::vector<PopUp> popup_queue;

				//Draw popups
				void drawPopUps();


				// ----------------------------
				// Internal PopUps
				// ----------------------------
				static bool b_popup_showing;

				struct InternalPopUp {
					std::any data = nullptr;
					bool b_is_open = false;
					std::function<void(std::any)> popUpFunction;
				};

				//Map of popups
				std::unordered_map<std::string, InternalPopUp> popups;

			protected:

				//Actions manager
				std::shared_ptr<CommandManager> command_manager;

				//Services
				std::shared_ptr<Services> services;

				//Panel dock ID
				unsigned int dock_id = 0;

				//Panel flags
				ImGuiWindowFlags flags = 0;

				//Panel name
				std::string name = CLASS_STR(IPanel);

				//Optional virtual panel window settings
				virtual void nextWindowSettings() {}
			public:

				//Constructor
				IPanel() = default;
				virtual ~IPanel() = default;

				//Get panel name
				std::string getPanelName() const { return name; }

				//Get panel dock id
				unsigned int getDockID() const { return dock_id; }

				//Virtual panel attach
				virtual void onAttach() {};

				//Virtual panel detach
				virtual void onDetach() {};

				//Virtual panel update
				virtual void onUpdate(AppTiming timing) = 0;

				//Virtual panel events
				virtual void onEvent(Event::Event& event) {}

				//Draw panel window
				void drawWindow(PAIN::AppTiming timing);

				// ----------------------------
				// Internal PopUps
				// ----------------------------
				//Register new popup
				void registerPopUp(std::string const& popup_id, std::function<void(std::any const&)> popup_func);

				//Edit registered popup
				void editPopUp(std::string const& popup_id, std::function<void(std::any const&)> popup_func);

				//Open popup
				void openPopUp(std::string const& popup_id, std::any&& data = nullptr);

				//Close popup
				void closePopUp(std::string const& popup_id);

				//Close all popups
				void closeAllPopUps();

				//Render popup
				void renderPopUps();

				//Check if popup is showing
				static bool checkPopUpShowing();

				//Default Popup
				std::function<void(std::any const&)> defPopUp(std::string const& id);
			};
		}
	}
}


#endif // IMGUI_LAYER_HPP
#endif // PDEBUG