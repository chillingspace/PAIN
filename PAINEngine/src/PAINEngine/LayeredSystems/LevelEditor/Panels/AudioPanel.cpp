#include "pch.h"
#include "AudioPanel.h"
#include "PAINEngine/Applications/Application.h" 
#include "PAINEngine/CoreSystems/Audio/Audio.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"

#ifdef _DEBUG

#define TOSTRING(x) #x

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

			void AudioPanel::acceptPayload() {
				ImVec2 pos = ImGui::GetCursorPos();
				ImGui::Dummy(ImGui::GetContentRegionAvail());
				//Drop target
				if (ImGui::BeginDragDropTarget()) {
#ifdef PN_PLATFORM_WINDOWS
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(Assets::assetTypeToString(Assets::Type::Audio) + "_FILE").c_str())) {
						//Get asset ID
						File* file(static_cast<File*>(payload->Data));

						//Init audio
						AudioSource new_audio;
						new_audio.selected_audio = file->id;
						audio_comp = new_audio;

						//Point to audio comp
						audio_ptr = &audio_comp;

						//Update selected index
						updateSelectedIndex();
					}
#endif
					std::string cname = TOSTRING(AudioSource);
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(cname + "_COMP").c_str())) {
						if (payload->DataSize == sizeof(void*)) {
							void* comp_ptr = *reinterpret_cast<void* const*>(payload->Data);
							// Now cast with static_cast or dynamic_cast
							audio_ptr = static_cast<AudioSource*>(comp_ptr); // or dynamic_cast if polymorphic
							if (!audio_ptr) throw std::runtime_error("Invalid comp casted.");
						}

						//Update selected index
						updateSelectedIndex();
					}
					ImGui::EndDragDropTarget();
				}
				ImGui::SetCursorPos(pos);
			}

			void AudioPanel::updateSelectedIndex() {
				//Update selected index
				selected_sound_index = 0;
				for (auto const& sound : sound_assets) {
					if (sound->guid == audio_ptr->selected_audio) break;
					++selected_sound_index;
				}

				//update audio group selected index
				selected_audio_group_index = 0;
				for (auto const& group : group_paths_storage) {
					if (group == audio_ptr->group_name) break;
					++selected_audio_group_index;
				}
			}

			void AudioPanel::onAttach()
			{
				//Populate audio assets
				audio_ptr = &audio_comp;
				sound_assets = services->get<Assets::Manager>()->getAllAssetDataOfType(Assets::Type::Audio);
				audio_ptr->audio_paths_storage.clear();
				sound_paths.clear();
				for (auto& asset : sound_assets) {
					audio_ptr->audio_paths_storage.push_back(asset->shipped_relative_path.string());
					sound_paths.push_back(audio_ptr->audio_paths_storage.back().c_str());
				}

				//Populate audio groups
				group_paths_storage = services->get<PAIN::Audio::Audio>()->getAllGroups();
				group_paths.clear();
				for (auto const& group : group_paths_storage) {
					group_paths.push_back(group.c_str());
				}

				//Init selected group
				selected_group = group_paths[selected_group_index];
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

					//Accepting audio payloads
					acceptPayload();

					ImGui::SeparatorText("Audio");

					//Auto update assets
					if (auto_refresh_timer > AUTO_REFRESH_INTERVAL) {

						//Populate sound assets
						sound_assets = services->get<Assets::Manager>()->getAllAssetDataOfType(Assets::Type::Audio);
						audio_ptr->audio_paths_storage.clear();
						sound_paths.clear();
						for (auto& asset : sound_assets) {
							audio_ptr->audio_paths_storage.push_back(asset->shipped_relative_path.string());
							sound_paths.push_back(audio_ptr->audio_paths_storage.back().c_str());
						}

						//Populate audio groups
						group_paths_storage = audio->getAllGroups();
						group_paths.clear();
						for (auto const& group : group_paths_storage) {
							group_paths.push_back(group.c_str());
						}
					}
				
					if (!sound_paths.empty()) {
						// Combo box to select sound
						if (ImGui::Combo("Sound Asset", &selected_sound_index, sound_paths.data(), sound_paths.size())) {
							// When selection changes, update path text box
							if (selected_sound_index >= 0 && selected_sound_index < sound_paths.size()) {
								audio_ptr->selected_audio = asset_service->findGUID(sound_paths[selected_sound_index]);
							}
						}
					}

					//Display groups
					if (!group_paths.empty()) {
						// Combo box to select sound
						if (ImGui::Combo("Audio Group", &selected_audio_group_index, group_paths.data(), group_paths.size())) {
							// When selection changes, update path text box
							if (selected_audio_group_index >= 0 && selected_audio_group_index < group_paths.size()) {
								audio_ptr->group_name = group_paths[selected_audio_group_index];
							}
						}
					}
					ImGui::SliderFloat("Volume (dB)##Audio", &audio_ptr->volumeDb, -20.0f, 20.0f, "%.2f");
					ImGui::SameLine(); if (ImGui::SmallButton("Reset##Vol")) audio_ptr->volumeDb = 0.0f;
					ImGui::SliderFloat("Pitch (dB)", &audio_ptr->pitchDb, -20.0f, 20.0f, "%.2f");
					ImGui::SameLine(); if (ImGui::SmallButton("Reset##Pitch")) audio_ptr->pitchDb = 0.0f;
					ImGui::Checkbox("Loop",  &audio_ptr->looping);
					ImGui::Checkbox("3D",    &audio_ptr->is3D);

					if (audio_ptr->is3D) {
						ImGui::SliderFloat("X", &audio_ptr->pos.x, -10.0f, 10.0f);
						ImGui::SameLine(); if (ImGui::SmallButton("Reset##Posx")) audio_ptr->pos.x = 0.0f;
						ImGui::SliderFloat("Y", &audio_ptr->pos.y, -10.0f, 10.0f);
						ImGui::SameLine(); if (ImGui::SmallButton("Reset##Posy")) audio_ptr->pos.y = 0.0f;
						ImGui::SliderFloat("Z", &audio_ptr->pos.z, -10.0f, 10.0f);
						ImGui::SameLine(); if (ImGui::SmallButton("Reset##Posz")) audio_ptr->pos.z = 0.0f;
					}

					if (ImGui::Button("Play Sound")) {
						if(asset_service->checkAssetRegistered(audio_ptr->selected_audio))
							audio->play(asset_service->getAsset<Sound>(audio_ptr->selected_audio), audio_ptr->group_name,
								audio_ptr->volumeDb, audio_ptr->pitchDb, audio_ptr->looping, audio_ptr->is3D,
								audio_ptr->pos, audio_ptr->minDistance, audio_ptr->maxDistance);
					}

					ImGui::SeparatorText("Group");

					//Display groups
					if (!group_paths.empty()) {
						// Combo box to select sound
						if (ImGui::Combo("Audio Group Manager", &selected_group_index, group_paths.data(), group_paths.size())) {
							// When selection changes, update path text box
							if (selected_group_index >= 0 && selected_group_index < group_paths.size()) {
								selected_group = group_paths[selected_group_index];
								group_vol = audio->getGroupVolumeDb(selected_group.c_str());
							}
						}
					}

					//Group options
					if (!selected_group.empty()) {
						if (ImGui::SliderFloat("Volume (dB)##Group", &group_vol, -20.0f, 20.0f, "%.2f")) {
							audio->setGroupVolumeDb(selected_group.c_str(), group_vol);
						}
						ImGui::SameLine(); if (ImGui::SmallButton("Reset##GroupVol")) {
							group_vol = 0.0f;
							audio->setGroupVolumeDb(selected_group.c_str(), group_vol);
						}

						if (audio->checkGroupIsPlaying(selected_group.c_str()) && !audio->checkGroupPaused(selected_group.c_str())) {
							//Pause group
							if (ImGui::Button("Pause Group##Group")) {
								audio->pauseGroup(selected_group.c_str());
							}
						}

						if (audio->checkGroupIsPlaying(selected_group.c_str()) && audio->checkGroupPaused(selected_group.c_str())) {
							//Resume group
							if (ImGui::Button("Resume Group##Group")) {
								audio->resumeGroup(selected_group.c_str());
							}
						}

						//Stop group
						if (audio->checkGroupIsPlaying(selected_group.c_str()) && ImGui::Button("Stop Group##Group")) {
							audio->stopGroup(selected_group.c_str());
						}
					}

					//Separator
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