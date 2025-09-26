#pragma once

#ifdef _DEBUG
#ifndef PANELS_HPP
#define PANELS_HPP

#include "../Command.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

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

				//Get panel name
				std::string getPanelName() const { return name; }

				//Get panel dock id
				unsigned int getDockID() const { return dock_id; }

				//Virtual panel update
				virtual void onUpdate() = 0;

				//Draw panel window
				void drawWindow();
			};
		}
	}
}


#endif // IMGUI_LAYER_HPP
#endif // PDEBUG