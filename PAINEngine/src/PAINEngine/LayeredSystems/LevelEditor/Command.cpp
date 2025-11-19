#include "pch.h"
#include "Command.h"

namespace PAIN {
	namespace Editor {

		void CommandManager::undo() {
			if (undo_stack.empty()) {
				PN_CORE_INFO("End Of Undo Stack.");
				return;
			}

			Action& action = undo_stack.top();

			if (!action.description.empty()) {
				PN_CORE_INFO("Undoing: {}", action.description);
			}

			// RAII guard to ensure flag is reset
			struct Guard {
				bool& flag;
				Guard(bool& f) : flag(f) { flag = true; }
				~Guard() { flag = false; }
			} guard(is_executing_undo_redo);

			action.undo_action();

			redo_stack.push(std::move(action));
			undo_stack.pop();

			// Flag is automatically reset when guard goes out of scope
		}

		void CommandManager::redo() {
			if (redo_stack.empty()) {
				PN_CORE_INFO("End Of Redo Stack.");
				return;
			}

			Action& action = redo_stack.top();

			if (!action.description.empty()) {
				PN_CORE_INFO("Redoing: {}", action.description);
			}

			// RAII guard to ensure flag is reset
			struct Guard {
				bool& flag;
				Guard(bool& f) : flag(f) { flag = true; }
				~Guard() { flag = false; }
			} guard(is_executing_undo_redo);

			action.do_action();

			undo_stack.push(std::move(action));
			redo_stack.pop();

			// Flag is automatically reset when guard goes out of scope
		}



		void CommandManager::executeAction(Action&& action) {
			// Log the action
			if (!action.description.empty()) {
				PN_CORE_INFO("Executing: {}", action.description);
			}

			// Execute action immediately
			action.do_action();

			// Push executed action onto the undo action stack
			undo_stack.push(std::move(action));

			// Limit stack size to prevent memory overflow
			if (undo_stack.size() > max_stack_size) {
				// Remove oldest action (bottom of stack)
				std::stack<Action> temp_stack;
				while (undo_stack.size() > 1) {
					temp_stack.push(std::move(undo_stack.top()));
					undo_stack.pop();
				}
				undo_stack.pop(); // Remove the oldest

				while (!temp_stack.empty()) {
					undo_stack.push(std::move(temp_stack.top()));
					temp_stack.pop();
				}
			}

			// Clear redo stack (reset upon new action)
			while (!redo_stack.empty()) {
				redo_stack.pop();
			}
		}

		void CommandManager::clearStacks() {
			PN_CORE_INFO("Clearing command history ({} undo, {} redo)",
				undo_stack.size(), redo_stack.size());

			// Clear redo stack
			while (!redo_stack.empty()) {
				redo_stack.pop();
			}

			// Clear undo stack
			while (!undo_stack.empty()) {
				undo_stack.pop();
			}
		}
	}
}
