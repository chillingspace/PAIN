#pragma once
#ifdef _DEBUG
#ifndef PAIN_EDITOR_SCENES_PANEL_HPP
#define PAIN_EDITOR_SCENES_PANEL_HPP

#include "Panels.h"


namespace PAIN {
    namespace Editor {
        namespace Panel {

            class ScenesPanel : public IPanel {
            public:
                ScenesPanel();
                ~ScenesPanel() override = default;

                void nextWindowSettings() override;   
                void onAttach() override;
                void onUpdate(AppTiming timing) override;

                static constexpr const char* getStaticName() { return "##ScenesPanel"; }

            private:

                //Selected GUID asset
                Assets::GUID selected;

                //Selected scn name
                std::string selected_scn_name;

                //Selected cam index
                int selected_cam_index;

                //Popups
                std::function<void(std::any const&)> createScenePopup(std::string const& popup_id);
                std::function<void(std::any const&)> saveSceneAsPopup(std::string const& popup_id);
                std::function<void(std::any const&)> deleteScenePopup(std::string const& popup_id);

                // UI 
                bool showCreate_ = false;
                bool showDelete_ = false;
                bool showSaveAs_ = false;
                bool showEditMask_ = false;
                std::string tmpNameBuf_;       

                // Hooks for future backend integration
                char nameBuf_[64] = "";  

            private:

                // modals
                void drawCreateModal();
                void drawDeleteModal();
                void drawSaveAsModal();
                void drawEditMaskModal();

                //Settings panel
                void drawLayerManagementPanel();

                // Layer management state
                void drawGraphicsSettingsPanel();

                // Cam Panel
                void drawActiveCamPanel();

                // Layer management state
                unsigned selectedLayerIdx_ = 0;

                //Camera ambient lighting
                glm::vec3 camera_ambient_color = glm::vec3(0);
                float camara_ambient_intensity = 0.5f;
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
