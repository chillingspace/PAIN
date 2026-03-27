#include "CoreSystems/Scene/ScriptModelPreloadScanner.h"

#include <cstdlib>
#include <iostream>
#include <unordered_set>

namespace {

    bool ExpectEqual(const std::unordered_set<std::string>& actual,
                     const std::unordered_set<std::string>& expected,
                     const char* testName) {
        if (actual == expected) {
            return true;
        }

        std::cerr << testName << " failed\n";
        std::cerr << "Expected:\n";
        for (const auto& item : expected) {
            std::cerr << "  " << item << '\n';
        }
        std::cerr << "Actual:\n";
        for (const auto& item : actual) {
            std::cerr << "  " << item << '\n';
        }
        return false;
    }

}

int main() {
    using PAIN::Scene::ExtractSetModelAssetNames;

    const bool basicExtraction = ExpectEqual(
        ExtractSetModelAssetNames(R"(
            SetModel(player, "Frog_Anim.mesh")
            SetModel ( player , 'Frog_Anim_Gear.mesh' )
        )"),
        {"Frog_Anim.mesh", "Frog_Anim_Gear.mesh"},
        "basicExtraction");

    const bool ignoresComments = ExpectEqual(
        ExtractSetModelAssetNames(R"(
            -- SetModel(player, "Commented.mesh")
            --[[
                SetModel(player, "CommentedBlock.mesh")
            ]]
            SetModel(
                player,
                "Active.mesh"
            )
        )"),
        {"Active.mesh"},
        "ignoresComments");

    return (basicExtraction && ignoresComments) ? EXIT_SUCCESS : EXIT_FAILURE;
}
