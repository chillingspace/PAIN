#pragma once

#ifndef __ANDROID_FS_H__
#define __ANDROID_FS_H__

#include "pch.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <string>

// Global asset manager - declare as inline to avoid multiple definition errors
extern AAssetManager* assetManager;

// Inline function so it can be in the header
inline std::string ReadFileAndroid(const std::string& path) {
    if (!assetManager) {
        PN_CORE_INFO("AssetManager not initialized!");
        return "";
    }

    AAsset* asset = AAssetManager_open(assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        PN_CORE_INFO("Failed to open: %s", path.c_str());
        return "";
    }

    off_t length = AAsset_getLength(asset);
    std::string buffer(length, '\0');
    AAsset_read(asset, &buffer[0], length);
    AAsset_close(asset);

    return buffer;
}

#endif
