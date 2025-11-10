#pragma once

#ifdef _DEBUG
#ifndef DEBUG_AUDIO_PANEL_HPP
#define DEBUG_AUDIO_PANEL_HPP

#include "Panels.h"

#include "ResourcePanel.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {
			
			using namespace Audio;

			class AudioPanel : public IPanel {
			private:
				//Assets auto refresh timer
				float auto_refresh_timer = 0.0f;
				const float AUTO_REFRESH_INTERVAL = 2.0f;

				//Get all sounds
				std::vector <std::shared_ptr<Assets::IAsset>> sound_assets;
				std::vector<const char*> sound_paths;

				//Selected sound index
				int selected_sound_index = -1;

				//Default audio comp for testing
				AudioSource audio_comp;
				AudioSource* audio_ptr = nullptr;

				//Selected group index
				std::vector<std::string> group_paths_storage;
				int selected_group_index = 0;
				int selected_audio_group_index = -1;
				std::vector<const char*> group_paths;

				//Selected group
				std::string selected_group = "";
				float group_vol = 0.0f;

				//Internal accept payload
				void acceptPayload();

				//Update selected sound index
				void updateSelectedIndex();
			public:
				AudioPanel();
				~AudioPanel() override = default;

				void nextWindowSettings() override; 

				void onAttach() override;
				void onUpdate(AppTiming timing) override;
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
