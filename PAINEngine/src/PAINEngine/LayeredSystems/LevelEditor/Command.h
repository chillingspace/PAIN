#pragma once

#include <functional>
#include <stack>

namespace PAIN {
	namespace Editor {

		//Level editor actions
		struct Action {
			std::function<void()> do_action;
			std::function<void()> undo_action;
		};

		//Undo & Redo Level Editor Manager
		class CommandManager {
		private:

			//Undo stack actions
			std::stack<Action> undo_stack;

			//Redo stack actions
			std::stack<Action> redo_stack;
		public:
			CommandManager() = default;
			~CommandManager() = default;

			//Undo action
			void undo();

			//Redo Action
			void redo();

			//Excute action
			void executeAction(Action&& action);

			//Clear Undo & Redo Stack
			void clearStacks();
		};
	}
}