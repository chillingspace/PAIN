#include "pch.h"
#include "Panels.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {

			//Draw window of panel
			void IPanel::drawWindow(PAIN::AppTiming timing) {

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
                    onUpdate(timing);

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


			// ----------------------------
			// Internal PopUps
			// ----------------------------
			bool IPanel::b_popup_showing = false;

			void IPanel::registerPopUp(std::string const& popup_id, std::function<void(std::any const&)> popup_func) {
				//Check if popup has already been registered
				if (popups.find(popup_id) != popups.end()) {
					throw std::runtime_error("Popup already registered");
				}

				//Emplace popup
				popups.emplace(popup_id, InternalPopUp{ nullptr, false, std::move(popup_func) });
			}

			void IPanel::editPopUp(std::string const& popup_id, std::function<void(std::any const&)> popup_func) {
				//Check if popup has been registered
				if (popups.find(popup_id) == popups.end()) {
					throw std::runtime_error("Popup not yet registered");
				}

				popups.at(popup_id) = InternalPopUp{ nullptr, false, popup_func };
			}

			void IPanel::openPopUp(std::string const& popup_id, std::any&& data) {
				auto it = popups.find(popup_id);

				if (it == popups.end()) {
					throw std::runtime_error("Popup doest not exist");
				}

				//Set pop management variables
				b_popup_showing = true;
				popups.at(popup_id).b_is_open = true;
				popups.at(popup_id).data = std::move(data);
			}

			void IPanel::closePopUp(std::string const& popup_id) {
				auto it = popups.find(popup_id);

				if (it == popups.end()) {
					throw std::runtime_error("Popup doest not exist");
				}

				b_popup_showing = false;
				popups.at(popup_id).b_is_open = false;
				ImGui::CloseCurrentPopup();
			}

			void IPanel::closeAllPopUps() {
				b_popup_showing = false;
				for (auto& p : popups) {
					p.second.b_is_open = false;
				}
				ImGui::CloseCurrentPopup(); // This will close only the CURRENT popup context in ImGui this frame.
			}

			void IPanel::renderPopUps() {

				//Iterate through all popup and render
				for (auto& popup : popups) {
					if (popup.second.b_is_open){

						//Calculate the center of the viewport
						ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
						ImVec2 viewport_pos = ImGui::GetMainViewport()->Pos;
						ImVec2 popup_pos = ImVec2(viewport_pos.x + viewport_size.x * 0.5f, viewport_pos.y + viewport_size.y * 0.5f);

						//Center the popup
						ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
						ImGui::OpenPopup(popup.first.c_str());
						
						//Begin popup modal
						if (ImGui::BeginPopupModal(popup.first.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
							popup.second.popUpFunction(popup.second.data);
							ImGui::EndPopup();
						}
						else {
							PN_CORE_WARN("Failed to open popup.");
						}
					}
				}
			}

			bool IPanel::checkPopUpShowing() {
				return b_popup_showing;
			}

			std::function<void(std::any const&)> IPanel::defPopUp(std::string const& id) {
				return [this, id](std::any const& data) {

					//Cast into vector of strings
					if (data.has_value() && data.type() == typeid(std::shared_ptr<std::vector<std::string>>)) {
						auto vec = std::any_cast<std::shared_ptr<std::vector<std::string>>>(data);

						//Iterate through vec
						for (auto text : *vec) {
							//Show message
							ImGui::Text("%s", text.c_str());
						}
					}
					else {
						//Show message
						ImGui::Text("%s", "Hello World!");
					}
				

					//Add Spacing
					ImGui::Spacing();

					//OK button to close the popup
					if (ImGui::Button("OK")) {
						closePopUp(id);
					}
				};
			}
		}
	}
}

#endif
