#pragma once

#include <functional>
#include <stack>
#include <string>
#include <memory>

namespace PAIN {
	namespace Editor {

		// Level editor actions
		struct Action {
			std::function<void()> do_action;
			std::function<void()> undo_action;
			std::string description;  // For debugging and UI display

			// Default constructor (required for std::stack operations)
			Action() = default;

			// Parameterized constructor
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
			// Undo stack actions
			std::stack<Action> undo_stack;

			// Redo stack actions
			std::stack<Action> redo_stack;

			// Optional: limit stack size to prevent memory issues
			size_t max_stack_size = 100;

			// NEW: Flag to prevent recording during undo/redo
			bool is_executing_undo_redo = false;

			int modification_count = 0;

		public:
			CommandManager() = default;
			~CommandManager() = default;

			// Undo action
			void undo();

			// Redo action
			void redo();

			// Execute action
			void executeAction(Action&& action);

			// Clear Undo & Redo Stack
			void clearStacks();

			// Query stack states
			bool canUndo() const { return !undo_stack.empty(); }
			bool canRedo() const { return !redo_stack.empty(); }

			// Get stack sizes (useful for UI)
			size_t getUndoCount() const { return undo_stack.size(); }
			size_t getRedoCount() const { return redo_stack.size(); }

			// Set maximum stack size
			void setMaxStackSize(size_t size) { max_stack_size = size; }

			// Get maximum stack size
			size_t getMaxStackSize() const { return max_stack_size; }

			// Get description of next undo/redo action (for UI tooltips)
			std::string getNextUndoDescription() const {
				return undo_stack.empty() ? "" : undo_stack.top().description;
			}

			std::string getNextRedoDescription() const {
				return redo_stack.empty() ? "" : redo_stack.top().description;
			}

			// Check if currently executing undo/redo
			bool isExecutingUndoRedo() const { return is_executing_undo_redo; }

			// Get modification counter
			int getModificationCount() const { return modification_count; }
				 
			// Hooks
			std::function<void()> onModifySceneHook;
		};

		// Helper macro for creating actions with less boilerplate
#define CREATE_ACTION(do_code, undo_code, desc) \
			PAIN::Editor::Action([=]() { do_code; }, [=]() { undo_code; }, desc)

	}
}
