#pragma once

#ifdef _DEBUG
#ifndef TOOLS_PANELS_HPP
#define TOOLS_PANELS_HPP

#include "pch.h"
#include "LayeredSystems/LevelEditor/Panels/Panels.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

			class Tools : public IPanel {
			private:
			public:
				Tools(std::shared_ptr<CommandManager> command_manager);

				void nextWindowSettings() override;

				void onUpdate() override;
			};
		}
	}
}

#endif
#endif