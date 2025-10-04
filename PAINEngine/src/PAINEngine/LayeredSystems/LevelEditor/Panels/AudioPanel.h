#pragma once

#ifdef _DEBUG
#ifndef DEBUG_AUDIO_PANEL_HPP
#define DEBUG_AUDIO_PANEL_HPP

#include "Panels.h"


namespace PAIN {
	namespace Editor {
		namespace Panel {

			class DebugAudioPanel : public IPanel {
			public:
				DebugAudioPanel();
				~DebugAudioPanel() override = default;

				void nextWindowSettings() override; 
				void onUpdate(AppTiming timing) override;
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
