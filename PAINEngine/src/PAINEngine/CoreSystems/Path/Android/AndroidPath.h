#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef ANDROID_PATH_HPP
#define ANDROID_PATH_HPP

#include "../Path.h"

namespace PAIN {
	namespace Path {

        class AndroidAssetStream : public IFileStream {
            FILE* file;
            AAsset* asset;
            size_t assetSize;
            FileMode mode;
        public:
            // For asset reading, use asset manager.
            AndroidAssetStream(AAssetManager* mgr, const std::string& path, FileMode mode_)
                : file(nullptr), asset(nullptr), assetSize(0), mode(mode_) {

                //Check if there is a valid asset manager
                if (mgr && mode == FileMode::Read) {
                    file = nullptr;
                    asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_STREAMING);
                    assetSize = asset ? AAsset_getLength(asset) : 0;
                }
                else if (mgr && mode == FileMode::Write) {
                    throw std::runtime_error("Assets are read only on android! Error creating stream!");
                }
                else {
                    asset = nullptr;
                    assetSize = 0;
                    file = fopen(path.c_str(), (mode == FileMode::Write) ? "wb" : "rb+");
                }
            }
            ~AndroidAssetStream() override {
                if (asset) AAsset_close(asset);
                if (file) fclose(file);

                asset = nullptr;
                file = nullptr;
            }
            size_t read(void* buffer, size_t size) override {
                if (asset) return static_cast<size_t>(AAsset_read(asset, buffer, size));
                if (file) return fread(buffer, 1, size, file);
                return 0;
            }
            size_t write(const void* buffer, size_t size) override {
                if (asset) return 0; // assets in APK are read-only
                if (file) return fwrite(buffer, 1, size, file);
                return 0;
            }
            void flush() override { if (file) fflush(file); }
            void seek(size_t pos) override {
                if (asset) AAsset_seek(asset, pos, SEEK_SET);
                if (file) fseek(file, pos, SEEK_SET);
            }
            size_t tell() override {
                if (asset) return assetSize - AAsset_getRemainingLength(asset);
                if (file) return static_cast<size_t>(ftell(file));
                return 0;
            }
            bool eof() const override {
                if (asset) return (AAsset_getRemainingLength(asset) == 0);
                if (file) return feof(file) != 0;
                return true;
            }
            size_t size() const override { return asset ? assetSize : 0; /* Can extend for files */ }
            bool good() const override { return (asset || file); }
        };

        class AndroidPath : public Path {
        private:
            // Path variables
            std::string app_name;
            std::string package_name;
            std::string internal_path;
            std::string external_path;
            std::string cache_path;

            // Native app reference
            android_app* m_app;

            // Internal path functions
            std::string normalizePath(const std::string& path) const;
            std::string normalizeFileIOPath(const std::string& path) const;

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

            std::string getVirtualParentPath(std::string const& virtual_path) const override;

            // Override functions from path interface
			void registerVirtualPath(const std::string& alias, const std::string& path, bool create_new = false) override;
			void updateVirtualPath(const std::string& alias, const std::string& path) override;
            std::string resolvePath(const std::string& virtualPath) const override;
            std::string resolvePath(const std::string& alias, std::string const& relative) const override;
            std::vector<std::string> listFiles(const std::string& virtualPath, const std::string& filter = "", const std::string& extension = "") const override;
            std::vector<std::string> listDirectories(const std::string& virtualPath, const std::string& filter = "") const override;
            bool pathExists(const std::string& virtualPath) const override;
            bool createDirectory(const std::string& virtualPath) const override;

            //Create file stream
            std::unique_ptr<IFileStream> createFileStream(const std::string& virtualPath, FileMode mode) override;

            // Android-specific asset methods
            bool isAssetPath(const std::string& virtualPath) const;
        };

	}
}

#endif
#endif
