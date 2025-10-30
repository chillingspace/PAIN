#include "pch.h"
#include "AudioPanel.h"
#include "PAINEngine/Applications/Application.h" 
#include "PAINEngine/CoreSystems/Audio/Audio.h"
#include "CoreSystems/Path/Path.h"

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
			}

			void AudioPanel::onUpdate(AppTiming timing) {

				if (ImGui::Begin("Audio Controls")) {
					auto audio = services->get<PAIN::Audio::Audio>();
                    if (!audio) {
                        ImGui::Text("Audio Service not available.");
                        ImGui::End();
                        return;
                    }

					static char  soundPath[256] = "game_assets://Audio/SFX/MovingSFX/Footstep_Grass_01.wav";
					static float volume         = 0.0f;  
					static bool  loop           = false;
					static bool  is3D           = true;
					static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

					ImGui::InputText("Sound Path (Virtual)", soundPath, IM_ARRAYSIZE(soundPath));
					ImGui::SliderFloat("Volume (dB)", &volume, -80.0f, 10.0f, "%.2f");
					ImGui::Checkbox("Loop",  &loop);
					ImGui::Checkbox("3D",    &is3D);

					if (is3D) {
						ImGui::SliderFloat("X", &posX, -10.0f, 10.0f);
						ImGui::SliderFloat("Y", &posY, -10.0f, 10.0f);
						ImGui::SliderFloat("Z", &posZ, -10.0f, 10.0f);
					}

					if (ImGui::Button("Load Sound")) {
                        std::string path = services->get<Path::Path>()->resolvePath(soundPath);
						audio->loadSound(path, is3D, loop, false, 1.0f, 50.0f);
					}
					ImGui::SameLine();
					if (ImGui::Button("Play Sound")) {
                        std::string path = services->get<Path::Path>()->resolvePath(soundPath);
						audio->play(path, { posX, posY, posZ }, volume);
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