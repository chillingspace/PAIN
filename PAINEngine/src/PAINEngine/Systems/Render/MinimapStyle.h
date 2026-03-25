#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace PAIN::Render {

    struct MinimapWallVisualStyle {
        glm::vec4 fillColor = glm::vec4(0.0f);
        glm::vec4 accentColor = glm::vec4(0.0f);
        glm::vec4 fallbackOutlineColor = glm::vec4(0.0f);
        float patternStrength = 0.0f;
        float patternScale = 0.0f;
        float patternPhase = 0.0f;
    };

    struct MinimapDangerVisualStyle {
        glm::vec4 fillColor = glm::vec4(0.0f);
        glm::vec4 edgeColor = glm::vec4(0.0f);
    };

    struct MinimapCoverageGeometry {
        std::vector<glm::vec2> fillTriangles;
        std::vector<glm::vec2> outlineLines;
    };

    MinimapCoverageGeometry BuildMinimapCircleCoverage(const glm::vec2& center,
                                                       float radius,
                                                       int segments);

    MinimapCoverageGeometry BuildMinimapDangerCoverage(const glm::vec2& center,
                                                       float radius,
                                                       int segments);

    MinimapWallVisualStyle BuildMinimapWallStyle(float timeSeconds);

    MinimapDangerVisualStyle BuildMinimapDangerStyle(float timeSeconds);

    float ComputeLightConeTopDownRadius(float planeHeightDelta,
                                        float maxDistance,
                                        float halfAngleRadians);

    float ComputeMinimapThreatPulse(float timeSeconds,
                                    float minValue = 0.7f,
                                    float maxValue = 1.0f,
                                    float frequencyHz = 0.8f);

}
