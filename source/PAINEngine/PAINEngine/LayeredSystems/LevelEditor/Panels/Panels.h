#pragma once

#ifdef _DEBUG
#ifndef PANELS_HPP
#define PANELS_HPP

#include "../Command.h"

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

				//Actions manager
				std::shared_ptr<CommandManager> command_manager;

				//Popup queue
				std::vector<PopUp> popup_queue;

				//Draw popups
				void drawPopUps();

			protected:

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
				IPanel(std::shared_ptr<CommandManager> command_manager) : command_manager{ command_manager }{}

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