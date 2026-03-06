#pragma once

#include <functional>
#include <deque>
#include <string>
#include <memory>

namespace PAIN {
	namespace Editor {

		// Level editor actions
		struct Action {
			std::function<void()> do_action;
			std::function<void()> undo_action;
			std::string description;

			Action() = default;

			Action(std::function<void()> do_fn,
				std::function<void()> undo_fn,
				const std::string& desc = "")
				: do_action(std::move(do_fn))
				, undo_action(std::move(undo_fn))
				, description(desc) {
			}
		};

		// Undo & Redo Level Editor Manager
		class CommandManager {
		private:
			// Using deque so the debug panel can iterate the full history
			// Back = top (most recent), Front = oldest
			std::deque<Action> undo_stack;
			std::deque<Action> redo_stack;

			size_t max_stack_size = 100;
			bool is_executing_undo_redo = false;
			int modification_count = 0;

			std::shared_ptr<Services> services;

		public:
			CommandManager(std::shared_ptr<Services> s) : services(s) {}
			~CommandManager() = default;

			void undo();
			void redo();
			void executeAction(Action&& action);
			void clearStacks();

			// Stack state queries
			bool canUndo() const { return !undo_stack.empty(); }
			bool canRedo() const { return !redo_stack.empty(); }

			// Stack sizes
			size_t getUndoCount() const { return undo_stack.size(); }
			size_t getRedoCount() const { return redo_stack.size(); }

			// Next action descriptions (top = back of deque)
			std::string getNextUndoDescription() const {
				return undo_stack.empty() ? "" : undo_stack.back().description;
			}
			std::string getNextRedoDescription() const {
				return redo_stack.empty() ? "" : redo_stack.back().description;
			}

			// Full history access for debug panel (index 0 = oldest, back = most recent)
			const std::deque<Action>& getUndoHistory() const { return undo_stack; }
			const std::deque<Action>& getRedoHistory() const { return redo_stack; }

			void setMaxStackSize(size_t size) { max_stack_size = size; }
			size_t getMaxStackSize() const { return max_stack_size; }

			bool isExecutingUndoRedo() const { return is_executing_undo_redo; }
			int getModificationCount() const { return modification_count; }
		};

#define CREATE_ACTION(do_code, undo_code, desc) \
		PAIN::Editor::Action([=]() { do_code; }, [=]() { undo_code; }, desc)

	}
}