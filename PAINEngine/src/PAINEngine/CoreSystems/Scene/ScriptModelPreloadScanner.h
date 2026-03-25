#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace PAIN::Scene {

    // Extracts string-literal model names passed to Lua SetModel(...) calls.
    // Only literal string arguments are preloadable ahead of runtime.
    std::unordered_set<std::string> ExtractSetModelAssetNames(std::string_view luaSource);

}
