#pragma once

#ifndef ASSET_TYPES
#define ASSET_TYPES

enum class AssetType {
    Texture,    // .png, .jpg
    Model,      // .obj
    Audio,      // .wav, .mp3, .ogg
    Script,     // .lua
    Data,       // .json
    Shader,     // .vert, .frag
    Other
};

#endif