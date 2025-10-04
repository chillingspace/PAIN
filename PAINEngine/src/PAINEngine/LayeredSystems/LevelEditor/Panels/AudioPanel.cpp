#include "pch.h"
#include "AudioPanel.h"
#include "PAINEngine/Applications/Application.h" 

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

			void AudioPanel::onUpdate(AppTiming timing) {

				//if (ImGui::Begin("Audio Controls")) {
				//	AudioManager& audio = PAIN::Application::Get().GetAudioManager();

				//	static char  soundPath[256] = "assets/audio/SFX/MovingSFX/Footstep_Metal_01.wav";
				//	static float volume         = 0.0f;  
				//	static bool  loop           = false;
				//	static bool  is3D           = true;
				//	static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

				//	ImGui::InputText("Sound Path", soundPath, IM_ARRAYSIZE(soundPath));
				//	ImGui::SliderFloat("Volume (dB)", &volume, -80.0f, 10.0f, "%.2f");
				//	ImGui::Checkbox("Loop",  &loop);
				//	ImGui::Checkbox("3D",    &is3D);

				//	if (is3D) {
				//		ImGui::SliderFloat("X", &posX, -10.0f, 10.0f);
				//		ImGui::SliderFloat("Y", &posY, -10.0f, 10.0f);
				//		ImGui::SliderFloat("Z", &posZ, -10.0f, 10.0f);
				//	}

				//	if (ImGui::Button("Load Sound")) {
				//		audio.LoadSound(soundPath, is3D, loop);
				//	}
				//	ImGui::SameLine();
				//	if (ImGui::Button("Play Sound")) {
				//		audio.PlaySound(soundPath, { posX, posY, posZ }, volume);
				//	}

				//	ImGui::Separator();
				//	ImGui::Text("Playlist Controls");
				//	if (ImGui::Button("Play Random Footstep")) {
				//		audio.PlayRandomFromPlaylist("FootstepsGrass", { posX, posY, posZ }, volume);
				//	}

				//	ImGui::Separator();
				//	if (ImGui::Button("Stop All Sounds")) {
				//		audio.StopAllChannels();
				//	}
				//}
				//ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
