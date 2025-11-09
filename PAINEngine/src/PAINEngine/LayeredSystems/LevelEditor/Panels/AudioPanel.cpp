#include "pch.h"
#include "AudioPanel.h"
#include "PAINEngine/Applications/Application.h" 
#include "PAINEngine/CoreSystems/Audio/Audio.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {


			AudioPanel::AudioPanel() {

				
				name = "##AudioPanel";

				//Set panel flag
				flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
					ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
					ImGuiWindowFlags_MenuBar;
			}


			void AudioPanel::nextWindowSettings() {
				// Default behavior (no fullscreen/docking hacks needed)
			}

			void AudioPanel::onAttach()
			{
				sound_assets = services->get<Assets::Manager>()->getAllAssetDataOfType(Assets::Type::Audio);
				sound_paths_storage.clear();
				sound_paths.clear();
				for (auto& asset : sound_assets) {
					sound_paths_storage.push_back(asset->shipped_relative_path.string());
					sound_paths.push_back(sound_paths_storage.back().c_str());
				}
			}

			void AudioPanel::onUpdate(AppTiming timing) {

				//Increment timer
				auto_refresh_timer += timing.dt;

				if (ImGui::Begin("Audio Controls")) {
					auto asset_service = services->get<Assets::Manager>();
					auto audio = services->get<PAIN::Audio::Audio>();
                    if (!audio) {
                        ImGui::Text("Audio Service not available.");
                        ImGui::End();
                        return;
                    }

					//Auto update assets
					if (auto_refresh_timer > AUTO_REFRESH_INTERVAL) {
						sound_assets = services->get<Assets::Manager>()->getAllAssetDataOfType(Assets::Type::Audio);
						sound_paths_storage.clear();
						sound_paths.clear();
						for (auto& asset : sound_assets) {
							sound_paths_storage.push_back(asset->shipped_relative_path.string());
							sound_paths.push_back(sound_paths_storage.back().c_str());
						}
					}
				
					static int selectedSoundIdx = -1;
					if (!sound_paths.empty()) {
						// Combo box to select sound
						if (ImGui::Combo("Sound Asset", &selectedSoundIdx, sound_paths.data(), sound_paths.size())) {
							// When selection changes, update path text box
							if (selectedSoundIdx >= 0 && selectedSoundIdx < sound_paths.size()) {
								selected = asset_service->findGUID(sound_paths[selectedSoundIdx]);
							}
						}
					}

					static float volume         = 0.0f;  
					static bool  loop           = false;
					static bool  is3D           = true;
					static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

					ImGui::SliderFloat("Volume (dB)", &volume, -80.0f, 10.0f, "%.2f");
					ImGui::Checkbox("Loop",  &loop);
					ImGui::Checkbox("3D",    &is3D);

					if (is3D) {
						ImGui::SliderFloat("X", &posX, -10.0f, 10.0f);
						ImGui::SliderFloat("Y", &posY, -10.0f, 10.0f);
						ImGui::SliderFloat("Z", &posZ, -10.0f, 10.0f);
					}

					if (ImGui::Button("Play Sound")) {
						if(asset_service->checkAssetRegistered(selected)) audio->play(asset_service->getAsset<Audio::Sound>(selected), { posX, posY, posZ }, volume);
					}

					ImGui::Separator();

					if (ImGui::Button("Stop All Sounds")) {
						audio->stopAll();
					}
				}
				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif