#include "AssetCompiler.h"

namespace PAIN {
	namespace Assets {


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
                if (!verifyDirectory(desc_path)) {
                    std::cout << "Directory doesnt exist: " << desc_path << std::endl;
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
                settings["android_compression"] = "ASTC";
                settings["generate_mipmaps"] = true;
                settings["max_size"] = 1024;
                settings["srgb"] = true;
                break;

            case Type::Audio:
                settings["compression"] = "OGG";
                settings["quality"] = 0.8;
                settings["loop"] = false;
                break;

            case Type::Model:
                settings["generate_lods"] = true;
                settings["optimize_vertices"] = true;
                settings["weld_threshold"] = 0.001;
                break;
            default:
                break;
            }

            return settings;
        }

        Descriptor Compiler::createDefaultDesc(Info const& asset, std::filesystem::path const& desc_path) const {

            //Default descriptor
            Descriptor desc;
            desc.guid = GUID::Generate();
            desc.type = asset.type;
            desc.import_settings = generateDefaultCompileSettings(desc.type);
            desc.raw_last_modified = asset.raw_last_modified;

            return desc;
        }

        Descriptor Compiler::readDescFile(Info const& asset, std::filesystem::path const& desc_path) const {
            try {
                std::ifstream file(desc_path);
                nlohmann::json desc_json;
                file >> desc_json;

                //Get descriptor info
                Descriptor desc;
                desc.guid = GUID(desc_json["guid"].get<std::string>());
                desc.type = stringToAssetType(desc_json["type"].get<std::string>());
                desc.import_settings = desc_json.value("import_settings", nlohmann::json{});
                desc.raw_last_modified = desc_json.value("raw_last_modified", 0ULL);

                return desc;
            }
            catch (const std::exception& e) {
                std::cout << "Error encountered reading desc file, reverting to default." << std::endl;
                return createDefaultDesc(asset, desc_path);
            }
        }

        bool Compiler::saveDescFile(Descriptor const& desc_file, std::filesystem::path const& desc_path) const {
            //Save generated descriptor
            try {
                //Verify directory
                if (!verifyDirectory(desc_path)) {
                    std::cout << "Directory doesnt exist: " << desc_path << std::endl;
                    return false;
                }

                nlohmann::json desc_json;
                desc_json["guid"] = desc_file.guid.ToString();
                desc_json["type"] = assetTypeToString(desc_file.type);
                desc_json["import_settings"] = desc_file.import_settings;
                desc_json["raw_last_modified"] = desc_file.raw_last_modified;

                std::ofstream file(desc_path, std::ios::out);
                if (file << desc_json.dump(2)) {
                    std::cout << "Descriptor file saved at: " << desc_path << std::endl;
                    return true;
                }
                std::cout << "Error saving default desc file to: " << desc_path << std::endl;
                return false;
            }
            catch (const std::exception& e) {
                std::cout << "Error saving default desc file to: " << desc_path << std::endl;
                return false;
            }
        }

		void Compiler::processAsset(Info& asset_info) {

			//Check if asset is a compilable
			if (Assets::isAssetCompilable(asset_info.type)) {

				//Get descriptor file path for asset
				std::string asset_name = asset_info.name.substr(0, asset_info.name.find_first_of("."));
				auto asset_desc_path = desc_path / asset_info.relative_folder / (asset_name + desc_ext);

                //Default desc obj
                Descriptor desc_obj;

                //Check if asset desc exists
                if (std::filesystem::exists(asset_desc_path)) {

                    //Try to read desc obj, fallback and create default if unable to read
                    desc_obj = readDescFile(asset_info, asset_desc_path);
                }
                else {

                    //Create a default desc file
                    desc_obj = createDefaultDesc(asset_info, asset_desc_path);
                }

                //Compliation operation

                //Save desc file
                saveDescFile(desc_obj, asset_desc_path);

                //Update asset info
                asset_info.guid = desc_obj.guid;
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