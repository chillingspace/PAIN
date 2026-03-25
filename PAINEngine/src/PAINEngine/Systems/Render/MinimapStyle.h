#pragma once

#include <cstdint>
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
        glm::vec4 innerFillColor = glm::vec4(0.0f);
        glm::vec4 innerEdgeColor = glm::vec4(0.0f);
        glm::vec4 edgeColor = glm::vec4(0.0f);
        float innerRadiusScale = 0.72f;
    };

    struct MinimapGridVisualStyle {
        glm::vec4 minorLineColor = glm::vec4(0.0f);
        glm::vec4 majorLineColor = glm::vec4(0.0f);
        int divisions = 4;
    };

    struct MinimapCoverageGeometry {
        std::vector<glm::vec2> fillTriangles;
        std::vector<glm::vec2> outlineLines;
    };

    enum class MinimapColorFormat {
        RGBA8,
        RGBA16F,
    };

    struct MinimapBackgroundTargetSpec {
        MinimapColorFormat colorFormat = MinimapColorFormat::RGBA8;
        bool needsDepthStencil = false;
    };

    struct MinimapWallCacheFingerprintEntry {
        uint64_t entityKey = 0;
        uint64_t modelKey = 0;
        glm::mat4 worldMatrix = glm::mat4(1.0f);
    };

    MinimapCoverageGeometry BuildMinimapCircleCoverage(const glm::vec2& center,
                                                       float radius,
                                                       int segments);

    void AppendMinimapCircleCoverage(std::vector<glm::vec2>& fillTriangles,
                                     std::vector<glm::vec2>& outlineLines,
                                     const glm::vec2& center,
                                     float radius,
                                     int segments);

    MinimapCoverageGeometry BuildMinimapDangerCoverage(const glm::vec2& center,
                                                       float radius,
                                                       int segments);

    void AppendMinimapDangerCoverage(std::vector<glm::vec2>& fillTriangles,
                                     std::vector<glm::vec2>& outlineLines,
                                     const glm::vec2& center,
                                     float radius,
                                     int segments);

    void AppendMinimapTacticalGridLines(std::vector<glm::vec2>& minorLines,
                                        std::vector<glm::vec2>& majorLines,
                                        const glm::vec2& topLeft,
                                        const glm::vec2& bottomRight,
                                        int divisions);

    MinimapWallVisualStyle BuildMinimapWallStyle(float timeSeconds);

    MinimapDangerVisualStyle BuildMinimapDangerStyle(float timeSeconds);

    MinimapGridVisualStyle BuildMinimapGridStyle();

    float ComputeLightConeTopDownRadius(float planeHeightDelta,
                                        float maxDistance,
                                        float halfAngleRadians);

    float ComputeMinimapThreatPulse(float timeSeconds,
                                    float minValue = 0.7f,
                                    float maxValue = 1.0f,
                                    float frequencyHz = 0.8f);

    uint64_t BuildMinimapWallCacheSignature(
        const std::vector<MinimapWallCacheFingerprintEntry>& entries);

    MinimapBackgroundTargetSpec BuildMinimapBackgroundTargetSpec();

    int ComputeMinimapBorderLayerCount(float borderThickness);

}
