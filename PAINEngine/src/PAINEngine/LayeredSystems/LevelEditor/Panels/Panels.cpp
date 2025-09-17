#include "pch.h"
#include "Panels.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {

			//Draw window of panel
			void IPanel::drawWindow() {

                //Next window settings ( Optional )
                nextWindowSettings();

                //Draw window
                bool b_check_active = true;
                if (ImGui::Begin(name.c_str(), &b_check_active, flags)) {

                    //Set window dock id
                    dock_id = ImGui::GetWindowDockID();

                    //Early exit if window is not active
                    if (!b_check_active) { ImGui::End(); return; }

                    //Update panel
                    onUpdate();

                    // Popups requested during OnGUI()/Update()
                    drawPopUps();
                }

                ImGui::End();
			}

            void IPanel::drawPopUps() {

                //Iterate through all popups and draw
                for (auto it = popup_queue.begin(); it != popup_queue.end();) {

                    //Get reference to popup
                    PopUp& p = *it;

                    //Unique id for popup
                    ImGui::PushID(p.id.c_str());
                    ImGui::OpenPopup("##popup");

                    //Boolean to check if popup shld still be open
                    bool popup_open = true;

                    //Check popup type
                    switch (p.type) {
                    case PopUpTypes::Regular: {
                        if (ImGui::BeginPopupModal("##popup", nullptr, p.flags)) {

                            //Update popup function
                            if (p.func) p.func(popup_open);
                            if (!popup_open) ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }
                        break;
                    }
                    case PopUpTypes::Modal: {
                        if (ImGui::BeginPopup("##popup")) {

                            //Update popup function
                            if (p.func) p.func(popup_open);
                            if (!popup_open) ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }
                        break;
                    }
                    default:
                        break;
                    }

                    //Clear queue
                    ImGui::PopID();
                    if (!popup_open) it = popup_queue.erase(it);
                    else          ++it;
                }
            }
		}
	}
}

#endif