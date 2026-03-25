#include "ScriptModelPreloadScanner.h"

#include <regex>

namespace PAIN::Scene {
    namespace {

        std::string StripLuaComments(std::string_view source) {
            std::string stripped;
            stripped.reserve(source.size());

            for (size_t i = 0; i < source.size();) {
                const bool isLineComment =
                    i + 1 < source.size() &&
                    source[i] == '-' &&
                    source[i + 1] == '-';

                if (!isLineComment) {
                    stripped.push_back(source[i]);
                    ++i;
                    continue;
                }

                const bool isBlockComment =
                    i + 3 < source.size() &&
                    source[i + 2] == '[' &&
                    source[i + 3] == '[';

                if (isBlockComment) {
                    i += 4;
                    while (i + 1 < source.size() &&
                           !(source[i] == ']' && source[i + 1] == ']')) {
                        ++i;
                    }
                    if (i + 1 < source.size()) {
                        i += 2;
                    }
                    continue;
                }

                i += 2;
                while (i < source.size() && source[i] != '\n') {
                    ++i;
                }
            }

            return stripped;
        }

    } // namespace

    std::unordered_set<std::string> ExtractSetModelAssetNames(std::string_view luaSource) {
        static const std::regex kSetModelPattern(
            R"(SetModel\s*\(\s*[^,]+,\s*(['"])([^'"]+)\1)",
            std::regex::ECMAScript);

        std::unordered_set<std::string> modelNames;
        const std::string stripped = StripLuaComments(luaSource);

        for (std::sregex_iterator it(stripped.begin(), stripped.end(), kSetModelPattern), end;
             it != end;
             ++it) {
            if ((*it).size() >= 3) {
                modelNames.insert((*it)[2].str());
            }
        }

        return modelNames;
    }

}
