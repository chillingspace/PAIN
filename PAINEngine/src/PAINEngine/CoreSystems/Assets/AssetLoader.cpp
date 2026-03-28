#include "pch.h"
#include "AssetLoader.h"

#include "CoreSystems/Windows/Window.h"

#ifdef PN_PLATFORM_ANDROID
#include <ktx.h>
#include <algorithm>
#include <cctype>

// ========================================
// DEFINE MISSING ASTC sRGB CONSTANTS
// ========================================
#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR   0x93D0
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR   0x93D1
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR   0x93D2
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR   0x93D3
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR   0x93D4
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR   0x93D5
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR   0x93D6
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR   0x93D7
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR  0x93D8
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR  0x93D9
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR  0x93DA
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR 0x93DB
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR 0x93DC
#endif

#ifndef GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR 0x93DD
#endif

#endif

#undef max
#undef min

namespace PAIN {
	namespace Assets {
#ifdef PN_PLATFORM_ANDROID
        namespace {
            std::string ToLowerAscii(std::string value) {
                std::transform(value.begin(), value.end(), value.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return value;
            }

            bool ContainsAny(std::string const& value, std::initializer_list<const char*> needles) {
                for (const char* needle : needles) {
                    if (value.find(needle) != std::string::npos) {
                        return true;
                    }
                }
                return false;
            }

            bool IsLikelyLinearDataTexturePath(std::string const& virtual_path) {
                const std::string lower = ToLowerAscii(virtual_path);

                // Keep color/HDR cubemap paths untouched.
                if (ContainsAny(lower, { "skybox", "cubemap", "ibl", "irradiance", "prefilter", "brdf" })) {
                    return false;
                }

                return ContainsAny(lower, {
                    "_normal", "normal_", "normalmap", "_nrm",
                    "_ao", "ao_", "occlusion",
                    "_rough", "roughness",
                    "_metal", "metallic", "metalness",
                    "_orm", "_rma", "_arm", "_mra", "_mrao"
                });
            }

            GLenum ToLinearAstcFormat(GLenum format) {
                switch (format) {
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR: return GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR: return GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR: return GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR: return GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR: return GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR: return GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR: return GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR: return GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR: return GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR: return GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR: return GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR: return GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR: return GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
                case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR: return GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
                default: return 0;
                }
            }
        }
#endif

        void Loader::RegisterLoader(Type const& type, LoaderFunc const& func) {

            //Check if type already regist
            if (asset_loader.find(type) != asset_loader.end()) {
                std::cout << "Loader of type: " << assetTypeToString(type) << " already registered. Overriding with new loader." << std::endl;
            }

            asset_loader[type] = func;
        }

        LoaderFunc Loader::GetLoader(Type const& type) const {
            
            //Check if loader mesh_id
            if (!CheckLoader(type)) {
                throw std::runtime_error("Loader does not exist! Unable to get loader function!");
            }

            return asset_loader.at(type);
        }

        bool Loader::CheckLoader(Type const& type) const {
            if (asset_loader.find(type) != asset_loader.end()) {
                return true;
            }

            return false;
        }

        std::unordered_map<GUID, std::shared_ptr<IAsset>> Loader::ImportAssetRegistry(std::string const& virtual_path) const {
            std::unordered_map<GUID, std::shared_ptr<IAsset>> assets;

            //Use your custom input stream API
            auto fileStream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!fileStream || !fileStream->good()) {
                throw std::runtime_error("Failed to open asset registry file");
            }

            //Read entire file into a buffer
            std::vector<uint8_t> buffer(fileStream->size());
            size_t totalRead = 0;
            while (totalRead < buffer.size()) {
                size_t chunk = fileStream->read(buffer.data() + totalRead, buffer.size() - totalRead);
                if (chunk == 0) break;
                totalRead += chunk;
            }
            buffer.resize(totalRead);

            //Parse JSON from buffer (assumes UTF-8 text)
            nlohmann::json json_package;
            try {
                json_package = nlohmann::json::parse(buffer.begin(), buffer.end());
            }
            catch (const std::exception& e) {
                PN_CORE_ERROR("JSON parse error: {}", e.what());
                return assets;
            }

            //Original asset registry parsing logic
            for (auto it = json_package.begin(); it != json_package.end(); ++it) {
                GUID guid = GUID(it.key());

                //ensure GUID is valid
                if (!guid.IsValid()) continue;

                const auto& obj = it.value();

                IAsset asset;
                asset.guid = guid;
                asset.type = stringToAssetType(obj["type"]);
                asset.name = obj.value("name", "");
                asset.main_relative_path = std::filesystem::path(obj.value("main_relative_path", ""));
                asset.shipped_relative_path = std::filesystem::path(obj.value("shipped_relative_path", ""));
                assets[guid] = std::make_shared<IAsset>(asset);
            }
            fileStream = nullptr;
            return assets;
        }

#ifdef PN_PLATFORM_ANDROID
        void Loader::extractKTX(std::string const& virtual_path, std::shared_ptr<Texture> tex) const {
            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good())
                throw std::runtime_error("Failed to open KTX file: " + virtual_path);

            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size())
                throw std::runtime_error("Failed to read all KTX data: " + virtual_path);
            stream = nullptr;

            // Load KTX texture
            ktxTexture* kTexture = nullptr;
            KTX_error_code result = ktxTexture_CreateFromMemory(
                data.data(),
                data.size(),
                KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                &kTexture
            );

            if (result != KTX_SUCCESS || !kTexture)
                throw std::runtime_error("Failed to parse KTX from memory: " + virtual_path);

            // ========================================
            // VERIFY KTX FILE INTEGRITY
            // ========================================
            PN_CORE_INFO("KTX file info for '{}':", virtual_path);
            PN_CORE_INFO("  Base dimensions: {}x{}", kTexture->baseWidth, kTexture->baseHeight);
            PN_CORE_INFO("  Mip levels: {}", kTexture->numLevels);
            PN_CORE_INFO("  Faces: {}", kTexture->numFaces);
            PN_CORE_INFO("  Data size: {}", kTexture->dataSize);
            PN_CORE_INFO("  Is array: {}", kTexture->isArray);
            PN_CORE_INFO("  Is compressed: {}", kTexture->isCompressed);

            if (kTexture->classId == ktxTexture1_c) {
                ktxTexture1* ktx1 = reinterpret_cast<ktxTexture1*>(kTexture);
                PN_CORE_INFO("  GL internal format: 0x{:X}", ktx1->glInternalformat);
                PN_CORE_INFO("  GL format: 0x{:X}", ktx1->glFormat);
                PN_CORE_INFO("  GL type: 0x{:X}", ktx1->glType);
                PN_CORE_INFO("  GL base internal format: 0x{:X}", ktx1->glBaseInternalformat);
            }

            // Extract metadata
            tex->width = static_cast<int>(kTexture->baseWidth);
            tex->height = static_cast<int>(kTexture->baseHeight);
            tex->mips = kTexture->numLevels ? static_cast<int>(kTexture->numLevels) : 1;
            int numFaces = kTexture->numFaces ? kTexture->numFaces : 1;

            tex->format = TextureFormat::UNKNOWN;
            tex->is_cube_map = (numFaces == 6);
            tex->is_compressed = kTexture->isCompressed == KTX_TRUE;

            // Get OpenGL format
            if (kTexture->classId == ktxTexture1_c) {
                ktxTexture1* ktx1 = reinterpret_cast<ktxTexture1*>(kTexture);
                tex->glTexFormat = ktx1->glInternalformat;
                tex->glBaseFormat = ktx1->glFormat;
                tex->glDataType = ktx1->glType;

                if (!tex->is_cube_map && IsLikelyLinearDataTexturePath(virtual_path)) {
                    const GLenum linearAstc = ToLinearAstcFormat(static_cast<GLenum>(tex->glTexFormat));
                    if (linearAstc != 0 && linearAstc != tex->glTexFormat) {
                        PN_CORE_INFO(
                            "Forcing linear ASTC decode for data texture '{}' (internal 0x{:X} -> 0x{:X})",
                            virtual_path, tex->glTexFormat, linearAstc
                        );
                        tex->glTexFormat = linearAstc;
                    }
                }
            }
            else {
                ktxTexture_Destroy(kTexture);
                throw std::runtime_error("Only KTX1 format is supported!");
            }

            // ========================================
            // SET BLOCK DIMENSIONS BASED ON FORMAT
            // ========================================
            switch (tex->glTexFormat) {
            case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 4;
                tex->blockHeight = 4;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 4x4 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 5;
                tex->blockHeight = 4;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 5x4 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 5;
                tex->blockHeight = 5;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 5x5 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 6;
                tex->blockHeight = 5;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 6x5 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 6;
                tex->blockHeight = 6;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 6x6 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 8;
                tex->blockHeight = 5;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 8x5 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 8;
                tex->blockHeight = 6;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 8x6 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 8;
                tex->blockHeight = 8;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 8x8 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 10;
                tex->blockHeight = 5;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 10x5 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 10;
                tex->blockHeight = 6;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 10x6 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 10;
                tex->blockHeight = 8;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 10x8 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 10;
                tex->blockHeight = 10;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 10x10 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 12;
                tex->blockHeight = 10;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 12x10 RGBA");
                break;

            case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
                tex->format = TextureFormat::ASTC;
                tex->blockWidth = 12;
                tex->blockHeight = 12;
                tex->blockSize = 16;
                PN_CORE_INFO("KTX Format: ASTC 12x12 RGBA");
                break;

            case GL_RGB16F:
            case GL_RGBA16F:
            case GL_R11F_G11F_B10F:
                tex->format = TextureFormat::FLOAT_HDR;
                tex->is_compressed = false;
                tex->blockWidth = 1;
                tex->blockHeight = 1;
                tex->blockSize = 0;
                PN_CORE_INFO("KTX Format: uncompressed HDR 0x{:X}, base format 0x{:X}, type 0x{:X}",
                    tex->glTexFormat, tex->glBaseFormat, tex->glDataType);
                break;

            default:
                PN_CORE_WARN("Unknown KTX format 0x{:X}, compressed={}, base format 0x{:X}, type 0x{:X}",
                    tex->glTexFormat, tex->is_compressed, tex->glBaseFormat, tex->glDataType);
                tex->blockWidth = tex->is_compressed ? 4 : 1;
                tex->blockHeight = tex->is_compressed ? 4 : 1;
                tex->blockSize = tex->is_compressed ? 16 : 0;
                break;
            }

            tex->mipOffsets.clear();
            tex->mipSizes.clear();
            tex->data.clear();

            PN_CORE_INFO("Loading KTX: '{}' {}x{}, {} mips, {} faces",
                virtual_path, tex->width, tex->height, tex->mips, numFaces);

            // ========================================
            //  USE KTX API TO GET ACTUAL MIP SIZES
            // ========================================
            for (uint32_t face = 0; face < numFaces; ++face) {
                for (uint32_t mip = 0; mip < static_cast<uint32_t>(tex->mips); ++mip) {
                    // Get offset
                    ktx_size_t imageOffset = 0;
                    KTX_error_code ofsResult = ktxTexture_GetImageOffset(
                        kTexture, mip, 0, face, &imageOffset);

                    if (ofsResult != KTX_SUCCESS) {
                        ktxTexture_Destroy(kTexture);
                        throw std::runtime_error("Failed to get image offset for mip " +
                            std::to_string(mip) + ", face " + std::to_string(face));
                    }

                    // GET ACTUAL SIZE FROM KTX
                    ktx_size_t mipSize = ktxTexture_GetImageSize(kTexture, mip);
                    if (mipSize == 0) {
                        ktxTexture_Destroy(kTexture);
                        throw std::runtime_error("No data for KTX mip level " +
                            std::to_string(mip) + " face " + std::to_string(face));
                    }

                    // Store offset and size
                    tex->mipOffsets.push_back(tex->data.size());
                    tex->mipSizes.push_back(static_cast<size_t>(mipSize));

                    // Copy mip data
                    const uint8_t* mipData = reinterpret_cast<const uint8_t*>(ktxTexture_GetData(kTexture)) + imageOffset;
                    tex->data.insert(tex->data.end(), mipData, mipData + mipSize);

                    PN_CORE_TRACE("  Mip {} face {}: offset {}, size {} bytes",
                        mip, face, tex->mipOffsets.back(), mipSize);
                }
            }

            PN_CORE_INFO("Total KTX data: {} bytes", tex->data.size());

            //Free up memory
            ktxTexture_Destroy(kTexture);
            kTexture = nullptr;
            std::vector<uint8_t>().swap(data);
        }
#else
        void Loader::extractDDS(std::string const& virtual_path, std::shared_ptr<Texture> tex) const {
            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good())
                throw std::runtime_error("Failed to open DDS file: " + virtual_path);

            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size())
                throw std::runtime_error("Failed to read full DDS file: " + virtual_path);
            stream = nullptr;

            size_t offset = 0;

            // Verify magic
            if (data.size() < 4)
                throw std::runtime_error("DDS file data too small for magic number!");
            if (std::memcmp(data.data(), "DDS ", 4) != 0)
                throw std::runtime_error("Not a DDS file!");
            offset += 4;

            // Read header
            if (data.size() < offset + 124)
                throw std::runtime_error("DDS file header too small");
            const uint32_t* header = reinterpret_cast<const uint32_t*>(data.data() + offset);

            tex->height = header[2];
            tex->width = header[3];
            uint32_t mipMapCount = header[7] ? header[7] : 1;
            tex->mips = mipMapCount;

            bool isDX10 = (header[20] == 0x30315844);  // 'DX10'
            offset += 124;

            // Parse format
            if (isDX10) {
                if (data.size() < offset + 20)
                    throw std::runtime_error("DDS file too small for DX10 header");
                const uint32_t* dx10Header = reinterpret_cast<const uint32_t*>(data.data() + offset);

                uint32_t dxgiFormat = dx10Header[0];

                // ========================================
                // SET FORMAT AND BLOCK INFO
                // ========================================
                switch (dxgiFormat) {
                case 98:  // DXGI_FORMAT_BC7_UNORM
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                    tex->blockWidth = 4;
                    tex->blockHeight = 4;
                    tex->blockSize = 16;
                    PN_CORE_INFO("DDS Format: BC7 RGBA");
                    break;

                case 99:  // DXGI_FORMAT_BC7_UNORM_SRGB
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
                    tex->blockWidth = 4;
                    tex->blockHeight = 4;
                    tex->blockSize = 16;
                    PN_CORE_INFO("DDS Format: BC7 sRGB RGBA");
                    break;

                case 77:  // DXGI_FORMAT_BC3_UNORM (DXT5)
                    tex->format = TextureFormat::BC3;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                    tex->blockWidth = 4;
                    tex->blockHeight = 4;
                    tex->blockSize = 16;
                    PN_CORE_INFO("DDS Format: BC3/DXT5 RGBA");
                    break;

                case 95:  // DXGI_FORMAT_BC6H_UF16
                    tex->format = TextureFormat::BC6H;
                    tex->glTexFormat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
                    tex->blockWidth = 4;
                    tex->blockHeight = 4;
                    tex->blockSize = 16;
                    PN_CORE_INFO("DDS Format: BC6H HDR (NO ALPHA)");
                    break;

                case 96:  // DXGI_FORMAT_BC6H_SF16
                    tex->format = TextureFormat::BC6H;
                    tex->glTexFormat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
                    tex->blockWidth = 4;
                    tex->blockHeight = 4;
                    tex->blockSize = 16;
                    PN_CORE_INFO("DDS Format: BC6H Signed HDR (NO ALPHA)");
                    break;

                case 71:  // DXGI_FORMAT_BC1_UNORM (DXT1)
                    tex->format = TextureFormat::BC1;
                    tex->glTexFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                    tex->blockWidth = 4;
                    tex->blockHeight = 4;
                    tex->blockSize = 8;  // BC1 is 8 bytes!
                    PN_CORE_INFO("DDS Format: BC1/DXT1 RGB (NO ALPHA)");
                    break;

                default:
                    throw std::runtime_error("Unsupported DXGI format: " + std::to_string(dxgiFormat));
                }

                offset += 20;
            }
            else {
                throw std::runtime_error("Legacy DDS format detected. Only DX10 format supported!");
            }

            uint32_t dwCaps2 = header[28];
            tex->is_cube_map = (dwCaps2 & 0x200) != 0;
            int faces = tex->is_cube_map ? 6 : 1;

            PN_CORE_INFO("Loading DDS: '{}' {}x{}, {} mips, {} faces",
                virtual_path, tex->width, tex->height, tex->mips, faces);

            // ========================================
            // READ MIP DATA WITH CORRECT BLOCK SIZE
            // ========================================
            for (int face = 0; face < faces; ++face) {
                int mipW = tex->width;
                int mipH = tex->height;

                for (uint32_t mip = 0; mip < mipMapCount; ++mip) {
                    // Calculate blocks using actual block dimensions
                    int blocks_w = (mipW + tex->blockWidth - 1) / tex->blockWidth;
                    int blocks_h = (mipH + tex->blockHeight - 1) / tex->blockHeight;
                    size_t mipSize = blocks_w * blocks_h * tex->blockSize;

                    if (data.size() < offset + mipSize)
                        throw std::runtime_error("DDS too small for mip " + std::to_string(mip) +
                            ", face " + std::to_string(face));

                    // Store offset and size
                    tex->mipOffsets.push_back(tex->data.size());
                    tex->mipSizes.push_back(mipSize);

                    // Copy data
                    size_t currentOffset = tex->data.size();
                    tex->data.resize(tex->data.size() + mipSize);
                    std::memcpy(tex->data.data() + currentOffset, data.data() + offset, mipSize);

                    PN_CORE_TRACE("  Mip {} face {}: offset {}, size {} bytes ({}x{} blocks)",
                        mip, face, tex->mipOffsets.back(), mipSize, blocks_w, blocks_h);

                    offset += mipSize;
                    mipW = std::max(1, mipW / 2);
                    mipH = std::max(1, mipH / 2);
                }
            }

            PN_CORE_INFO("Total DDS data: {} bytes", tex->data.size());

            //Free up memory
            std::vector<uint8_t>().swap(data);
        }
#endif

        std::shared_ptr<Texture> Loader::ImportTexture(std::string const& virtual_path) const {

            auto tex = std::make_shared<Texture>();

            // EXTRACT DATA (PLATFORM-SPECIFIC)
#ifdef PN_PLATFORM_ANDROID
            extractKTX(virtual_path, tex);
#else
            extractDDS(virtual_path, tex);
#endif

            PN_CORE_TRACE("Texture {} loaded, not uploaded to GPU yet.", virtual_path);
            return tex;
        }

		std::shared_ptr<Model> Loader::ImportModel(std::string const& virtual_path) const {
            PN_CORE_INFO("ImportModel {}", virtual_path);

            //Check if virtual path has valid extension
            if (std::filesystem::path(path_service->resolvePath(virtual_path)).extension() != ".mesh") {
                throw std::runtime_error("Invalid file type: " + virtual_path);
            }

            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good()) {
                PN_CORE_ERROR("Failed to open model file: {}", virtual_path);
                throw std::runtime_error("Failed to open model file: " + virtual_path);
            }

            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size()) {
                PN_CORE_ERROR("Failed to read full model file: {}", virtual_path);
                throw std::runtime_error("Failed to read full model file: " + virtual_path);
            }

            Model asset;
            size_t offset = 0;

            // Lamda helper functions
            auto require = [&](size_t n) {
                if (offset + n > data.size()) { 
                    PN_CORE_ERROR("Unexpected end of model file");
                    throw std::runtime_error("Unexpected end of model file"); 
                }
                };
            auto readMem = [&](void* dst, size_t n) {
                if (n == 0) return;
                require(n); std::memcpy(dst, data.data() + offset, n); offset += n;
                };
            auto readStr = [&](std::string& str) {
                uint32_t len = 0;
                readMem(&len, sizeof(len));
                if (len > 2048) { // Safety Cap
                    PN_CORE_ERROR("String too long: {} at offset {}", len, offset);
                    throw std::runtime_error("Corrupt file: String too long");
                }
                str.resize(len);
                if (len > 0) {
                    readMem(str.data(), len);
                    
                    if (str.back() == '\0') str.pop_back();
                }
                };


            PN_CORE_INFO("File size: {} bytes", data.size());

            PN_CORE_TRACE("ImportModel: Before reading bounding box");

            // Read bounding box
            PN_CORE_TRACE("sizeof(glm::vec3) = {}", sizeof(glm::vec3));
            PN_CORE_TRACE("Offset before AABB: {}", offset);
            readMem(&asset.aabbMin, sizeof(asset.aabbMin));
            PN_CORE_TRACE("Offset after aabbMin: {}", offset);
            readMem(&asset.aabbMax, sizeof(asset.aabbMax));
            PN_CORE_TRACE("Offset after aabbMax: {}", offset);

            PN_CORE_TRACE("ImportModel: Before reading LODs");

            // Read LODs
            uint32_t lodCount = 0;
            readMem(&lodCount, sizeof(lodCount));
            PN_CORE_TRACE("lodCount = {} at offset {}", lodCount, offset - 4);
            PN_CORE_TRACE("ImportModel: after readMem for LOD");
            PN_CORE_TRACE("Attempting to allocate {} bytes for `asset`", lodCount);
            asset.lods.resize(lodCount);
            PN_CORE_TRACE("ImportModel: after reszing asset for LOD");
            readMem(asset.lods.data(), lodCount * sizeof(uint32_t));

            PN_CORE_TRACE("ImportModel: Before reading vertices");

            // Vertices/Indices
            uint32_t vtxCount = 0, idxCount = 0;
            readMem(&vtxCount, sizeof(vtxCount));
            readMem(&idxCount, sizeof(idxCount));
            asset.vertices.resize(vtxCount);
            asset.indices.resize(idxCount);
            readMem(asset.vertices.data(), vtxCount * sizeof(Vertex));
            readMem(asset.indices.data(), idxCount * sizeof(uint32_t));

            PN_CORE_TRACE("ImportModel: Read {} vertices and {} indices from {}", vtxCount, idxCount, virtual_path);

            // Submeshes
            uint32_t submeshCount = 0;
            readMem(&submeshCount, sizeof(submeshCount));
            asset.submeshes.resize(submeshCount);
            for (Submesh& sm : asset.submeshes) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                sm.name.resize(nameLen);
                readMem(sm.name.data(), nameLen);
                readMem(&sm.materialIndex, sizeof(sm.materialIndex));
                readMem(&sm.firstIndex, sizeof(sm.firstIndex));
                readMem(&sm.indexCount, sizeof(sm.indexCount));
                readMem(&sm.vertexOffset, sizeof(sm.vertexOffset));
            }

            // MorphTargets
            uint32_t morphCount = 0;
            readMem(&morphCount, sizeof(morphCount));
            asset.morphTargets.resize(morphCount);
            for (MorphTarget& mt : asset.morphTargets) {
                uint32_t nameLen = 0, deltaCount = 0;
                readMem(&nameLen, sizeof(nameLen));
                mt.name.resize(nameLen);
                readMem(mt.name.data(), nameLen);

                readMem(&deltaCount, sizeof(deltaCount));
                mt.positionDeltas.resize(deltaCount);
                mt.normalDeltas.resize(deltaCount);
                readMem(mt.positionDeltas.data(), deltaCount * sizeof(glm::vec3));
                readMem(mt.normalDeltas.data(), deltaCount * sizeof(glm::vec3));
            }

            // Skeleton Bones
            uint32_t boneCount = 0;
            readMem(&boneCount, sizeof(boneCount));
            asset.skeleton.resize(boneCount);
            for (size_t i = 0; i < boneCount; ++i) {
                Bone& b = asset.skeleton[i];
                readStr(b.name);
                readMem(&b.parent, sizeof(b.parent));
                readMem(&b.bindPose, sizeof(glm::mat4));

                //PN_CORE_INFO("  Bone [{}] '{}' -> Parent Index: {}", i, b.name, b.parent);
            }

            // check if bones are well or poorly ordered
            {
                // parents should come before children in vector order
                // if not, could cause issues/require a lot more work in transforming vertices for animation
                bool ok = true;
                for (int i{}; i < asset.skeleton.size(); ++i) {
                    if (asset.skeleton[i].parent > i) {
                        PN_CORE_WARN("Malformed bone order in {}", virtual_path);
                        ok = false;
                        break;
                    }
                }
                if (ok) PN_CORE_INFO("Bone order OK: {}", virtual_path);
                else PN_CORE_WARN("Bone order malformed: {}", virtual_path);
            }

            // Animations
            uint32_t animCount = 0;
            readMem(&animCount, sizeof(animCount));
            //PN_CORE_INFO("--- Reading {} Animations ---", animCount);
            asset.animations.resize(animCount);
            for (AnimationClip& anim : asset.animations) {
                readStr(anim.name);
                readMem(&anim.duration, sizeof(anim.duration));
                readMem(&anim.isAdditive, sizeof(anim.isAdditive));

                uint32_t trackCount = 0;
                readMem(&trackCount, sizeof(trackCount));
                //anim.tracks.resize(trackCount);
                //PN_CORE_TRACE("  Anim '{}' ({}s) has {} tracks", anim.name, anim.duration, trackCount);

                int no_bone_tracks{};
                for (size_t i{}; i < trackCount; ++i) {
                    //uint32_t boneLen = 0, keyCount = 0;
                    //readMem(&boneLen, sizeof(boneLen));
                    //track.boneName.resize(boneLen);
                    //readMem(track.boneName.data(), boneLen);

                    std::string boneName;
                    readStr(boneName); 
                    
                    bool foundBone = false;
                    for (const auto& b : asset.skeleton) {
                        if (b.name == boneName) { foundBone = true; break; }
                    }
                    if (!foundBone) {
                        PN_CORE_WARN("  [WARNING] Track for '{}' NOT FOUND in skeleton!", boneName);
                    }

                    // unnamed bones suck tf, but i guess this could cause more problems
                    // if bone doesn't exist, store as root xform or scene xform
                    //auto it = std::find_if(asset.skeleton.begin(), asset.skeleton.end(), [](const Assets::Bone& b) { return b.name == boneName; });
                    //if (it == asset.skeleton.end()) {
                    //    if (no_bone_tracks > 0) {
                    //        PN_CORE_ERROR("Error with loading model. Too many tracks for skeleton size");
                    //        throw std::runtime_error("");
                    //    }
                    //    boneName = "root" + std::to_string(no_bone_tracks++);
                    //}

                    auto& track = anim.track_map[boneName];

                    uint32_t keyCount = 0;
                    readMem(&keyCount, sizeof(keyCount));
                    track.resize(keyCount);

                    for (AnimationKey& key : track) {
                        readMem(&key.time, sizeof(key.time));
                        readMem(&key.translation, sizeof(key.translation));
                        readMem(&key.rotation, sizeof(key.rotation));
                        readMem(&key.scale, sizeof(key.scale));

                        // Morph weights
                        uint32_t morphWeightsCount = 0;
                        readMem(&morphWeightsCount, sizeof(morphWeightsCount));
                        key.morphTargetWeights.resize(morphWeightsCount);
                        readMem(key.morphTargetWeights.data(), morphWeightsCount * sizeof(float));
                    }
                }
            }

            //PN_CORE_TRACE("ImportModel: Before reading materials");

            // Materials
            uint32_t matCount = 0;
            readMem(&matCount, sizeof(matCount));
            asset.materials.resize(matCount);
            for (auto& mat : asset.materials) {
                std::string temp_str;
                readStr(temp_str);
                mat = std::filesystem::path(temp_str);
            }

            asset.vpath = virtual_path;

            return std::make_shared<Model>(std::move(asset));
		}

        bool Loader::CheckShader(GLuint shader, const char* label) const {
            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_TRUE) return true;

            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            std::string log(len ? len - 1 : 0, '\0');
            if (len > 1) glGetShaderInfoLog(shader, len, nullptr, log.data());
#ifdef PN_PLATFORM_ANDROID
            PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
#else
            PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
#endif
            return false;
        }

        bool Loader::CheckProgram(GLuint program) const {
            GLint linked = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linked);
            if (linked == GL_TRUE) return true;

            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            std::string log(len ? len - 1 : 0, '\0');
            if (len > 1) glGetProgramInfoLog(program, len, nullptr, log.data());
#ifdef PN_PLATFORM_ANDROID
            PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
#else
            PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
#endif
            return false;
        }

        uint32_t Loader::CompileShader(unsigned int type, const std::string& source) const
        {
            // Create vert & frag shaders
            uint32_t shader = glCreateShader(type);
            const char* src = source.c_str();

            PN_CORE_INFO("Compiling {} shader", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
            // PN_CORE_INFO("Shader source:\n{0}", source);

            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            // Check compilation
            int success;

            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetShaderInfoLog(shader, 512, nullptr, infoLog);
#ifdef PN_PLATFORM_ANDROID
                PN_CORE_ERROR("Shader compile error {0}: {1}\nSource: {2}",
                    type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog, source);
#else
                PN_CORE_ERROR("Shader Compilation Failed ({0}): {1}", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
#endif
				glDeleteShader(shader);
				return 0;
            }

            return shader;

        }

        uint32_t Loader::LinkProgram(unsigned int vert_shader, unsigned int frag_shader) const
        {
			if (vert_shader == 0 || frag_shader == 0) {
				PN_CORE_ERROR("Program link skipped due to invalid shader handles. vert={}, frag={}", vert_shader, frag_shader);
				return 0;
			}

            GLuint program = glCreateProgram();
            glAttachShader(program, vert_shader);
            glAttachShader(program, frag_shader);
            glLinkProgram(program);

            GLint numUniforms = 0;
            glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
#ifdef _DEBUG
            PN_CORE_INFO("Linked program {} has {} active uniforms", program, numUniforms);

            for (int i = 0; i < numUniforms; i++) {
                char name[256] = { 0 };
                GLsizei length = 0;
                GLint size = 0;
                GLenum type = 0;
                glGetActiveUniform(program, i, 256, &length, &size, &type, name);
                PN_CORE_INFO("  Uniform {}: '{}'", i, name);
            }
#endif

            if (!CheckProgram(program)) {
#ifdef PN_PLATFORM_ANDROID
                PN_CORE_ERROR("Program link FAILED");
#else
                PN_CORE_ERROR("Program link FAILED");
#endif
                assert("Program link failed");
            }
            else {
                PN_CORE_INFO("Program link succeeded");
            }

            // (Optional but recommended)
            glDetachShader(program, vert_shader);
            glDetachShader(program, frag_shader);

            return program;
        }

        std::shared_ptr<Shader> Loader::ImportShader(std::string const& virtual_vert, std::string const& virtual_frag) const {

            //Get path service
            auto path_service = services->get<Path::Path>();

            //Read data
            PN_CORE_INFO("Using paths: {0}, {1}", virtual_vert, virtual_frag);
            auto vert_stream = path_service->createFileStream(virtual_vert, Path::FileMode::Read);
            std::string vert_code(vert_stream->size(), '\0');
            size_t chunk = vert_stream->read(&vert_code[0], vert_code.size());
            PN_CORE_INFO("Successfully read vertex shader");
            auto frag_stream = path_service->createFileStream(virtual_frag, Path::FileMode::Read);
            std::string frag_code(frag_stream->size(), '\0');
            chunk = frag_stream->read(&frag_code[0], frag_code.size());
            PN_CORE_INFO("Successfully read fragment shader");

            //Compiler shader
            uint32_t vert_shader = CompileShader(GL_VERTEX_SHADER, vert_code);
            uint32_t frag_shader = CompileShader(GL_FRAGMENT_SHADER, frag_code);

            //Link program
            std::shared_ptr<Shader> shader = std::make_shared<Shader>(LinkProgram(vert_shader, frag_shader));

            // Clean up shaders (they're linked now)
            glDeleteShader(vert_shader);
            glDeleteShader(frag_shader);

            return shader;
        }

        std::shared_ptr<Fonts::FontFace> Loader::ImportFont(std::string const& virtual_path) const {
            return std::make_shared<Fonts::FontFace>(path_service, virtual_path);
        }

        std::shared_ptr<Material> Loader::ImportMaterial(std::string const& virtual_path) const {

            //Check if virtual path has valid extension
            if (std::filesystem::path(path_service->resolvePath(virtual_path)).extension() != ".material") {
                throw std::runtime_error("Invalid file type: " + virtual_path);
            }

            try {
                auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
                if (!stream || !stream->good()) {
                    PN_CORE_ERROR("Failed to open model file: {}", virtual_path);
                    throw std::runtime_error("Failed to open model file: " + virtual_path);
                }

                std::vector<uint8_t> data(stream->size());
                size_t read = stream->read(data.data(), data.size());
                if (read != data.size()) {
                    PN_CORE_ERROR("Failed to read full model file: {}", virtual_path);
                    throw std::runtime_error("Failed to read full model file: " + virtual_path);
                }

                std::string jsonString(data.begin(), data.end());
                nlohmann::json j = nlohmann::json::parse(jsonString);

                Material material;

                if (j.contains("asset")) {
                    material.guid = Assets::GUID(j["asset"]["guid"]);
                    material.name = j["asset"]["name"];
                    material.type = Assets::Type::Material;
                }

                auto LoadTexture = [&](const char* key, std::filesystem::path& path) {
                    if (j.contains("textures") && j["textures"].contains(key)) {
                        path = j["textures"][key].get<std::string>();
                    }
                    };

                // PBR
                LoadTexture("albedo", material.albedoTexturePath);
                LoadTexture("normal", material.normalTexturePath);
                LoadTexture("metallic", material.metallicTexturePath);
                LoadTexture("roughness", material.roughnessTexturePath);
                LoadTexture("ao", material.aoTexturePath);
                LoadTexture("emissive", material.emissiveTexturePath);
                LoadTexture("height", material.heightTexturePath);
                LoadTexture("opacity", material.opacityTexturePath);

                // Advanced
                LoadTexture("sheen", material.sheenTexturePath);
                LoadTexture("clearCoat", material.clearCoatTexturePath);
                LoadTexture("transmission", material.transmissionTexturePath);

                // Legacy
                LoadTexture("specular", material.specularTexturePath);
                LoadTexture("glossiness", material.glossinessTexturePath);
                LoadTexture("ambient", material.ambientTexturePath);

                // Special
                LoadTexture("lightmap", material.lightmapTexturePath);
                LoadTexture("reflection", material.reflectionTexturePath);
                LoadTexture("displacement", material.displacementTexturePath);

                if (j.contains("properties")) {
                    auto& props = j["properties"];

                    if (props.contains("baseColor")) {
                        material.baseColor = glm::vec3(
                            props["baseColor"][0],
                            props["baseColor"][1],
                            props["baseColor"][2]
                        );
                    }

                    if (props.contains("metallic")) {
                        material.metallic = props["metallic"];
                    }

                    if (props.contains("roughness")) {
                        material.roughness = props["roughness"];
                    }

                    if (props.contains("emissive")) {
                        material.emissive = glm::vec3(
                            props["emissive"][0],
                            props["emissive"][1],
                            props["emissive"][2]
                        );
                    }
                }

                if (j.contains("flags")) {
                    material.isTransparent = j["flags"]["transparent"];
                    material.doubleSided = j["flags"]["doubleSided"];
                }

                PN_CORE_INFO("Material loaded: {}", virtual_path);
                return std::make_shared<Material>(std::move(material));

            }
            catch (const std::exception& e) {
                PN_CORE_ERROR("Failed to load material: {}", e.what());
                return nullptr;
            }
        }

        std::shared_ptr<Scene::SceneAsset> Loader::ImportScene(std::string const& virtual_path) const {
            PN_CORE_INFO("[AssetLoader] Loading scene: {}", virtual_path);

            // Check if virtual path has valid extension
            std::filesystem::path resolved = path_service->resolvePath(virtual_path);
            if (resolved.extension() != ".scn") {
                PN_CORE_ERROR("[AssetLoader] Invalid file type: {}", virtual_path);
                throw std::runtime_error("Invalid file type, expected .scn: " + virtual_path);
            }

            // Open file stream
            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good()) {
                PN_CORE_ERROR("[AssetLoader] Failed to open scene file: {}", virtual_path);
                throw std::runtime_error("Failed to open scene file: " + virtual_path);
            }

            // Read entire file
            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size()) {
                PN_CORE_ERROR("[AssetLoader] Failed to read full scene file: {}", virtual_path);
                throw std::runtime_error("Failed to read full scene file: " + virtual_path);
            }

            // Parse JSON
            nlohmann::json sceneJson;
            try {
                std::string jsonString(data.begin(), data.end());
                sceneJson = nlohmann::json::parse(jsonString);
            }
            catch (const nlohmann::json::exception& e) {
                PN_CORE_ERROR("[AssetLoader] JSON parse error in {}: {}", virtual_path, e.what());
                throw std::runtime_error("Failed to parse scene JSON: " + std::string(e.what()));
            }

            // Create SceneAsset
            auto sceneAsset = std::make_shared<Scene::SceneAsset>();

            // Parse camera settings
            if (sceneJson.contains("camera")) {
                auto& cam = sceneJson["camera"];

                if (cam.contains("active_game_cam")) {
                    sceneAsset->camera.active_game_cam = cam["active_game_cam"].get<std::string>();
                }

                if (cam.contains("position") && cam["position"].is_array() && cam["position"].size() >= 3) {
                    sceneAsset->camera.position = glm::vec3(
                        cam["position"][0].get<float>(),
                        cam["position"][1].get<float>(),
                        cam["position"][2].get<float>()
                    );
                }

                if (cam.contains("forward") && cam["forward"].is_array() && cam["forward"].size() >= 3) {
                    sceneAsset->camera.forward = glm::vec3(
                        cam["forward"][0].get<float>(),
                        cam["forward"][1].get<float>(),
                        cam["forward"][2].get<float>()
                    );
                }

                if (cam.contains("up") && cam["up"].is_array() && cam["up"].size() >= 3) {
                    sceneAsset->camera.up = glm::vec3(
                        cam["up"][0].get<float>(),
                        cam["up"][1].get<float>(),
                        cam["up"][2].get<float>()
                    );
                }

                if (cam.contains("fov")) {
                    sceneAsset->camera.fov = cam["fov"].get<float>();
                }

                if (cam.contains("nearPlane")) {
                    sceneAsset->camera.nearPlane = cam["nearPlane"].get<float>();
                }

                if (cam.contains("farPlane")) {
                    sceneAsset->camera.farPlane = cam["farPlane"].get<float>();
                }

                if (cam.contains("aspectRatioW")) {
                    sceneAsset->camera.aspectRatioW = cam["aspectRatioW"].get<float>();
                }

                if (cam.contains("aspectRatioH")) {
                    sceneAsset->camera.aspectRatioH = cam["aspectRatioH"].get<float>();
                }

                if (cam.contains("sensitivity")) {
                    sceneAsset->camera.sensitivity = cam["sensitivity"].get<float>();
                }

                if (cam.contains("speed")) {
                    sceneAsset->camera.speed = cam["speed"].get<float>();
                }
                
                // Parse camera collision settings
                if (cam.contains("collisionEnabled")) {
                    sceneAsset->camera.collisionEnabled = cam["collisionEnabled"].get<bool>();
                }
                if (cam.contains("collisionRadius")) {
                    sceneAsset->camera.collisionRadius = cam["collisionRadius"].get<float>();
                }
                if (cam.contains("collisionOffset")) {
                    sceneAsset->camera.collisionOffset = cam["collisionOffset"].get<float>();
                }
                if (cam.contains("capsuleHeight")) {
                    sceneAsset->camera.capsuleHeight = cam["capsuleHeight"].get<float>();
                }
                if (cam.contains("useCapsuleCollision")) {
                    sceneAsset->camera.useCapsuleCollision = cam["useCapsuleCollision"].get<bool>();
                }
                if (cam.contains("showCollisionGizmo")) {
                    sceneAsset->camera.showCollisionGizmo = cam["showCollisionGizmo"].get<bool>();
                }

                // Parse camera bookmarks
                if (auto bmIt = cam.find("bookmarks"); bmIt != cam.end() && bmIt->is_array()) {
                    int i = 0;
                    for (const auto& bm : *bmIt) {
                        if (i >= 5) break;
                        auto& b = sceneAsset->cameraBookmarks[i++];
                        b.occupied = bm.value("occupied", false);
                        b.pos = { bm["pos"][0],     bm["pos"][1],     bm["pos"][2] };
                        b.forward = { bm["forward"][0], bm["forward"][1], bm["forward"][2] };
                        b.up = { bm["up"][0],      bm["up"][1],      bm["up"][2] };
                    }
                }
            }

            // Parse environment settings
            if (sceneJson.contains("environment")) {
                auto& env = sceneJson["environment"];

                if (env.contains("skyboxGUID")) {
                    sceneAsset->environment.skyboxGUID = Assets::GUID(env["skyboxGUID"].get<std::string>());
                }

                if (env.contains("cameraLightIntensity") && env["cameraLightIntensity"].is_array() && env["cameraLightIntensity"].size() >= 3) {
                    sceneAsset->environment.cameraLightIntensity = glm::vec3(
                        env["cameraLightIntensity"][0].get<float>(),
                        env["cameraLightIntensity"][1].get<float>(),
                        env["cameraLightIntensity"][2].get<float>()
                    );
                }

                if (env.contains("worldLightIntensity") && env["worldLightIntensity"].is_array() && env["worldLightIntensity"].size() >= 3) {
                    sceneAsset->environment.worldLightIntensity = glm::vec3(
                        env["worldLightIntensity"][0].get<float>(),
                        env["worldLightIntensity"][1].get<float>(),
                        env["worldLightIntensity"][2].get<float>()
                    );
                }

                if (env.contains("useWorldLight")) {
                    sceneAsset->environment.useWorldLight = env["useWorldLight"].get<bool>();
                }
                
                // World light shadow settings
                if (env.contains("worldLightDirection") && env["worldLightDirection"].is_array() && env["worldLightDirection"].size() >= 3) {
                    sceneAsset->environment.worldLightDirection = glm::normalize(glm::vec3(
                        env["worldLightDirection"][0].get<float>(),
                        env["worldLightDirection"][1].get<float>(),
                        env["worldLightDirection"][2].get<float>()
                    ));
                }
                if (env.contains("worldLightPosition") && env["worldLightPosition"].is_array() && env["worldLightPosition"].size() >= 3) {
                    sceneAsset->environment.worldLightPosition = glm::vec3(
                        env["worldLightPosition"][0].get<float>(),
                        env["worldLightPosition"][1].get<float>(),
                        env["worldLightPosition"][2].get<float>()
                    );
                }
                if (env.contains("worldLightShadowFollowDistance")) {
                    sceneAsset->environment.worldLightShadowFollowDistance = env["worldLightShadowFollowDistance"].get<float>();
                }
                if (env.contains("worldLightShadowResolution")) {
                    sceneAsset->environment.worldLightShadowResolution = env["worldLightShadowResolution"].get<int>();
                }
                if (env.contains("worldLightFarPlane")) {
                    sceneAsset->environment.worldLightFarPlane = env["worldLightFarPlane"].get<float>();
                }
                if (env.contains("worldLightShadowsEnabled")) {
                    sceneAsset->environment.worldLightShadowsEnabled = env["worldLightShadowsEnabled"].get<bool>();
                }
                
                if (env.contains("useIBL")) {
                    sceneAsset->environment.useIBL = env["useIBL"].get<bool>();
                }
                if (env.contains("useDiffuseMap")) {
                    sceneAsset->environment.useDiffuseMap = env["useDiffuseMap"].get<bool>();
                }
                if (env.contains("useAOMap")) {
                    sceneAsset->environment.useAOMap = env["useAOMap"].get<bool>();
                }
                if (env.contains("useNormalMap")) {
                    sceneAsset->environment.useNormalMap = env["useNormalMap"].get<bool>();
                }
                if (env.contains("useRoughnessMetallicMap")) {
                    sceneAsset->environment.useRoughnessMetallicMap = env["useRoughnessMetallicMap"].get<bool>();
                }
                if (env.contains("useEmissionMap")) {
                    sceneAsset->environment.useEmissionMap = env["useEmissionMap"].get<bool>();
                }
                if (env.contains("pbr_map")) {
                    sceneAsset->environment.pbr_map = static_cast<GraphicsSettings::DEBUG_PBR_MAP_TYPES>(env["pbr_map"].get<int>());
                }
            }

            // Parse loading screen settings
            if (sceneJson.contains("loadingScreen")) {
                auto& ls = sceneJson["loadingScreen"];

                if (ls.contains("backgroundTextureGUID")) {
                    sceneAsset->loadingScreen.backgroundTextureGUID = Assets::GUID(ls["backgroundTextureGUID"].get<std::string>());
                }
                else {
                    //Set default digipen screen for texture rendering
#ifdef PN_PLATFORM_WINDOWS
                    std::filesystem::path tex_path = "engine/textures/DigiPen_BLACK.png";
#else
                    std::filesystem::path tex_path = "engine\\textures\\DigiPen_BLACK.png";
#endif
                    sceneAsset->loadingScreen.backgroundTextureGUID = services->get<Assets::Manager>()->findGUID(tex_path);
                }
                if (ls.contains("backgroundColor") && ls["backgroundColor"].is_array() && ls["backgroundColor"].size() >= 3) {
                    sceneAsset->loadingScreen.backgroundColor = glm::vec3(
                        ls["backgroundColor"][0].get<float>(),
                        ls["backgroundColor"][1].get<float>(),
                        ls["backgroundColor"][2].get<float>()
                    );
                }
                if (ls.contains("bgScale")) {
                    sceneAsset->loadingScreen.bgScale = ls["bgScale"].get<float>();
                }
                if (ls.contains("showBackground")) {
                    sceneAsset->loadingScreen.showBackground = ls["showBackground"].get<bool>();
                }
                if (ls.contains("showOverlay")) {
                    sceneAsset->loadingScreen.showOverlay = ls["showOverlay"].get<bool>();
                }

                if (ls.contains("progressBarPosition") && ls["progressBarPosition"].is_array() && ls["progressBarPosition"].size() >= 2) {
                    sceneAsset->loadingScreen.progressBarPosition = glm::vec2(
                        ls["progressBarPosition"][0].get<float>(),
                        ls["progressBarPosition"][1].get<float>()
                    );
                }
                else {
                    auto win = services->get<Window::Window>();
                    if (win) {
                        auto framebuffer = win->getFrameBuffer();
                        float screenWidth = framebuffer.x;
                        float screenHeight = framebuffer.y;

                        // Set default progress bar position
                        sceneAsset->loadingScreen.progressBarPosition = glm::vec2(screenWidth / 2.0f, screenHeight * 0.15f);
                    }
                }
                if (ls.contains("progressBarSize") && ls["progressBarSize"].is_array() && ls["progressBarSize"].size() >= 2) {
                    sceneAsset->loadingScreen.progressBarSize = glm::vec2(
                        ls["progressBarSize"][0].get<float>(),
                        ls["progressBarSize"][1].get<float>()
                    );
                }
                else {
                    auto win = services->get<Window::Window>();
                    if (win) {
                        auto framebuffer = win->getFrameBuffer();
                        float screenWidth = framebuffer.x;
                        float screenHeight = framebuffer.y;

                        // Set default progress bar size
                        sceneAsset->loadingScreen.progressBarSize = glm::vec2(screenWidth * 0.6f, 40.0f);
                    }
                }
                if (ls.contains("fillColor") && ls["fillColor"].is_array() && ls["fillColor"].size() >= 3) {
                    sceneAsset->loadingScreen.fillColor = glm::vec3(
                        ls["fillColor"][0].get<float>(),
                        ls["fillColor"][1].get<float>(),
                        ls["fillColor"][2].get<float>()
                    );
                }
                if (ls.contains("glowColor") && ls["glowColor"].is_array() && ls["glowColor"].size() >= 3) {
                    sceneAsset->loadingScreen.glowColor = glm::vec3(
                        ls["glowColor"][0].get<float>(),
                        ls["glowColor"][1].get<float>(),
                        ls["glowColor"][2].get<float>()
                    );
                }
                if (ls.contains("glowIntensity")) {
                    sceneAsset->loadingScreen.glowIntensity = ls["glowIntensity"].get<float>();
                }
                if (ls.contains("showProgressBar")) {
                    sceneAsset->loadingScreen.showProgressBar = ls["showProgressBar"].get<bool>();
                }

                if (ls.contains("statusTextPosition") && ls["statusTextPosition"].is_array() && ls["statusTextPosition"].size() >= 2) {
                    sceneAsset->loadingScreen.statusTextPosition = glm::vec2(
                        ls["statusTextPosition"][0].get<float>(),
                        ls["statusTextPosition"][1].get<float>()
                    );
                }
                else {
                    auto win = services->get<Window::Window>();
                    if (win) {
                        auto framebuffer = win->getFrameBuffer();
                        float screenWidth = framebuffer.x;
                        float screenHeight = framebuffer.y;

                        // Set default status text position (if not already set by user)
                        sceneAsset->loadingScreen.statusTextPosition = glm::vec2(screenWidth / 2.0f, sceneAsset->loadingScreen.progressBarPosition.y - 70.0f);
                    }
                }
                if (ls.contains("statusTextScale")) {
                    sceneAsset->loadingScreen.statusTextScale = ls["statusTextScale"].get<float>();
                }
                if (ls.contains("showStatusText")) {
                    sceneAsset->loadingScreen.showStatusText = ls["showStatusText"].get<bool>();
                }
                if (ls.contains("frameCount")) {
                    sceneAsset->loadingScreen.frameCount = ls["frameCount"].get<int>();
                }
                if (ls.contains("framesPerRow")) {
                    sceneAsset->loadingScreen.framesPerRow = ls["framesPerRow"].get<int>();
                }
                if (ls.contains("frameTime")) {
                    sceneAsset->loadingScreen.frameTime = ls["frameTime"].get<float>();
                }
                if (ls.contains("animationEnabled")) {
                    sceneAsset->loadingScreen.animationEnabled = ls["animationEnabled"].get<bool>();
                }
            }
            else {
                //Set default digipen screen for texture rendering
#ifdef PN_PLATFORM_WINDOWS
                std::filesystem::path tex_path = "engine/textures/DigiPen_BLACK.png";
#else
                std::filesystem::path tex_path = "engine\\textures\\DigiPen_BLACK.png";
#endif
                sceneAsset->loadingScreen.backgroundTextureGUID = services->get<Assets::Manager>()->findGUID(tex_path);

                //Setup other variables
                auto win = services->get<Window::Window>();
                if (win) {
                    auto framebuffer = win->getFrameBuffer();
                    float screenWidth = framebuffer.x;
                    float screenHeight = framebuffer.y;

                    // Set default progress bar position and size (if not already set by user)
                    sceneAsset->loadingScreen.progressBarPosition = glm::vec2(screenWidth / 2.0f, screenHeight * 0.15f);
                    sceneAsset->loadingScreen.progressBarSize = glm::vec2(screenWidth * 0.6f, 40.0f);

                    // Set default status text position (if not already set by user)
                    sceneAsset->loadingScreen.statusTextPosition = glm::vec2(screenWidth / 2.0f, sceneAsset->loadingScreen.progressBarPosition.y - 70.0f);
                }
            }

            // Parse minimap settings
            if (sceneJson.contains("minimap")) {
                auto& minimap = sceneJson["minimap"];

                if (minimap.contains("enabled")) {
                    sceneAsset->minimap.enabled = minimap["enabled"].get<bool>();
                }
                if (minimap.contains("radius")) {
                    sceneAsset->minimap.radius = minimap["radius"].get<float>();
                }
                if (minimap.contains("size_px") && minimap["size_px"].is_array() && minimap["size_px"].size() >= 2) {
                    sceneAsset->minimap.size_px = glm::vec2(
                        minimap["size_px"][0].get<float>(),
                        minimap["size_px"][1].get<float>()
                    );
                }
                if (minimap.contains("pos_px") && minimap["pos_px"].is_array() && minimap["pos_px"].size() >= 2) {
                    sceneAsset->minimap.pos_px = glm::vec2(
                        minimap["pos_px"][0].get<float>(),
                        minimap["pos_px"][1].get<float>()
                    );
                }
                if (minimap.contains("override_position")) {
                    sceneAsset->minimap.override_position = minimap["override_position"].get<bool>();
                }
                if (minimap.contains("recommended_position")) {
                    sceneAsset->minimap.recommended_position =
                        static_cast<GraphicsSettings::MINIMAP_RECOMMENDED_POSITION>(minimap["recommended_position"].get<int>());
                }
                if (minimap.contains("shape")) {
                    sceneAsset->minimap.shape =
                        static_cast<GraphicsSettings::MINIMAP_SHAPE>(minimap["shape"].get<int>());
                }
                if (minimap.contains("rotate_with_player")) {
                    sceneAsset->minimap.rotate_with_player = minimap["rotate_with_player"].get<bool>();
                }
                if (minimap.contains("show_player")) {
                    sceneAsset->minimap.show_player = minimap["show_player"].get<bool>();
                }
                if (minimap.contains("show_danger")) {
                    sceneAsset->minimap.show_danger = minimap["show_danger"].get<bool>();
                }
                if (minimap.contains("show_items")) {
                    sceneAsset->minimap.show_items = minimap["show_items"].get<bool>();
                }
                if (minimap.contains("show_objective")) {
                    sceneAsset->minimap.show_objective = minimap["show_objective"].get<bool>();
                }
                if (minimap.contains("show_walls")) {
                    sceneAsset->minimap.show_walls = minimap["show_walls"].get<bool>();
                }
                if (minimap.contains("show_route")) {
                    sceneAsset->minimap.show_route = minimap["show_route"].get<bool>();
                }
                if (minimap.contains("route_mode")) {
                    sceneAsset->minimap.route_mode =
                        static_cast<GraphicsSettings::MINIMAP_ROUTE_MODE>(minimap["route_mode"].get<int>());
                }
                if (minimap.contains("use_icon_textures")) {
                    sceneAsset->minimap.use_icon_textures = minimap["use_icon_textures"].get<bool>();
                }
                if (minimap.contains("icon_scale")) {
                    sceneAsset->minimap.icon_scale = minimap["icon_scale"].get<float>();
                }
                if (minimap.contains("show_legend")) {
                    sceneAsset->minimap.show_legend = minimap["show_legend"].get<bool>();
                }
                if (minimap.contains("icon_player_guid")) {
                    sceneAsset->minimap.icon_player_guid = Assets::GUID(minimap["icon_player_guid"].get<std::string>());
                }
                if (minimap.contains("icon_danger_guid")) {
                    sceneAsset->minimap.icon_danger_guid = Assets::GUID(minimap["icon_danger_guid"].get<std::string>());
                }
                if (minimap.contains("icon_item_guid")) {
                    sceneAsset->minimap.icon_item_guid = Assets::GUID(minimap["icon_item_guid"].get<std::string>());
                }
                if (minimap.contains("icon_objective_guid")) {
                    sceneAsset->minimap.icon_objective_guid = Assets::GUID(minimap["icon_objective_guid"].get<std::string>());
                }
                if (minimap.contains("icon_wall_guid")) {
                    sceneAsset->minimap.icon_wall_guid = Assets::GUID(minimap["icon_wall_guid"].get<std::string>());
                }
                if (minimap.contains("background_alpha")) {
                    sceneAsset->minimap.background_alpha = minimap["background_alpha"].get<float>();
                }
                if (minimap.contains("border_thickness")) {
                    sceneAsset->minimap.border_thickness = minimap["border_thickness"].get<float>();
                }
                if (minimap.contains("border_color") && minimap["border_color"].is_array() && minimap["border_color"].size() >= 4) {
                    sceneAsset->minimap.border_color = glm::vec4(
                        minimap["border_color"][0].get<float>(),
                        minimap["border_color"][1].get<float>(),
                        minimap["border_color"][2].get<float>(),
                        minimap["border_color"][3].get<float>()
                    );
                }
                if (minimap.contains("camera_height")) {
                    sceneAsset->minimap.camera_height = minimap["camera_height"].get<float>();
                }
            }

            // Parse layers
            if (sceneJson.contains("layers") && sceneJson["layers"].is_array()) {
                sceneAsset->layers.clear();
                for (auto& layerJson : sceneJson["layers"]) {
                    Scene::Layer layer;
                    if (layerJson.contains("id")) layer.id = layerJson["id"].get<int>();
                    if (layerJson.contains("mask")) layer.mask = layerJson["mask"].get<int>();
                    if (layerJson.contains("enabled")) layer.enabled = layerJson["enabled"].get<bool>();
                    if (layerJson.contains("pickable")) layer.pickable = layerJson["pickable"].get<bool>();
                    if (layerJson.contains("name")) layer.name = layerJson["name"].get<std::string>();
                    if (layerJson.contains("color") && layerJson["color"].is_array() && layerJson["color"].size() >= 3) {
                        layer.color = glm::vec3(
                            layerJson["color"][0].get<float>(),
                            layerJson["color"][1].get<float>(),
                            layerJson["color"][2].get<float>()
                        );
                    }
                    sceneAsset->layers.push_back(layer);
                }
            }

            // Parse mask matrix
            if (sceneJson.contains("mask_matrix") && sceneJson["mask_matrix"].is_array()) {
                sceneAsset->mask_matrix.clear();
                for (auto& row : sceneJson["mask_matrix"]) {
                    std::vector<bool> matrixRow;
                    for (auto& val : row) {
                        matrixRow.push_back(val.get<bool>());
                    }
                    sceneAsset->mask_matrix.push_back(matrixRow);
                }
            }

            // Parse assets
            if (sceneJson.contains("assets") && sceneJson["assets"].is_array()) {
                sceneAsset->assets_to_cache = sceneJson["assets"].get<std::unordered_set<Assets::GUID>>();
            }

            // Store entity data as-is (will be parsed by SceneManager)
            if (sceneJson.contains("ecs")) {
                sceneAsset->entityData = sceneJson["ecs"];
            }

            PN_CORE_INFO("[AssetLoader] Successfully loaded scene: {}", virtual_path);
            PN_CORE_INFO("[AssetLoader] Camera pos: ({}, {}, {})",
                sceneAsset->camera.position.x,
                sceneAsset->camera.position.y,
                sceneAsset->camera.position.z);

            if (!sceneAsset->entityData.empty()) {
                int entityCount = sceneAsset->entityData.contains("Entities") ?
                    sceneAsset->entityData["Entities"].size() : 0;
                PN_CORE_INFO("[AssetLoader] Scene contains {} entities", entityCount);
            }

            return sceneAsset;
        }
	}
}
