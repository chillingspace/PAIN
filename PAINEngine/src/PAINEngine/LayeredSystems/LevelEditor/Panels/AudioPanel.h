#pragma once

#ifdef _DEBUG
#ifndef DEBUG_AUDIO_PANEL_HPP
#define DEBUG_AUDIO_PANEL_HPP

#include "Panels.h"


namespace PAIN {
	namespace Editor {
		namespace Panel {

			class AudioPanel : public IPanel {
			private:
				//Assets auto refresh timer
				float auto_refresh_timer = 0.0f;
				const float AUTO_REFRESH_INTERVAL = 2.0f;

				//Sound ptr
				std::shared_ptr<Audio::Sound> sound = nullptr;

				//Get all sounds
				std::vector <std::shared_ptr<Assets::IAsset>> sound_assets;
				std::vector<std::string> sound_paths_storage;
				std::vector<const char*> sound_paths;

				//Selected sound asset
				Assets::GUID selected;
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
