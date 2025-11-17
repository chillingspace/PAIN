#pragma once
#ifdef _DEBUG
#ifndef PAIN_EDITOR_SCENES_PANEL_HPP
#define PAIN_EDITOR_SCENES_PANEL_HPP

#include "Panels.h"


namespace PAIN {
    namespace Editor {
        namespace Panel {

            // Optional wiring points for future integration (leave empty for now).
            struct ScenesHooks {
                // Called after creating a new scene (name without ".scn")
                std::function<void(const std::string& baseName)> onCreate;
                // Called on "Save Scene As" (new base name)
                std::function<bool(const std::string& baseName)> onSaveAs;
                // Called on "Save Curr Scene"
                std::function<bool(const std::string& currSceneId)> onSaveCurrent;
                // Called before deleting current scene (id with extension or your internal id)
                std::function<bool(const std::string& sceneId)> onDelete;
                // Called when user requests a scene change (id with extension)
                std::function<bool(const std::string& sceneId)> onChange;

                std::function<void(const std::string& sceneId)> onModifyScene; // for modified scene, but havent changed scene file
                std::function<void(unsigned i, unsigned j, bool v)> onMaskChanged;
                std::function<void(unsigned idx, bool visible)>     onLayerVisibleChanged;
                std::function<void()>                               onDirty;
            };

            class ScenesPanel : public IPanel {
            public:
                ScenesPanel(ScenesHooks hooks = {});
                ~ScenesPanel() override = default;

                void nextWindowSettings() override;   
                void setHooks(ScenesHooks h) { hooks_ = std::move(h); }


                void onAttach() override;
                void onUpdate(AppTiming timing) override;

                static constexpr const char* getStaticName() { return "##ScenesPanel"; }

            private:
                // Error message when loading scene fails
                bool showSceneLoadError_ = false;
                std::string loadSceneErrorMsg_;

                // Temporary in-panel “model” 
               int selected_scene_index = 0; 
               std::string currSceneId_;
                struct Layer {
                    unsigned id;
                    bool visible = true;
                };
                std::vector<Layer> layers_;                    
                std::vector<std::vector<bool>> mask_;         
                unsigned selectedLayerIdx_ = 0;

                Assets::GUID selected;

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
                ScenesHooks hooks_;
                char nameBuf_[64] = "";  

            private:
                // helpers
                void ensureAtLeastOneLayer();
                void rebuildMaskSize(std::size_t n);
                static std::string baseNameFromId(const std::string& sceneId);

                // modals
                void drawCreateModal();
                void drawDeleteModal();
                void drawSaveAsModal();
                void drawEditMaskModal();

                // ui 

                void drawSkyboxSettingsPanel();
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
