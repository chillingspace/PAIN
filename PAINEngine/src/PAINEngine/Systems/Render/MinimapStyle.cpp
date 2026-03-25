#include "MinimapStyle.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include <glm/gtc/constants.hpp>

namespace PAIN::Render {

    namespace {

        constexpr uint64_t kFnvOffset = 1469598103934665603ull;
        constexpr uint64_t kFnvPrime = 1099511628211ull;

        void HashCombineU64(uint64_t& hash, uint64_t value) {
            hash ^= value;
            hash *= kFnvPrime;
        }

        void HashCombineFloat(uint64_t& hash, float value) {
            static_assert(sizeof(float) == sizeof(uint32_t));
            uint32_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            HashCombineU64(hash, static_cast<uint64_t>(bits));
        }

        void AppendLine(std::vector<glm::vec2>& lines,
                        const glm::vec2& a,
                        const glm::vec2& b) {
            lines.push_back(a);
            lines.push_back(b);
        }

    }

    MinimapCoverageGeometry BuildMinimapCircleCoverage(const glm::vec2& center,
                                                       float radius,
                                                       int segments) {
        MinimapCoverageGeometry geometry;
        AppendMinimapCircleCoverage(
            geometry.fillTriangles,
            geometry.outlineLines,
            center,
            radius,
            segments);
        return geometry;
    }

    void AppendMinimapCircleCoverage(std::vector<glm::vec2>& fillTriangles,
                                     std::vector<glm::vec2>& outlineLines,
                                     const glm::vec2& center,
                                     float radius,
                                     int segments) {
        if (radius <= 0.0f) {
            return;
        }

        const int safeSegments = std::max(3, segments);
        fillTriangles.reserve(fillTriangles.size() + static_cast<size_t>(safeSegments) * 3);
        outlineLines.reserve(outlineLines.size() + static_cast<size_t>(safeSegments) * 2);

        for (int i = 0; i < safeSegments; ++i) {
            const float a0 = (static_cast<float>(i) / static_cast<float>(safeSegments)) * glm::two_pi<float>();
            const float a1 = (static_cast<float>(i + 1) / static_cast<float>(safeSegments)) * glm::two_pi<float>();

            const glm::vec2 p0 = center + glm::vec2(std::cos(a0), std::sin(a0)) * radius;
            const glm::vec2 p1 = center + glm::vec2(std::cos(a1), std::sin(a1)) * radius;

            fillTriangles.push_back(center);
            fillTriangles.push_back(p0);
            fillTriangles.push_back(p1);

            AppendLine(outlineLines, p0, p1);
        }
    }

    MinimapCoverageGeometry BuildMinimapDangerCoverage(const glm::vec2& center,
                                                       float radius,
                                                       int segments) {
        return BuildMinimapCircleCoverage(center, radius, segments);
    }

    void AppendMinimapDangerCoverage(std::vector<glm::vec2>& fillTriangles,
                                     std::vector<glm::vec2>& outlineLines,
                                     const glm::vec2& center,
                                     float radius,
                                     int segments) {
        AppendMinimapCircleCoverage(fillTriangles, outlineLines, center, radius, segments);
    }

    void AppendMinimapTacticalGridLines(std::vector<glm::vec2>& minorLines,
                                        std::vector<glm::vec2>& majorLines,
                                        const glm::vec2& topLeft,
                                        const glm::vec2& bottomRight,
                                        int divisions) {
        const glm::vec2 size = bottomRight - topLeft;
        const int safeDivisions = std::max(2, divisions);
        if (size.x <= 0.0f || size.y <= 0.0f) {
            return;
        }

        minorLines.reserve(minorLines.size() + static_cast<size_t>(safeDivisions - 2) * 4);
        majorLines.reserve(majorLines.size() + 4);

        for (int i = 1; i < safeDivisions; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(safeDivisions);
            const bool isCenter = std::abs(t - 0.5f) <= 0.0001f;
            auto& target = isCenter ? majorLines : minorLines;

            const float x = glm::mix(topLeft.x, bottomRight.x, t);
            AppendLine(target, glm::vec2(x, topLeft.y), glm::vec2(x, bottomRight.y));

            const float y = glm::mix(topLeft.y, bottomRight.y, t);
            AppendLine(target, glm::vec2(topLeft.x, y), glm::vec2(bottomRight.x, y));
        }
    }

    MinimapWallVisualStyle BuildMinimapWallStyle(float timeSeconds) {
        MinimapWallVisualStyle style;
        style.fillColor = glm::vec4(0.18f, 0.26f, 0.31f, 0.78f);
        style.accentColor = glm::vec4(0.55f, 0.78f, 0.82f, 0.75f);
        style.fallbackOutlineColor = glm::vec4(0.64f, 0.82f, 0.86f, 0.5f);
        style.patternStrength = 0.28f;
        style.patternScale = 14.0f;
        style.patternPhase = timeSeconds * 0.045f;
        return style;
    }

    MinimapDangerVisualStyle BuildMinimapDangerStyle(float timeSeconds) {
        const float pulse = ComputeMinimapThreatPulse(timeSeconds, 0.75f, 1.0f, 0.65f);

        MinimapDangerVisualStyle style;
        style.fillColor = glm::vec4(0.95f, 0.34f, 0.10f, 0.10f + 0.08f * pulse);
        style.innerFillColor = glm::vec4(1.0f, 0.52f, 0.16f, 0.18f + 0.08f * pulse);
        style.innerEdgeColor = glm::vec4(1.0f, 0.74f, 0.30f, 0.38f + 0.14f * pulse);
        style.edgeColor = glm::vec4(1.0f, 0.62f, 0.22f, 0.62f + 0.18f * pulse);
        style.innerRadiusScale = 0.72f;
        return style;
    }

    MinimapGridVisualStyle BuildMinimapGridStyle() {
        MinimapGridVisualStyle style;
        style.minorLineColor = glm::vec4(0.42f, 0.62f, 0.66f, 0.10f);
        style.majorLineColor = glm::vec4(0.66f, 0.86f, 0.90f, 0.18f);
        style.divisions = 4;
        return style;
    }

    float ComputeLightConeTopDownRadius(float planeHeightDelta,
                                        float maxDistance,
                                        float halfAngleRadians) {
        const float safeHeight = std::abs(planeHeightDelta);
        const float safeDistance = std::max(0.0f, maxDistance);
        const float coneRadius = std::tan(std::max(0.0f, halfAngleRadians)) * safeHeight;
        const float sphereRadius = safeHeight >= safeDistance
            ? 0.0f
            : std::sqrt(std::max(0.0f, safeDistance * safeDistance - safeHeight * safeHeight));
        return glm::max(0.25f, glm::min(coneRadius, sphereRadius));
    }

    float ComputeMinimapThreatPulse(float timeSeconds,
                                    float minValue,
                                    float maxValue,
                                    float frequencyHz) {
        const float lo = std::min(minValue, maxValue);
        const float hi = std::max(minValue, maxValue);
        const float phase = 0.5f + 0.5f * std::sin(timeSeconds * glm::two_pi<float>() * std::max(0.0f, frequencyHz));
        return glm::mix(lo, hi, phase);
    }

    uint64_t BuildMinimapWallCacheSignature(
        const std::vector<MinimapWallCacheFingerprintEntry>& entries) {
        uint64_t hash = kFnvOffset;
        for (const MinimapWallCacheFingerprintEntry& entry : entries) {
            HashCombineU64(hash, entry.entityKey);
            HashCombineU64(hash, entry.modelKey);

            for (int column = 0; column < 4; ++column) {
                for (int row = 0; row < 4; ++row) {
                    HashCombineFloat(hash, entry.worldMatrix[column][row]);
                }
            }
        }
        return hash;
    }

    MinimapBackgroundTargetSpec BuildMinimapBackgroundTargetSpec() {
        return MinimapBackgroundTargetSpec{
            MinimapColorFormat::RGBA8,
            false,
        };
    }

    int ComputeMinimapBorderLayerCount(float borderThickness) {
        return static_cast<int>(glm::round(glm::max(0.0f, borderThickness)));
    }

}
