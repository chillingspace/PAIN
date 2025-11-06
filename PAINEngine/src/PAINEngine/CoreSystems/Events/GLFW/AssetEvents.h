#pragma once

#ifdef PN_PLATFORM_WINDOWS
#ifndef ASSET_EVENTS_HPP
#define ASSET_EVENTS_HPP

#include "../Event.h"

namespace PAIN {
	namespace Event {

		class FileDropped : public Event {
		private:
			std::vector<std::string> paths;
		public:

			//Construct event
			FileDropped(int count, const char** raw_paths) {
				for (int i = 0; i < count; ++i)
					paths.emplace_back(raw_paths[i]);
			}

			//Get Files Count
			std::vector<std::string> getPaths() const { return paths; }

			//Register Event
			EVENT_CLASS_TYPE(FileDrop);
			EVENT_CLASS_CATEGORY(Category::Asset)
		};
	}
}

#endif
#endif

