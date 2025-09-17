#pragma once

#ifndef ASSET_EVENTS_HPP
#define ASSET_EVENTS_HPP

#include "Event.h"

namespace PAIN {
	namespace Event {

		class FileDropped : public Event {
		private:
			int count;
			const char** paths;
		public:

			//Construct event
			FileDropped(int count, const char** paths) : count{ count }, paths{ paths } {}

			//Get Files Count
			int getFilesCount() const { return count; }

			//Get Files Count
			const char** getPaths() const { return paths; }

			//Register Event
			EVENT_CLASS_TYPE(FileDrop);
			EVENT_CLASS_CATEGORY(Category::Asset)
		};
	}
}

#endif

