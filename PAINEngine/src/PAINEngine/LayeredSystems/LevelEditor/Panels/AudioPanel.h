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
				DebugAudioPanel(std::shared_ptr<CommandManager> command_manager);

				void nextWindowSettings() override; // no-op (standard window)
				void onUpdate() override;
			};

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
