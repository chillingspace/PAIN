#ifdef _DEBUG
#include "pch.h"
#include "Command.h"

namespace PAIN {
	namespace Editor {

		void CommandManager::undo() {
			if (undo_stack.empty()) {
				PN_CORE_INFO("End Of Undo Stack.");
				return;
			}

			Action& action = undo_stack.back(); // back = top

			if (!action.description.empty()) {
				PN_CORE_INFO("Undoing: {}", action.description);
			}

			struct Guard {
				bool& flag;
				Guard(bool& f) : flag(f) { flag = true; }
				~Guard() { flag = false; }
			} guard(is_executing_undo_redo);

			action.undo_action();

#ifdef PN_PLATFORM_WINDOWS
			if (services) {
				if (auto ser = services->get<Serialization::Service>()) {
					ser->modifyScene();
				}
			}
#endif

			redo_stack.push_back(std::move(action)); // push to back = top
			undo_stack.pop_back();                   // pop from back = top
		}

		void CommandManager::redo() {
			if (redo_stack.empty()) {
				PN_CORE_INFO("End Of Redo Stack.");
				return;
			}

			Action& action = redo_stack.back(); // back = top

			if (!action.description.empty()) {
				PN_CORE_INFO("Redoing: {}", action.description);
			}

			struct Guard {
				bool& flag;
				Guard(bool& f) : flag(f) { flag = true; }
				~Guard() { flag = false; }
			} guard(is_executing_undo_redo);

			action.do_action();

#ifdef PN_PLATFORM_WINDOWS
			if (services) {
				if (auto ser = services->get<Serialization::Service>()) {
					ser->modifyScene();
				}
			}
#endif

			undo_stack.push_back(std::move(action)); // push to back = top
			redo_stack.pop_back();                   // pop from back = top
		}

		void CommandManager::executeAction(Action&& action) {
			if (!action.description.empty()) {
				PN_CORE_INFO("Executing: {}", action.description);
			}

			action.do_action();

#ifdef PN_PLATFORM_WINDOWS
			if (services) {
				if (auto ser = services->get<Serialization::Service>()) {
					ser->modifyScene();
				}
			}
#endif

			undo_stack.push_back(std::move(action)); // push to back = top

			// Enforce max stack size by dropping the oldest (front)
			if (undo_stack.size() > max_stack_size) {
				undo_stack.pop_front();
			}

			// Clear redo stack on new action
			redo_stack.clear();
		}

		void CommandManager::clearStacks() {
			PN_CORE_INFO("Clearing command history ({} undo, {} redo)",
				undo_stack.size(), redo_stack.size());
			undo_stack.clear();
			redo_stack.clear();
		}

	}
}
#endif