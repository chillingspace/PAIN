#include "pch.h"
#include "AnimationPanel.h"

#include "EntityPanel.h"
#include "ECS/Components/AllComponents.h"
#include "ECS/sMetaData.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            AnimationPanel::AnimationPanel() {
                name = "Animation";
                flags = ImGuiWindowFlags_None;
            }

            void AnimationPanel::onUpdate(PAIN::AppTiming timing) {

                auto ecs = services->get<ECS::Controller>();
                if (!ecs) {
                    ImGui::TextDisabled("ECS Controller unavailable");
                    return;
                }

                // If no entity is selected, show all animated entities
                if (m_SelectedEntity == entt::null) {
                    RenderAnimatedEntitiesList();
                    return;
                }

                // Check if selected entity is valid
                if (!ecs->checkEntity(m_SelectedEntity)) {
                    ImGui::TextDisabled("Invalid entity");
                    m_SelectedEntity = entt::null;
                    return;
                }

                auto modelRenderer = ecs->getEntityComponent<ModelRenderer>(m_SelectedEntity);

                if (!modelRenderer || !modelRenderer->get().cachedModelAsset) {
                    ImGui::TextDisabled("Selected entity has no model");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Show list of animated entities as fallback
                    RenderAnimatedEntitiesList();
                    return;
                }

                auto& mdl = modelRenderer->get();

                if (mdl.cachedModelAsset->animations.empty()) {
                    ImGui::TextDisabled("This model has no animations");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Show list of animated entities
                    RenderAnimatedEntitiesList();
                    return;
                }

                // ===== BACK BUTTON =====
                if (ImGui::Button("< Back to Animated Entities", ImVec2(-1, 0))) {
                    m_SelectedEntity = entt::null;
                    return;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Display entity info
                auto metadata = services->get<MetaData::Service>();
                if (metadata) {
                    std::string entityName = metadata->getEntityName(m_SelectedEntity);
                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Entity: %s", entityName.c_str());
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }

                RenderAnimationList();
                ImGui::Separator();
                RenderAnimationControls();
                ImGui::Separator();
                RenderTimelineEditor();
            }


            void AnimationPanel::RenderAnimatedEntitiesList() {
                auto ecs = services->get<ECS::Controller>();
                if (!ecs) return;

                ImGui::Text("Animated Entities in Scene");
                ImGui::Spacing();

                auto& registry = ecs->getRegistry();
                auto view = registry.view<ModelRenderer>();

                bool foundAnimated = false;

                if (ImGui::BeginChild("AnimatedEntitiesList", ImVec2(0, 200), true)) {
                    for (auto e : view) {
                        auto modelRenderer = ecs->getEntityComponent<ModelRenderer>(e);
                        if (!modelRenderer) continue;

                        auto& mdl = modelRenderer->get();

                        // Check if this model has animations
                        if (!mdl.cachedModelAsset || mdl.cachedModelAsset->animations.empty()) {
                            continue;
                        }

                        foundAnimated = true;

                        // Get entity name
                        std::string entityName = "Entity " + std::to_string(static_cast<uint32_t>(e));
                        auto metadata = services->get<MetaData::Service>();
                        if (metadata) {
                            entityName = metadata->getEntityName(e);
                        }

                        ImGui::PushID(static_cast<int>(e));

                        bool isSelected = (m_SelectedEntity == e);

                        // Display entity with animation info
                        std::string label = entityName + " (" +
                            std::to_string(mdl.cachedModelAsset->animations.size()) +
                            " animations)";

                        if (ImGui::Selectable(label.c_str(), isSelected)) {
                            SetSelectedEntity(e);
                        }

                        // Show current animation status
                        if (mdl.currentAnimationIndex >= 0 &&
                            mdl.currentAnimationIndex < static_cast<int>(mdl.cachedModelAsset->animations.size())) {

                            ImGui::SameLine();
                            if (mdl.isPlaying) {
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[Playing]");
                            }
                            else {
                                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "[Paused]");
                            }
                        }

                        ImGui::PopID();
                    }

                    if (!foundAnimated) {
                        ImGui::TextDisabled("No animated models in scene");
                        ImGui::Spacing();
                        ImGui::TextWrapped("Add a model with animations to the scene to use this panel.");
                    }
                }
                ImGui::EndChild();

                if (foundAnimated) {
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                        "Click on an entity to edit its animations");
                }
            }


            void AnimationPanel::RenderAnimationList() {
                auto ecs = services->get<ECS::Controller>();
                auto& mdl = ecs->getEntityComponent<ModelRenderer>(m_SelectedEntity)->get();

                ImGui::Text("Available Animations");
                ImGui::Spacing();

                if (ImGui::BeginChild("AnimationList", ImVec2(0, 150), true)) {
                    for (size_t i = 0; i < mdl.cachedModelAsset->animations.size(); ++i) {
                        const auto& anim = mdl.cachedModelAsset->animations[i];

                        ImGui::PushID(static_cast<int>(i));
                        bool isSelected = (mdl.currentAnimationIndex == static_cast<int>(i));

                        if (ImGui::Selectable(anim.name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                            mdl.currentAnimationIndex = static_cast<int>(i);

                            if (ImGui::IsMouseDoubleClicked(0)) {
                                mdl.PlayAnimation(static_cast<int>(i), mdl.loopAnimation, mdl.playbackSpeed);
                            }
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("Duration: %.2fs", anim.duration);
                            ImGui::Text("Double-click to play");
                            ImGui::EndTooltip();
                        }

                        ImGui::PopID();
                    }
                }
                ImGui::EndChild();
            }

            void AnimationPanel::RenderAnimationControls() {
                auto ecs = services->get<ECS::Controller>();
                auto& mdl = ecs->getEntityComponent<ModelRenderer>(m_SelectedEntity)->get();

                if (mdl.currentAnimationIndex < 0 ||
                    mdl.currentAnimationIndex >= static_cast<int>(mdl.cachedModelAsset->animations.size())) {
                    ImGui::TextDisabled("Select an animation to preview");
                    return;
                }

                const auto& currentAnim = mdl.cachedModelAsset->animations[mdl.currentAnimationIndex];

                ImGui::Text("Playback Controls");
                ImGui::Spacing();

                if (mdl.isPlaying) {
                    if (ImGui::Button("Pause", ImVec2(80, 0))) {
                        mdl.isPlaying = false;
                    }
                }
                else {
                    if (ImGui::Button("Play", ImVec2(80, 0))) {
                        mdl.PlayAnimation(mdl.currentAnimationIndex, mdl.loopAnimation, mdl.playbackSpeed);
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Stop", ImVec2(80, 0))) {
                    mdl.isPlaying = false;
                    mdl.animationTime = 0.0f;
                }

                ImGui::SameLine();
                if (ImGui::Button("Reset", ImVec2(80, 0))) {
                    mdl.animationTime = 0.0f;
                }

                ImGui::Spacing();
                ImGui::Checkbox("Loop Animation", &mdl.loopAnimation);

                ImGui::Text("Playback Speed");
                ImGui::SliderFloat("##Speed", &mdl.playbackSpeed, 0.1f, 5.0f, "%.2fx");

                ImGui::Spacing();
                ImGui::Text("Timeline Scrubber");
                float timeNormalized = (currentAnim.duration > 0.0f)
                    ? mdl.animationTime / currentAnim.duration
                    : 0.0f;

                if (ImGui::SliderFloat("##TimeScrubber", &timeNormalized, 0.0f, 1.0f, "%.3f")) {
                    mdl.animationTime = timeNormalized * currentAnim.duration;
                    mdl.isPlaying = false;
                }

                ImGui::Text("Time: %.2fs / %.2fs", mdl.animationTime, currentAnim.duration);
            }

            void AnimationPanel::RenderTimelineEditor() {
                auto ecs = services->get<ECS::Controller>();
                auto& mdl = ecs->getEntityComponent<ModelRenderer>(m_SelectedEntity)->get();

                if (mdl.currentAnimationIndex < 0 ||
                    mdl.currentAnimationIndex >= static_cast<int>(mdl.cachedModelAsset->animations.size())) {
                    return;
                }

                const auto& currentAnim = mdl.cachedModelAsset->animations[mdl.currentAnimationIndex];

                ImGui::Text("Timeline");
                ImGui::Spacing();

                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                ImVec2 canvas_size = ImGui::GetContentRegionAvail();
                canvas_size.y = 80.0f;

                if (canvas_size.x < 50.0f) return;

                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                draw_list->AddRectFilled(
                    canvas_pos,
                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                    IM_COL32(40, 40, 40, 255)
                );

                draw_list->AddRect(
                    canvas_pos,
                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                    IM_COL32(100, 100, 100, 255)
                );

                float timelineWidth = canvas_size.x;
                int numMarkers = std::max(5, static_cast<int>(currentAnim.duration * 2));

                for (int i = 0; i <= numMarkers; ++i) {
                    float t = (i / static_cast<float>(numMarkers)) * currentAnim.duration;
                    float x = canvas_pos.x + (t / currentAnim.duration) * timelineWidth;

                    float tickHeight = (i % 2 == 0) ? 15.0f : 10.0f;
                    draw_list->AddLine(
                        ImVec2(x, canvas_pos.y),
                        ImVec2(x, canvas_pos.y + tickHeight),
                        IM_COL32(150, 150, 150, 255),
                        1.0f
                    );

                    if (i % 2 == 0) {
                        char label[16];
                        snprintf(label, sizeof(label), "%.1fs", t);
                        draw_list->AddText(
                            ImVec2(x - 15, canvas_pos.y + 18),
                            IM_COL32(200, 200, 200, 255),
                            label
                        );
                    }
                }

                float currentX = canvas_pos.x + (mdl.animationTime / currentAnim.duration) * timelineWidth;
                draw_list->AddLine(
                    ImVec2(currentX, canvas_pos.y),
                    ImVec2(currentX, canvas_pos.y + canvas_size.y),
                    IM_COL32(255, 200, 0, 255),
                    2.5f
                );

                ImVec2 triangle[3] = {
                    ImVec2(currentX - 6, canvas_pos.y),
                    ImVec2(currentX + 6, canvas_pos.y),
                    ImVec2(currentX, canvas_pos.y + 10)
                };
                draw_list->AddTriangleFilled(triangle[0], triangle[1], triangle[2], IM_COL32(255, 200, 0, 255));

                ImGui::InvisibleButton("timeline", canvas_size);
                if (ImGui::IsItemClicked()) {
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    float clickX = mouse_pos.x - canvas_pos.x;
                    float normalizedTime = std::clamp(clickX / timelineWidth, 0.0f, 1.0f);
                    mdl.animationTime = normalizedTime * currentAnim.duration;
                    mdl.isPlaying = false;
                }
            }

            void AnimationPanel::SetSelectedEntity(entt::entity entity) {
                m_SelectedEntity = entity;
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
