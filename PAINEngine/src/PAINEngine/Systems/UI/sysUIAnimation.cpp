#include "pch.h"
#include "sysUIAnimation.h"
#include "ECS/Components/cMeshRenderer.h" 

namespace PAIN {

    namespace UI {

        AnimationSystem::AnimationSystem(std::shared_ptr<Services> svc) : ISystem(svc)
        {
        }

        AnimationSystem::~AnimationSystem()
        {
        }
        
        void AnimationSystem::onUpdate(AppTiming timing, entt::registry& registry) {
            // Group entities that have both UIAnimation and Texture2D
            auto view = registry.group<UIAnimation>(entt::get<Texture2D>);

            for (auto entity : view) {
                auto& ani_comp = view.get<UIAnimation>(entity);
                
                if (!ani_comp.b_playing) continue;

                // Update elapsed time
                ani_comp.elapsed += timing.dt;

                // Calculate total duration if not manually set (safety check)
                if (ani_comp.duration <= 0.0f) {
                    ani_comp.duration = ani_comp.total_frames * ani_comp.frame_duration;
                }

                // Calculate current frame
                // We use fmod to cycle through the duration if looping
                if (ani_comp.total_frames > 0 && ani_comp.frame_duration > 0) {
                    float mapping_time = ani_comp.elapsed;
                    
                    // Handle animation completion
                    if (mapping_time >= ani_comp.duration) {
                        if (ani_comp.b_loop) {
                            ani_comp.elapsed = 0.0f;
                            mapping_time = 0.0f;
                        } else {
                            ani_comp.b_playing = false;
                            mapping_time = ani_comp.duration; // Clamp to end
                            // Ensure we show the last frame
                            ani_comp.current_frame = ani_comp.total_frames - 1;
                        }
                    }

                     if (ani_comp.b_playing || ani_comp.b_loop) {
                        int frame = static_cast<int>(mapping_time / ani_comp.frame_duration);
                        ani_comp.current_frame = frame % ani_comp.total_frames;
                     }
                }

                // Calculate UV coordinates
                if (ani_comp.spritesheet_columns > 0 && ani_comp.spritesheet_rows > 0) {
                     // Calculate padding
                    float padding_u = 0.0f;
                    float padding_v = 0.0f;
                    auto svc = services.lock();
                    if (svc && registry.all_of<Texture2D>(entity)) {
                        auto& tex = view.get<Texture2D>(entity);
                        auto texture_opt = svc->get<Assets::Manager>()->getAsset<Assets::Texture>(tex.texture_guid);
                         if (texture_opt.has_value()) {
                             float tex_w = static_cast<float>(texture_opt.value().get()->width);
                             float tex_h = static_cast<float>(texture_opt.value().get()->height);
                             if(tex_w > 0) padding_u = 1.0f / tex_w;
                             if(tex_h > 0) padding_v = 1.0f / tex_h;
                         }
                    }

                    int col = ani_comp.current_frame % ani_comp.spritesheet_columns;
                    int row = ani_comp.current_frame / ani_comp.spritesheet_columns;

                    // Ensure row doesn't exceed total rows (safety)
                    row = row % ani_comp.spritesheet_rows;

                    float frame_width = 1.0f / ani_comp.spritesheet_columns;
                    float frame_height = 1.0f / ani_comp.spritesheet_rows;

                    glm::vec4 new_uvs{};
                    new_uvs.x = (col * frame_width) + padding_u;
                    new_uvs.y = (row * frame_height) + padding_v;
                    new_uvs.z = ((col + 1) * frame_width) - padding_u;
                    new_uvs.w = ((row + 1) * frame_height) - padding_v;

                    // store in UVCoordinates component
                    // Create if it doesn't exist
                    if (!registry.all_of<UVCoordinates>(entity)) {
                        registry.emplace<UVCoordinates>(entity);
                    }
                    
                    auto& uv_comp = registry.get<UVCoordinates>(entity);
                    uv_comp.uv = new_uvs;
                }
            }
        }

        // Removed easeInOut as it was only used for position interpolation
    }
}