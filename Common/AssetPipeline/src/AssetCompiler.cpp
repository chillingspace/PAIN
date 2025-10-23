#include "AssetCompiler.h"

#include <cmath>
#include <chrono>
#include <limits>

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"

#include <sol/sol.hpp>

static void open_libs_from_settings(sol::state& lua, const nlohmann::json& import_settings) {
    std::vector<std::string> libs = { "base","math","string","table","utf8" };
    if (import_settings.contains("lua_libs") && import_settings["lua_libs"].is_array()) {
        libs.clear();
        for (auto& s : import_settings["lua_libs"]) libs.push_back(s.get<std::string>());
    }
    for (auto& name : libs) {
        if (name == "base")   lua.open_libraries(sol::lib::base);
        else if (name == "math")   lua.open_libraries(sol::lib::math);
        else if (name == "string") lua.open_libraries(sol::lib::string);
        else if (name == "table")  lua.open_libraries(sol::lib::table);
        else if (name == "utf8")   lua.open_libraries(sol::lib::utf8);
        else if (name == "os")     lua.open_libraries(sol::lib::os);
        else if (name == "io")     lua.open_libraries(sol::lib::io);
    }
}

// forward with policy flag
static nlohmann::json to_json_sol_obj(const sol::object& o, bool error_on_unsupported);

static nlohmann::json to_json_sol_table(const sol::table& t, bool error_on_unsupported) {
    const int n = static_cast<int>(t.size());

    // Treat as array ONLY if there is at least one element and all keys are 1..n
    bool array_like = (n > 0);
    for (int i = 1; array_like && i <= n; ++i) {
        if (!t.get<sol::object>(i).valid()) array_like = false;
    }

    // debug: which path was chosen
    // std::cerr << "[LuaBake] table size=" << n << " -> "
    //     << (array_like ? "ARRAY" : "OBJECT") << "\n";

    if (array_like) {
        nlohmann::json a = nlohmann::json::array();
        for (int i = 1; i <= n; ++i) {
            a.push_back(to_json_sol_obj(t.get<sol::object>(i), error_on_unsupported));
        }
        return a;
    }
    // object-like
    nlohmann::json j = nlohmann::json::object();
    for (auto& kv : t) {
        if (kv.first.get_type() != sol::type::string) {
            if (error_on_unsupported)
                throw std::runtime_error("[LuaBake] non-string key in object table");
            // permissive fallback
            std::string k = kv.first.as<std::string>();
            j[k] = to_json_sol_obj(kv.second, error_on_unsupported);
            continue;
        }
        std::string k = kv.first.as<std::string>();
        j[k] = to_json_sol_obj(kv.second, error_on_unsupported);
    }
    return j;
}

static nlohmann::json to_json_sol_obj(const sol::object& o, bool error_on_unsupported) {
    switch (o.get_type()) {
    case sol::type::nil:     return nullptr;
    case sol::type::boolean: return o.as<bool>();
    case sol::type::number: {
                                double d = o.as<double>();
                                if (std::isfinite(d) && std::floor(d) == d &&
                                    d >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
                                    d <= static_cast<double>(std::numeric_limits<int64_t>::max())) {
                                        return static_cast<int64_t>(d); // keep integers
                                    }
                                return d; 
                            }
    case sol::type::string:  return o.as<std::string>();
    case sol::type::table:   return to_json_sol_table(o.as<sol::table>(), error_on_unsupported);
    default:
        if (error_on_unsupported)
            throw std::runtime_error("[LuaBake] unsupported Lua type");
        return nullptr;
    }
}

void enforce_platform_rules(const nlohmann::json& baked,
    const nlohmann::json& settings,
    PAIN::Assets::Platform platform,
    std::string_view source)
{
    // bail if no overrides section
    if (!settings.contains("platform_overrides") || !settings["platform_overrides"].is_object())
        return;

    const auto& overrides = settings["platform_overrides"];
    const char* key = (platform == PAIN::Assets::Platform::Android) ? "Android" : "Windows";

    const nlohmann::json* po = nullptr;
    if (auto it = overrides.find(key); it != overrides.end())
        po = &it.value();

    if (!po) return;

    if (po->contains("max_waypoints")) {
        int mw = (*po)["max_waypoints"].get<int>();
        if (baked.contains("waypoints") && baked["waypoints"].is_array()
            && static_cast<int>(baked["waypoints"].size()) > mw) {
            throw PAIN::Assets::CompilationException(std::string(source) + ": too many waypoints for platform");
        }
    }

    if (po->contains("forbid_fields")) {
        for (const auto& f : (*po)["forbid_fields"]) {
            const std::string fld = f.get<std::string>();
            if (baked.contains(fld)) {
                throw PAIN::Assets::CompilationException(std::string(source) + ": field forbidden on platform: " + fld);
            }
        }
    }
}

namespace PAIN {
	namespace Assets {

         

        uint64_t Compiler::getCurrentTimeStamp() const {
            //Get current timestamp in milliseconds since epoch
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            return current_timestamp;
        }

        bool Compiler::verifyDirectory(std::filesystem::path const& dest) const {

            //Check if directory exists
            if (!std::filesystem::exists(dest)) {

                //Create directory if it doesnt exist
                std::filesystem::path parent_dir = dest.parent_path();
                if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
                    if (!std::filesystem::create_directories(parent_dir)) {
                        std::cout << "Failed to create parent directory: " << parent_dir << std::endl;
                        return false;
                    }

                    std::cout << "Created directory: " << parent_dir << std::endl;
                    return true;
                }
            }

            return true;
        }

		bool Compiler::copyFile(std::filesystem::path const& copy, std::filesystem::path const& dest) const {
            try {

                //Verify directory
                if (!verifyDirectory(dest)) {
                    std::cout << "Directory doesnt exist: " << dest << std::endl;
                    return false;
                }

                //Use std::filesystem::copy_file with update_existing option
                std::filesystem::copy_options options =
                    std::filesystem::copy_options::update_existing;

                if (std::filesystem::copy_file(copy, dest, options)) {
                    std::cout << "File Copied From: " << copy << " To: " << dest << std::endl;
                    return true;
                }
                else {
                    std::cout << copy << " Reposition Failed." << std::endl;
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cout << copy << " Reposition Failed." << e.what() << std::endl;
                return false;
            }
		}

        nlohmann::json Compiler::generateDefaultCompileSettings(Type const& type) const {
            nlohmann::json settings;

            switch (type) {
            case Type::Texture:
                settings["window_compression"] = "BC7";
                settings["android_compression"] = "ASTC_4x4";
                settings["generate_mipmaps"] = true;
                settings["max_size"] = 1024;
                settings["srgb"] = true;
                break;

            case Type::Audio:
                //settings["compression"] = "OGG";
                //settings["quality"] = 0.8;
                //settings["loop"] = false;
                break;

            case Type::Model:
                //settings["generate_lods"] = true;
                //settings["optimize_vertices"] = true;
                //settings["weld_threshold"] = 0.001;
                break;

            case Type::Script:
                settings["schema_version"] = 1;
                settings["output"] = "json"; 
                settings["error_on_unsupported"] = true; // fail build on functions/userdata
                settings["lua_libs"] = {"base","math","string","table","utf8"};
                break;
                
            default:
                break;
            }

            return settings;
        }

        bool Compiler::verifyCompileSettings(Type const& type, nlohmann::json const& settings) const {

            try {

                bool checker = true;

                switch (type) {
                case Type::Texture:
                    checker =   (settings.contains("window_compression") &&
                                settings.contains("android_compression") &&
                                settings.contains("generate_mipmaps") &&
                                settings.contains("max_size") &&
                                settings.contains("srgb"));
                    break;

                case Type::Audio:
                    //checker =   (settings.contains("compression") &&
                    //            settings.contains("quality") &&
                    //            settings.contains("loop"));
                    break;

                case Type::Model:
                    //checker =   (settings.contains("generate_lods") &&
                    //            settings.contains("optimize_vertices") &&
                    //            settings.contains("weld_threshold"));
                    break;
                
                case Type::Script:
                    checker = (settings.contains("schema_version") && settings.contains("output"));
                    break;

                default:
                    break;
                }

                if (!checker) return false;
                return true;
            }
            catch (const std::exception& e) {
                return false;
            }

        }

        Descriptor Compiler::createDefaultDesc(Info const& asset, std::filesystem::path const& path) const {

            //Extract asset name from path
            std::string asset_name = asset.raw_path.stem().string();

            // Create default descriptor
            Descriptor desc;

            // Identity
            desc.guid = GUID::Generate();
            desc.descriptor_version = 1;
            desc.created_timestamp = getCurrentTimeStamp();

            // Asset classification
            desc.type = asset.type;
            desc.name = asset_name;

            // Import/processing settings
            desc.import_settings = generateDefaultCompileSettings(desc.type);

            // Build data
            desc.raw_last_modified = asset.raw_last_modified;

            // Dependencies
            desc.dependencies.clear();

            // Metadata (start empty, expandable)
            desc.meta_data = nlohmann::json::object();

            // Add some basic metadata based on asset type
            desc.meta_data["source_file"] = asset.raw_path;

            //Save desc file
            saveDescFile(desc, path);

            return desc;
        }

        Descriptor Compiler::readDescFile(Info const& asset, std::filesystem::path const& path) const {
            try {
                std::ifstream file(path);
                nlohmann::json desc_json;
                file >> desc_json;

                Descriptor desc;
                desc.descriptor_version = desc_json.value("descriptor_version", 1);
                desc.guid = GUID(desc_json["guid"].get<std::string>());
                desc.created_timestamp = desc_json.value("created_timestamp", 0ULL);

                //Asset info
                auto asset_info = desc_json["asset_info"];
                desc.type = stringToAssetType(asset_info["type"].get<std::string>());
                desc.name = asset_info.value("name", "");

                //Settings and build data
                desc.import_settings = desc_json.value("import_settings", nlohmann::json{});
                auto build_data = desc_json["build_data"];
                desc.raw_last_modified = build_data.value("raw_last_modified", 0ULL);

                //Dependencies
                auto deps_array = desc_json.value("dependencies", nlohmann::json::array());
                for (const auto& dep_str : deps_array) {
                    desc.dependencies.push_back(GUID(dep_str.get<std::string>()));
                }

                desc.meta_data = desc_json.value("meta_data", nlohmann::json{});

                file.close();

                return desc;
            }
            catch (const std::exception& e) {
                std::cout << "Error encountered reading desc file, reverting to default." << std::endl;
                return createDefaultDesc(asset, path);
            }
        }

        bool Compiler::saveDescFile(Descriptor const& desc_file, std::filesystem::path const& path) const {
            //Save generated descriptor
            try {
                //Verify directory
                if (!verifyDirectory(path)) {
                    std::cout << "Directory doesnt exist: " << path << std::endl;
                    return false;
                }

                nlohmann::json desc_json;

                //Core identity
                desc_json["descriptor_version"] = desc_file.descriptor_version;
                desc_json["guid"] = desc_file.guid.ToString();
                desc_json["created_timestamp"] = desc_file.created_timestamp;

                // Asset info
                desc_json["asset_info"]["type"] = assetTypeToString(desc_file.type);
                desc_json["asset_info"]["name"] = desc_file.name;

                // Settings and build data
                desc_json["import_settings"] = desc_file.import_settings;
                desc_json["build_data"]["raw_last_modified"] = desc_file.raw_last_modified;

                // Dependencies and metadata
                nlohmann::json deps_array = nlohmann::json::array();
                for (const auto& dep : desc_file.dependencies) {
                    deps_array.push_back(dep.ToString());
                }
                desc_json["dependencies"] = deps_array;
                desc_json["meta_data"] = desc_file.meta_data;

                std::ofstream file(path, std::ios::out);
                if (file << desc_json.dump(2)) {
                    std::cout << "Descriptor file saved at: " << path << std::endl;
                    file.close();
                    return true;
                }
                std::cout << "Error saving default desc file to: " << path << std::endl;
                file.close();
                return false;
            }
            catch (const std::exception& e) {
                std::cout << "Error saving default desc file to: " << path << std::endl;
                return false;
            }
        }

        void Compiler::compileAndShip(Descriptor const& desc_file, Info& asset_info) const {

            //Find asset type and platform
            switch (desc_file.type) {
            case Type::Texture:
                compileTexture(desc_file, asset_info);
                break;

            case Type::Audio:
                compileAudio(desc_file, asset_info);
                break;

            case Type::Model:
                compileModel(desc_file, asset_info);
                break;

            case Type::Script:
                compileScript(desc_file, asset_info);
                break;
                
            default:
                break;
            }
        }

        void Compiler::compileTexture(Descriptor const& desc_file, Info& asset_info) const {

            //Determine output format and shipped path
            std::string output_extension;
            std::string compression_format;

            switch (platform) {
            case Platform::Windows:
                output_extension = ".dds";
                compression_format = desc_file.import_settings.value("compression", "BC7");
                asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + output_extension);
                break;
            case Platform::Android:
                output_extension = ".astc";
                compression_format = desc_file.import_settings.value("compression", "ASTC_4x4");
                asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + output_extension);
                break;
            default:
                std::cout << "ERROR: Unsupported platform for texture compilation" << std::endl;
                return;
            }

            //Load texture data using STB
            int width, height, channels;
            unsigned char* raw_pixels = stbi_load(asset_info.raw_path.string().c_str(),
                &width, &height, &channels, STBI_rgb_alpha);

            if (!raw_pixels) {
                std::cout << "ERROR: Failed to load texture: " << asset_info.raw_path
                    << " - " << stbi_failure_reason() << std::endl;
                return;
            }

            std::cout << "Loaded texture: " << width << "x" << height << " (" << channels << " channels)" << std::endl;

            //Apply import settings (resize if needed)
            int target_width = width;
            int target_height = height;
            int max_size = desc_file.import_settings.value("max_size", 2048);

            if (width > max_size || height > max_size) {
                float scale = static_cast<float>(max_size) / std::max(width, height);
                int target_width = static_cast<int>(width * scale);
                int target_height = static_cast<int>(height * scale);

                //Resize using STB
                unsigned char* resized_pixels = (unsigned char*)malloc(target_width * target_height * 4);

                // NEW API: stbir_resize (not stbir_resize_uint8)
                if (stbir_resize(raw_pixels, width, height, 0,           // input
                    resized_pixels, target_width, target_height, 0, // output  
                    STBIR_RGBA, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT)) { 

                    stbi_image_free(raw_pixels);
                    raw_pixels = resized_pixels;
                    width = target_width;
                    height = target_height;
                    std::cout << "Resized texture to: " << width << "x" << height << std::endl;
                }
                else {
                    std::cout << "ERROR: STB resize failed!" << std::endl;
                    free(resized_pixels);
                }
            }

            //Verify output directory
            if (!verifyDirectory(asset_info.shipped_path)) {
                std::cout << "ERROR: Failed to create output directory: " << asset_info.shipped_path.parent_path() << std::endl;
                stbi_image_free(raw_pixels);
                return;
            }

            //Compress assets
            bool compression_success = false;

            if (platform == Platform::Windows) {
                // Use Cuttlefish for DDS/BC7 compression
                compression_success = CompressTextureDDS(raw_pixels, width, height, 4,
                    asset_info.shipped_path.string(),
                    compression_format, desc_file.import_settings);
            }
            else if (platform == Platform::Android) {
                // Use Cuttlefish for ASTC compression
                compression_success = CompressTextureASTC(raw_pixels, width, height, 4,
                    asset_info.shipped_path.string(),
                    compression_format, desc_file.import_settings);
            }

            //Clean up
            stbi_image_free(raw_pixels);

            //Verify output
            if (compression_success && std::filesystem::exists(asset_info.shipped_path)) {
                std::cout << "Texture compiled successfully: " << asset_info.shipped_path.filename() << std::endl;
            }
            else {
                std::cout << "ERROR: Texture compilation failed for: " << asset_info.raw_path.filename() << std::endl;
            }
        }

        void Compiler::compileAudio(Descriptor const& desc_file, Info& asset_info) const {
            //To be implemented
        }

        void Compiler::compileModel(Descriptor const& desc_file, Info& asset_info) const {
            //To be implemented
        }

        void Compiler::compileScript(Descriptor const& desc_file, Info& asset_info) const {
            asset_info.shipped_path =
                output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + ".json");

            if (asset_info.raw_last_modified <= (getFileLastModified(asset_info.shipped_path) + TOLERANCE_MS)) {
                std::cout << "Script up to date: " << asset_info.raw_path.filename() << "\n";
                return;
            }

            if (!verifyDirectory(asset_info.shipped_path)) {
                throw CompilationException("Cannot create directory for: " + asset_info.shipped_path.string());
            }

            auto t0 = std::chrono::high_resolution_clock::now();
            bool ok = BakeLuaFileToJson(asset_info.raw_path, asset_info.shipped_path, desc_file.import_settings);

            if (ok && desc_file.import_settings.contains("required_fields")) { // schema check
                try {
                    std::ifstream in(asset_info.shipped_path);
                    nlohmann::json baked; in >> baked;
                    enforce_platform_rules(baked, desc_file.import_settings, platform, asset_info.raw_path.string());
                    for (auto& fld : desc_file.import_settings["required_fields"]) {
                        const std::string k = fld.get<std::string>();
                        if (!baked.contains(k)) {
                            std::cerr << "[LuaBake] ERROR: missing required field '" << k
                                      << "' in " << asset_info.raw_path << "\n";
                            ok = false;
                        }
                    }
                } 
                catch (...) {
                    ok = false;
                }
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t0).count();

            if (ok) {
                std::cout << "Script OK (" << ms << "ms): " << asset_info.raw_path.filename() << "\n";
            } 
            else {
                throw CompilationException("Script FAILED (" + std::to_string(ms) + "ms): " + asset_info.raw_path.string());
            }
        }

        bool Compiler::BakeLuaFileToJson(const std::filesystem::path& lua_in,
                                 const std::filesystem::path& json_out,
                                 const nlohmann::json& import_settings) const
        {
            try {
                sol::state lua;
                open_libs_from_settings(lua, import_settings);

                auto res = lua.safe_script_file(lua_in.string(), &sol::script_pass_on_error);
                if (!res.valid()) {
                    sol::error err = res;
                    std::cerr << "[LuaBake] ERROR in " << std::filesystem::absolute(lua_in).string() << "\n"
                                << "  " << err.what() << "\n";
                    return false;
                }

                sol::object ret = res.get<sol::object>();               
                if (ret.get_type() != sol::type::table) {
                    std::cerr << "[LuaBake] ERROR: " << lua_in << " must `return { ... }`\n";
                    return false;
                }

                const bool strict = import_settings.value("error_on_unsupported", true);
                nlohmann::json j = to_json_sol_table(ret.as<sol::table>(), strict);

                if (!verifyDirectory(json_out)) 
                    return false;

                std::ofstream out(json_out, std::ios::binary);
                out << j.dump(2);
                return true;
            } 
            catch (const std::exception& e) {
                std::cerr << "[LuaBake] exception in " << lua_in << ": " << e.what() << "\n";
                return false;
            }
        }


        std::string Compiler::GetCuttlefishExecutable() const {
            // Try to find cuttlefish executable
            std::filesystem::path cuttle_fish_path = assets_root.parent_path() / "vendor/cuttlefish/cuttlefish.exe";
            std::cout << cuttle_fish_path << std::endl;

            //Check if exists
            if (std::filesystem::exists(cuttle_fish_path)) {
                return cuttle_fish_path.string();
            }
        
            std::cout << "WARNING: Cuttlefish executable not found!" << std::endl;
            return "cuttlefish"; // Fallback
        }

        bool Compiler::CompressTextureDDS(unsigned char* pixels, int width, int height, int channels,
            const std::string& output_path, const std::string& format,
            const nlohmann::json& settings) const {
            try {
                std::string cuttlefish_exe = GetCuttlefishExecutable();

                // Create temporary PNG file (easier for cuttlefish to handle)
                std::string temp_input = "temp_" + std::to_string(getCurrentTimeStamp()) + ".png";
                if (!stbi_write_png(temp_input.c_str(), width, height, 4, pixels, width * 4)) {
                    std::cout << "Failed to write temporary PNG" << std::endl;
                    return false;
                }

                // WINDOWS SYSTEM() REQUIRES SPECIAL QUOTING [web:1061]
                std::stringstream cmd;

                // Method: Wrap ENTIRE command in outer quotes for Windows system()
                cmd << "\"";  // Start outer quotes
                cmd << "\"" << cuttlefish_exe << "\"";  // Quoted executable
                cmd << " -i \"" << temp_input << "\"";
                cmd << " -f " << format;
                cmd << " -Q " << settings.value("quality", "normal");
                cmd << " -s rgba";
                cmd << " -o \"" << output_path << "\"";
                cmd << " --file-format dds";
                cmd << " --create-dir";

                if (settings.value("generate_mipmaps", true)) {
                    cmd << " -m";
                }

                cmd << "\"";  // End outer quotes

                std::string final_command = cmd.str();
                std::cout << "Running: " << final_command << std::endl;

                int result = system(final_command.c_str());

                // Clean up temp file
                std::filesystem::remove(temp_input);

                if (result == 0) {
                    std::cout << "DDS compression successful" << std::endl;
                }
                else {
                    std::cout << "DDS compression failed with code: " << result << std::endl;
                }

                return result == 0;
            }
            catch (const std::exception& e) {
                std::cout << "Cuttlefish DDS compression failed: " << e.what() << std::endl;
                return false;
            }
        }

        std::string Compiler::GetASTCEncoderExecutable() const {

            // Try optimized versions in order of performance
            std::vector<std::filesystem::path> possible_paths = {
                assets_root.parent_path() / "vendor/astc-encoder/astcenc-avx2.exe",
                assets_root.parent_path() / "vendor/astc-encoder/astcenc-sse4.1.exe",
                assets_root.parent_path() / "vendor/astc-encoder/astcenc-sse2.exe"
            };

            for (const auto& path : possible_paths) {
                if (std::filesystem::exists(path)) {
                    std::cout << "Found ASTC encoder: " << path << std::endl;
                    return path.string();
                }
            }

            std::cout << "WARNING: ASTC encoder (astcenc) not found!" << std::endl;
            return "astcenc-avx2.exe"; // Fallback
        }

        std::string Compiler::ConvertToASTCBlockSize(const std::string& format) const {
            // Convert format like "ASTC_4x4" to "4x4" for astcenc
            if (format.find("ASTC_") == 0) {
                return format.substr(5); // Remove "ASTC_" prefix
            }

            // Default mappings
            if (format == "ASTC_4x4") return "4x4";
            if (format == "ASTC_6x6") return "6x6";
            if (format == "ASTC_8x8") return "8x8";

            return "4x4"; // Default fallback
        }

        bool Compiler::CompressTextureASTC(unsigned char* pixels, int width, int height, int channels,
            const std::string& output_path, const std::string& format,
            const nlohmann::json& settings) const {
            try {
                // Get astcenc executable (ARM's ASTC Encoder)
                std::string astcenc_exe = GetASTCEncoderExecutable();

                // Create temporary PNG input (astcenc works best with standard image formats)
                std::string temp_input = "temp_astc_" + std::to_string(getCurrentTimeStamp()) + ".png";
                if (!stbi_write_png(temp_input.c_str(), width, height, 4, pixels, width * 4)) {
                    std::cout << "Failed to write temporary PNG for ASTC" << std::endl;
                    return false;
                }

                // Build astcenc command with proper syntax
                std::stringstream cmd;

                // Windows system() double-quote wrapping
                cmd << "\"";  // Start outer quotes for Windows
                cmd << "\"" << astcenc_exe << "\"";  // Quoted executable path

                // astcenc syntax: astcenc -cl input.png output.astc block_size quality
                cmd << " -cl";  // Compress LDR (Low Dynamic Range)
                cmd << " \"" << temp_input << "\"";  // Input file
                cmd << " \"" + output_path + "\"";   // Output file
                cmd << " " << ConvertToASTCBlockSize(format);  // Block size (e.g., "4x4")
                cmd << " -" << settings.value("quality", "medium");  // Quality: -fastest, -fast, -medium, -thorough, -exhaustive

                // Additional ASTC options
                if (settings.value("srgb", false)) {
                    cmd << " -srgb";  // sRGB color space
                }

                cmd << "\"";  // End outer quotes for Windows

                std::string final_command = cmd.str();
                std::cout << "Running ASTC: " << final_command << std::endl;

                int result = system(final_command.c_str());

                // Clean up temp file
                std::filesystem::remove(temp_input);

                if (result == 0) {
                    std::cout << "ASTC compression successful" << std::endl;
                }
                else {
                    std::cout << "ASTC compression failed with code: " << result << std::endl;
                }

                return result == 0;
            }
            catch (const std::exception& e) {
                std::cout << "ASTC compression failed: " << e.what() << std::endl;
                return false;
            }
        }


		void Compiler::processAsset(Info& asset_info) {

            //Check for desc files and output
            auto asset_desc_path = assets_root / asset_info.relative_folder / (asset_info.raw_path.stem().string() + desc_ext);

            //Get desc obj
            Descriptor desc_obj;

            //Check if desc exists
            if (!std::filesystem::exists(asset_desc_path)) {
                desc_obj = createDefaultDesc(asset_info, asset_desc_path);
            }
            else {
                desc_obj = readDescFile(asset_info, asset_desc_path);
            }

            //Update asset GUID
            asset_info.guid = desc_obj.guid;

            //Check if asset is compilable
            if (Assets::isAssetCompilable(asset_info.type)) {

                //Compiling operation
                compileAndShip(desc_obj, asset_info);
            }
            else {

                //If asset is not compilable ship asset straight into 
                asset_info.shipped_path = output_dir / asset_info.relative_folder / asset_info.name;

                // Only ship if source is SIGNIFICANTLY newer than destination
                if (asset_info.raw_last_modified > (getFileLastModified(asset_info.shipped_path) + TOLERANCE_MS)) {

                    //Copy all assets
                    copyFile(asset_info.raw_path, asset_info.shipped_path);
                }
            }
		}
	}
}