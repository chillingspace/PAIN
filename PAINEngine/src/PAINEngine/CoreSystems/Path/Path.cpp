#include "pch.h"
#include "path.h"

namespace PAIN {
	namespace Path {

		//std::filesystem::path Path::normalizePath(const std::filesystem::path& path) const {
		//	return std::filesystem::path();
		//}

		//std::filesystem::path Path::findRootDirectory() const {
		//	return std::filesystem::path();
		//}

		//void Path::init() {

		//	//Json config
		//	json config;

		//	try {
		//		//Convert the path to json
		//		std::ifstream file("assets/Config.json");
		//		if (!file.is_open()) {
		//			PN_CORE_WARN("Could not open config file: {}", "assets/Config.json");
		//			return;
		//		}
		//		file >> config;
		//		auto const& data = config.at("WindowsConfig");
		//	}
		//	catch (const nlohmann::json::exception& e) {
		//		PN_CORE_WARN(e.what());
		//		PN_CORE_WARN("Paths config invalid! No default virtual path registered");
		//	}

		//}

		//void Path::registerVirtualPath(const std::string& alias, const std::string& path) {
		//}

		//void Path::updateVirtualPath(const std::string& alias, const std::string& path)  {

		//}

		//std::filesystem::path Path::resolvePath(const std::string& virtualPath) const {
		//	return std::filesystem::path();
		//}

		//std::vector<std::filesystem::path> Path::listFiles(const std::string& virtualPath, const std::string& filter, const std::string& ext ) const {
		//	return std::vector<std::filesystem::path>();
		//}

		//std::vector<std::filesystem::path> Path::listDirectories(const std::string& virtualPath, const std::string& filter) const {
		//	return std::vector<std::filesystem::path>();
		//}

		//std::string Path::getAlias(const std::string& virtualPath) const {
		//	return "";
		//}

		//std::string Path::convertToVirtualPath(const std::string& alias, const std::string& path) const {
		//	return "";
		//}

		//bool Path::pathExists(const std::string& virtualPath) const {
		//	return false;
		//}

		//void Path::logVirtualPaths() const {
		//}
	}
}
