#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef ANDROID_PATH_HPP
#define ANDROID_PATH_HPP

#include "../Path.h"

namespace PAIN {
	namespace Path {

        class AndroidPath : public Path {
        private:
            // Path variables
            std::string app_name;
            std::string package_name;
            std::string internal_path;
            std::string external_path;
            std::string cache_path;
            std::string assets_path;

            // Native app reference
            android_app* m_app;

            // Internal path functions
            std::string normalizePath(const std::string& path) const;
            bool isValidPath(const std::string& path) const;
            bool createDirectoryRecursive(const std::string& path) const;

            // Private initialize and destroy
            void init();
            void destroy();

        public:
            AndroidPath(void* app) { initWithNativeApp(static_cast<android_app*>(app)); }
            virtual ~AndroidPath() { destroy(); }

            // Native initialization - takes android_app* pointer
            void initWithNativeApp(android_app* app);

            // Override functions from path interface
			void registerVirtualPath(const std::string& alias, const std::string& path, bool create_new = false) override;
			void updateVirtualPath(const std::string& alias, const std::string& path) override;
            std::string resolvePath(const std::string& virtualPath) const override;
            std::vector<std::string> listFiles(const std::string& virtualPath, const std::string& filter = "", const std::string& extension = "") const override;
            std::vector<std::string> listDirectories(const std::string& virtualPath, const std::string& filter = "") const override;
            bool pathExists(const std::string& virtualPath) const override;
            bool createDirectory(const std::string& virtualPath) const override;

            // Android-specific asset methods
            bool isAssetPath(const std::string& virtualPath) const;
        };

	}
}

#endif
#endif
