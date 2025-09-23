#include "pch.h"
#include "AudioPanel.h"
#include "PAINEngine/Applications/Application.h" 

#ifdef _DEBUG

#include "PAINEngine/Audio/AudioManager.h"


namespace PAIN {
	namespace Editor {
		namespace Panel {

			DebugAudioPanel::DebugAudioPanel(std::shared_ptr<CommandManager> command_manager)
				: IPanel(command_manager)
			{
				// Visible title for the window
				name  = "Debug & Audio";
				flags = ImGuiWindowFlags_None;
			}

			void DebugAudioPanel::nextWindowSettings() {
				// Default behavior (no fullscreen/docking hacks needed)
			}

			void DebugAudioPanel::onUpdate() {
				// --- ImGui Demo toggle + small perf HUD ---
				static bool show_demo = true;
				if (show_demo) ImGui::ShowDemoWindow(&show_demo);

				if (ImGui::Begin("PAIN Engine Debug")) {
					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
						1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
					ImGui::Checkbox("Show Demo Window", &show_demo);
				}
				ImGui::End();

				// --- Functional Audio Controls ---
				if (ImGui::Begin("Audio Controls")) {
					AudioManager& audio = PAIN::Application::Get().GetAudioManager();

					static char  soundPath[256] = "assets/audio/SFX/MovingSFX/Footstep_Metal_01.wav";
					static float volume         = 0.0f;  // dB
					static bool  loop           = false;
					static bool  is3D           = true;
					static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

					ImGui::InputText("Sound Path", soundPath, IM_ARRAYSIZE(soundPath));
					ImGui::SliderFloat("Volume (dB)", &volume, -80.0f, 10.0f, "%.2f");
					ImGui::Checkbox("Loop",  &loop);
					ImGui::Checkbox("3D",    &is3D);

					if (is3D) {
						ImGui::SliderFloat("X", &posX, -10.0f, 10.0f);
						ImGui::SliderFloat("Y", &posY, -10.0f, 10.0f);
						ImGui::SliderFloat("Z", &posZ, -10.0f, 10.0f);
					}

					if (ImGui::Button("Load Sound")) {
						audio.LoadSound(soundPath, is3D, loop);
					}
					ImGui::SameLine();
					if (ImGui::Button("Play Sound")) {
						audio.PlaySound(soundPath, { posX, posY, posZ }, volume);
					}

					ImGui::Separator();
					ImGui::Text("Playlist Controls");
					if (ImGui::Button("Play Random Footstep")) {
						audio.PlayRandomFromPlaylist("FootstepsGrass", { posX, posY, posZ }, volume);
					}

					ImGui::Separator();
					if (ImGui::Button("Stop All Sounds")) {
						audio.StopAllChannels();
					}
				}
				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
