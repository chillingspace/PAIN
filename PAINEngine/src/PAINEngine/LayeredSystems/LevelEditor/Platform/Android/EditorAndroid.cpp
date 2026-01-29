#include "pch.h"

#ifdef _DEBUG
#ifdef PN_PLATFORM_ANDROID

#include "EditorAndroid.h"
#include "CoreSystems/Events/Android/OtherEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"
#include <android/keycodes.h>
#include <android/input.h>

namespace PAIN {
	namespace Editor {

		EditorPlatform* EditorPlatform::createEditorPlatform(void* window) {
			return new EditorAndroid(static_cast<ANativeWindow*>(window));
		}

		void EditorAndroid::init() {
			// CRITICAL: Create ImGui context FIRST!
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			// Configure ImGui
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			// For Android, you might want to scale the UI
			io.FontGlobalScale = 2.0f; // Adjust based on your DPI

			//Set style
			ImGui::StyleColorsDark();

			//Initialize    platform and renderer backends
			ImGui_ImplAndroid_Init(a_window);
			ImGui_ImplOpenGL3_Init("#version 300 es");
		}

		void EditorAndroid::shutdown() {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplAndroid_Shutdown();
			ImGui::DestroyContext();
		}

		void EditorAndroid::beginFrame() {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplAndroid_NewFrame();
			ImGui::NewFrame();
		}

	// Helper function to map Android keycodes to ImGui keys
	static ImGuiKey mapAndroidKeyToImGuiKey(int32_t key_code) {
		switch (key_code) {
		case AKEYCODE_TAB: return ImGuiKey_Tab;
		case AKEYCODE_DPAD_LEFT: return ImGuiKey_LeftArrow;
		case AKEYCODE_DPAD_RIGHT: return ImGuiKey_RightArrow;
		case AKEYCODE_DPAD_UP: return ImGuiKey_UpArrow;
		case AKEYCODE_DPAD_DOWN: return ImGuiKey_DownArrow;
		case AKEYCODE_PAGE_UP: return ImGuiKey_PageUp;
		case AKEYCODE_PAGE_DOWN: return ImGuiKey_PageDown;
		case AKEYCODE_MOVE_HOME: return ImGuiKey_Home;
		case AKEYCODE_MOVE_END: return ImGuiKey_End;
		case AKEYCODE_INSERT: return ImGuiKey_Insert;
		case AKEYCODE_FORWARD_DEL: return ImGuiKey_Delete;
		case AKEYCODE_DEL: return ImGuiKey_Backspace;
		case AKEYCODE_SPACE: return ImGuiKey_Space;
		case AKEYCODE_ENTER: return ImGuiKey_Enter;
		case AKEYCODE_ESCAPE: return ImGuiKey_Escape;
		case AKEYCODE_APOSTROPHE: return ImGuiKey_Apostrophe;
		case AKEYCODE_COMMA: return ImGuiKey_Comma;
		case AKEYCODE_MINUS: return ImGuiKey_Minus;
		case AKEYCODE_PERIOD: return ImGuiKey_Period;
		case AKEYCODE_SLASH: return ImGuiKey_Slash;
		case AKEYCODE_SEMICOLON: return ImGuiKey_Semicolon;
		case AKEYCODE_EQUALS: return ImGuiKey_Equal;
		case AKEYCODE_LEFT_BRACKET: return ImGuiKey_LeftBracket;
		case AKEYCODE_BACKSLASH: return ImGuiKey_Backslash;
		case AKEYCODE_RIGHT_BRACKET: return ImGuiKey_RightBracket;
		case AKEYCODE_GRAVE: return ImGuiKey_GraveAccent;
		case AKEYCODE_CAPS_LOCK: return ImGuiKey_CapsLock;
		case AKEYCODE_SCROLL_LOCK: return ImGuiKey_ScrollLock;
		case AKEYCODE_NUM_LOCK: return ImGuiKey_NumLock;
		case AKEYCODE_SYSRQ: return ImGuiKey_PrintScreen;
		case AKEYCODE_BREAK: return ImGuiKey_Pause;
		case AKEYCODE_NUMPAD_0: return ImGuiKey_Keypad0;
		case AKEYCODE_NUMPAD_1: return ImGuiKey_Keypad1;
		case AKEYCODE_NUMPAD_2: return ImGuiKey_Keypad2;
		case AKEYCODE_NUMPAD_3: return ImGuiKey_Keypad3;
		case AKEYCODE_NUMPAD_4: return ImGuiKey_Keypad4;
		case AKEYCODE_NUMPAD_5: return ImGuiKey_Keypad5;
		case AKEYCODE_NUMPAD_6: return ImGuiKey_Keypad6;
		case AKEYCODE_NUMPAD_7: return ImGuiKey_Keypad7;
		case AKEYCODE_NUMPAD_8: return ImGuiKey_Keypad8;
		case AKEYCODE_NUMPAD_9: return ImGuiKey_Keypad9;
		case AKEYCODE_NUMPAD_DOT: return ImGuiKey_KeypadDecimal;
		case AKEYCODE_NUMPAD_DIVIDE: return ImGuiKey_KeypadDivide;
		case AKEYCODE_NUMPAD_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case AKEYCODE_NUMPAD_SUBTRACT: return ImGuiKey_KeypadSubtract;
		case AKEYCODE_NUMPAD_ADD: return ImGuiKey_KeypadAdd;
		case AKEYCODE_NUMPAD_ENTER: return ImGuiKey_KeypadEnter;
		case AKEYCODE_NUMPAD_EQUALS: return ImGuiKey_KeypadEqual;
		case AKEYCODE_CTRL_LEFT: return ImGuiKey_LeftCtrl;
		case AKEYCODE_SHIFT_LEFT: return ImGuiKey_LeftShift;
		case AKEYCODE_ALT_LEFT: return ImGuiKey_LeftAlt;
		case AKEYCODE_META_LEFT: return ImGuiKey_LeftSuper;
		case AKEYCODE_CTRL_RIGHT: return ImGuiKey_RightCtrl;
		case AKEYCODE_SHIFT_RIGHT: return ImGuiKey_RightShift;
		case AKEYCODE_ALT_RIGHT: return ImGuiKey_RightAlt;
		case AKEYCODE_META_RIGHT: return ImGuiKey_RightSuper;
		case AKEYCODE_MENU: return ImGuiKey_Menu;
		case AKEYCODE_0: return ImGuiKey_0;
		case AKEYCODE_1: return ImGuiKey_1;
		case AKEYCODE_2: return ImGuiKey_2;
		case AKEYCODE_3: return ImGuiKey_3;
		case AKEYCODE_4: return ImGuiKey_4;
		case AKEYCODE_5: return ImGuiKey_5;
		case AKEYCODE_6: return ImGuiKey_6;
		case AKEYCODE_7: return ImGuiKey_7;
		case AKEYCODE_8: return ImGuiKey_8;
		case AKEYCODE_9: return ImGuiKey_9;
		case AKEYCODE_A: return ImGuiKey_A;
		case AKEYCODE_B: return ImGuiKey_B;
		case AKEYCODE_C: return ImGuiKey_C;
		case AKEYCODE_D: return ImGuiKey_D;
		case AKEYCODE_E: return ImGuiKey_E;
		case AKEYCODE_F: return ImGuiKey_F;
		case AKEYCODE_G: return ImGuiKey_G;
		case AKEYCODE_H: return ImGuiKey_H;
		case AKEYCODE_I: return ImGuiKey_I;
		case AKEYCODE_J: return ImGuiKey_J;
		case AKEYCODE_K: return ImGuiKey_K;
		case AKEYCODE_L: return ImGuiKey_L;
		case AKEYCODE_M: return ImGuiKey_M;
		case AKEYCODE_N: return ImGuiKey_N;
		case AKEYCODE_O: return ImGuiKey_O;
		case AKEYCODE_P: return ImGuiKey_P;
		case AKEYCODE_Q: return ImGuiKey_Q;
		case AKEYCODE_R: return ImGuiKey_R;
		case AKEYCODE_S: return ImGuiKey_S;
		case AKEYCODE_T: return ImGuiKey_T;
		case AKEYCODE_U: return ImGuiKey_U;
		case AKEYCODE_V: return ImGuiKey_V;
		case AKEYCODE_W: return ImGuiKey_W;
		case AKEYCODE_X: return ImGuiKey_X;
		case AKEYCODE_Y: return ImGuiKey_Y;
		case AKEYCODE_Z: return ImGuiKey_Z;
		case AKEYCODE_F1: return ImGuiKey_F1;
		case AKEYCODE_F2: return ImGuiKey_F2;
		case AKEYCODE_F3: return ImGuiKey_F3;
		case AKEYCODE_F4: return ImGuiKey_F4;
		case AKEYCODE_F5: return ImGuiKey_F5;
		case AKEYCODE_F6: return ImGuiKey_F6;
		case AKEYCODE_F7: return ImGuiKey_F7;
		case AKEYCODE_F8: return ImGuiKey_F8;
		case AKEYCODE_F9: return ImGuiKey_F9;
		case AKEYCODE_F10: return ImGuiKey_F10;
		case AKEYCODE_F11: return ImGuiKey_F11;
		case AKEYCODE_F12: return ImGuiKey_F12;
		default: return ImGuiKey_None;
		}
	}

		void EditorAndroid::updateShortCuts(std::shared_ptr<CommandManager> command) {

		}

		void EditorAndroid::handleEvents(Event::Event& event) {
		ImGuiIO& io = ImGui::GetIO();

		// Create event dispatcher
		Event::Dispatcher dispatcher(event);

		// Handle touch events for ImGui
		dispatcher.Dispatch<Event::TouchDown>([&](Event::TouchDown& e) -> bool {
			io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
			io.AddMousePosEvent(e.getX(), e.getY());
			io.AddMouseButtonEvent(0, true); // Left button down
			return io.WantCaptureMouse; // Consumed if ImGui wants it
			});

		dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
			io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
			io.AddMousePosEvent(e.getX(), e.getY());
			io.AddMouseButtonEvent(0, false); // Left button up
			return io.WantCaptureMouse;
			});

		dispatcher.Dispatch<Event::TouchMove>([&](Event::TouchMove& e) -> bool {
			io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
			io.AddMousePosEvent(e.getX(), e.getY());
			return io.WantCaptureMouse;
			});

		dispatcher.Dispatch<Event::TouchCancel>([&](Event::TouchCancel& e) -> bool {
			io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
			io.AddMouseButtonEvent(0, false); // Release on cancel
			return io.WantCaptureMouse;
			});

		// Handle Android key events for ImGui
		dispatcher.Dispatch<Event::AndroidKeyDown>([&](Event::AndroidKeyDown& e) -> bool {
			ImGuiKey key = mapAndroidKeyToImGuiKey(e.getKeyCode());
			if (key != ImGuiKey_None) {
				int32_t meta_state = e.getMetaState();

				// Update modifier states
				io.AddKeyEvent(ImGuiMod_Ctrl, (meta_state & AMETA_CTRL_ON) != 0);
				io.AddKeyEvent(ImGuiMod_Shift, (meta_state & AMETA_SHIFT_ON) != 0);
				io.AddKeyEvent(ImGuiMod_Alt, (meta_state & AMETA_ALT_ON) != 0);
				io.AddKeyEvent(ImGuiMod_Super, (meta_state & AMETA_META_ON) != 0);

				// Send key down event
				io.AddKeyEvent(key, true);
				io.SetKeyEventNativeData(key, e.getKeyCode(), 0);
			}
			return io.WantTextInput;
			});

		dispatcher.Dispatch<Event::AndroidKeyUp>([&](Event::AndroidKeyUp& e) -> bool {
			ImGuiKey key = mapAndroidKeyToImGuiKey(e.getKeyCode());
			if (key != ImGuiKey_None) {
				int32_t meta_state = e.getMetaState();

				// Update modifier states
				io.AddKeyEvent(ImGuiMod_Ctrl, (meta_state & AMETA_CTRL_ON) != 0);
				io.AddKeyEvent(ImGuiMod_Shift, (meta_state & AMETA_SHIFT_ON) != 0);
				io.AddKeyEvent(ImGuiMod_Alt, (meta_state & AMETA_ALT_ON) != 0);
				io.AddKeyEvent(ImGuiMod_Super, (meta_state & AMETA_META_ON) != 0);

				// Send key up event
				io.AddKeyEvent(key, false);
				io.SetKeyEventNativeData(key, e.getKeyCode(), 0);
			}
			return io.WantCaptureKeyboard;
			});

		// Handle back button (can be used for ImGui navigation or custom handling)
		dispatcher.Dispatch<Event::BackButton>([&](Event::BackButton& e) -> bool {
			// ImGui doesn't have a specific back button, but we can treat it as Escape
			io.AddKeyEvent(ImGuiKey_Escape, true);
			io.AddKeyEvent(ImGuiKey_Escape, false);
			return io.WantCaptureKeyboard;
			});
	}
	}
}

#endif
#endif
