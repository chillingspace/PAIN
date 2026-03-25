#include "Systems/Render/MinimapStyle.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

bool NearlyEqual(float lhs, float rhs, float epsilon = 0.0001f) {
    return std::fabs(lhs - rhs) <= epsilon;
}

bool ExpectTrue(bool condition, const char* testName, const char* message) {
    if (condition) {
        return true;
    }

    std::cerr << testName << " failed: " << message << '\n';
    return false;
}

bool TestCircleCoverageCounts() {
    using PAIN::Render::BuildMinimapCircleCoverage;

    const auto coverage = BuildMinimapCircleCoverage(glm::vec2(0.0f), 2.0f, 12);
    const bool fillOk = ExpectTrue(
        coverage.fillTriangles.size() == 36,
        "TestCircleCoverageCounts",
        "expected 12 triangles worth of fill vertices");
    const bool lineOk = ExpectTrue(
        coverage.outlineLines.size() == 24,
        "TestCircleCoverageCounts",
        "expected 12 line segments worth of outline vertices");
    return fillOk && lineOk;
}

bool TestDangerCoverageUsesCircleGeometry() {
    using PAIN::Render::BuildMinimapDangerCoverage;

    const auto coverage = BuildMinimapDangerCoverage(glm::vec2(0.0f), 4.0f, 10);
    const bool fillOk = ExpectTrue(
        coverage.fillTriangles.size() == 30,
        "TestDangerCoverageUsesCircleGeometry",
        "expected 10 triangles worth of danger fill vertices");
    const bool lineOk = ExpectTrue(
        coverage.outlineLines.size() == 20,
        "TestDangerCoverageUsesCircleGeometry",
        "expected 10 line segments worth of danger outline vertices");
    return fillOk && lineOk;
}

bool TestDangerStyleComesFromSingleHelper() {
    using PAIN::Render::BuildMinimapDangerStyle;

    const auto styleA = BuildMinimapDangerStyle(0.0f);
    const auto styleB = BuildMinimapDangerStyle(1.0f);

    const bool fillAlphaOk = ExpectTrue(
        styleA.fillColor.a > 0.05f && styleA.fillColor.a < 0.25f,
        "TestDangerStyleComesFromSingleHelper",
        "expected restrained danger fill alpha");
    const bool edgeAlphaOk = ExpectTrue(
        styleA.edgeColor.a > styleA.fillColor.a,
        "TestDangerStyleComesFromSingleHelper",
        "expected danger edge alpha to exceed fill alpha");
    const bool pulseOk = ExpectTrue(
        !NearlyEqual(styleA.edgeColor.a, styleB.edgeColor.a, 0.0001f),
        "TestDangerStyleComesFromSingleHelper",
        "expected helper-driven pulse to change edge alpha over time");
    return fillAlphaOk && edgeAlphaOk && pulseOk;
}

bool TestWallStyleProvidesFallbackOutline() {
    using PAIN::Render::BuildMinimapWallStyle;

    const auto style = BuildMinimapWallStyle(0.5f);
    const bool baseOk = ExpectTrue(
        style.fillColor.a > 0.5f,
        "TestWallStyleProvidesFallbackOutline",
        "expected solid structural wall fill");
    const bool outlineOk = ExpectTrue(
        style.fallbackOutlineColor.a > 0.0f,
        "TestWallStyleProvidesFallbackOutline",
        "expected fallback wall outline color");
    const bool patternOk = ExpectTrue(
        style.patternStrength > 0.0f && style.patternScale > 0.0f,
        "TestWallStyleProvidesFallbackOutline",
        "expected tactical wall pattern settings");
    return baseOk && outlineOk && patternOk;
}

bool TestLightConeTopDownRadiusMatchesLegacyShape() {
    using PAIN::Render::ComputeLightConeTopDownRadius;

    const float radius = ComputeLightConeTopDownRadius(2.0f, 9.0f, glm::radians(32.0f));
    const float expected = glm::max(
        0.25f,
        glm::min(std::tan(glm::radians(32.0f)) * 2.0f, std::sqrt(81.0f - 4.0f)));

    return ExpectTrue(
        NearlyEqual(radius, expected),
        "TestLightConeTopDownRadiusMatchesLegacyShape",
        "expected legacy light cone top-down radius");
}

}

int main() {
    const bool ok =
        TestCircleCoverageCounts() &&
        TestDangerCoverageUsesCircleGeometry() &&
        TestDangerStyleComesFromSingleHelper() &&
        TestWallStyleProvidesFallbackOutline() &&
        TestLightConeTopDownRadiusMatchesLegacyShape();

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
