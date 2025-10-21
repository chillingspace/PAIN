#pragma once

#ifdef PN_PLATFORM_WINDOWS

#ifndef LOADER_HPP
#define LOADER_HPP

#include "Applications/AppSystem.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Assets/sAssetCompiler.h"

using namespace glm;

namespace PAIN {
	namespace Loader {

		//Font Type Data Structure
		struct Font {
			struct Character {
				unsigned int texture;
				fvec2 size;			// Size of the character
				fvec2 bearing;		// Offset from the baseline to the top-left of the character
				unsigned int advance;   // Horizontal offset to advance to the next character

				Character() : texture{ 0 }, size(), bearing(), advance{ 0 } {}
				Character(unsigned int texture, fvec2 const& size, fvec2 const& bearing, unsigned int advance)
					: texture{ texture }, size{ size }, bearing{ bearing }, advance{ advance } {
				}
			};

			std::unordered_map<unsigned char, Character> char_map;
		};

		////Abstract font lib interface
		//class IFontLib {
		//private:
		//public:
		//	IFontLib() = default;
		//	virtual ~IFontLib() = default;
		//};


		////Font Service
		//class NIKEFontLib : public IFontLib {
		//private:
		//	//Free type lib
		//	FT_Library ft_lib;

		//	//Generate texture from glyphs for rendering
		//	Font generateGlyphsTex(std::string const& file_path, FT_Face& font_face);

		//public:
		//	//Default constructor
		//	NIKEFontLib();

		//	//Load free type font
		//	Font generateFont(std::string const& file_path, Vector2f const& pixel_sizes = { 0.0f, 48.0f });

		//	//Default destructor
		//	~NIKEFontLib();
		//};

		////Font Loader
		//class FontLoader {
		//private:
		//	std::shared_ptr<IFontLib> font_lib;
		//public:
		//	FontLoader();

		//	//Get font lib
		//	std::shared_ptr<IFontLib> getFontLib() const;
		//};

		//Model data structure
		struct Model {
			unsigned int vaoid;
			unsigned int vboid;
			unsigned int eboid;

			std::vector<unsigned int> indices;
			unsigned int primitive_type;
			unsigned int draw_count;

			std::vector<PAIN::Vertex> vertices;

			Model() : vaoid{ 0 }, vboid{ 0 }, eboid{ 0 }, primitive_type{ 0 }, draw_count{ 0 } {}
		};

		//Texture data structure
		struct Texture {
			unsigned int gl_data;
			ivec3 size;
			// Store texture target (GL_TEXTURE_2D, GL_TEXTURE_3D)
			GLenum target; 
			std::string file_path;

			Texture() : gl_data{ 0 }, size{}, target{ GL_TEXTURE_2D }, file_path{ "" } {}
			Texture(unsigned int gl_data, ivec3 size, GLenum target, std::string file_path)
				: gl_data{ gl_data }, size{ std::move(size) }, target{ target }, file_path{ std::move(file_path) } {
			}
		};

		//Shader/Model/Texture Loader
		class RenderLoader {
		private:
			/**
			 * creates a vertex array object for base opengl shaders.
			 *
			 * \param vertices
			 * \param indices
			 * \param model		vao will be stored here
			 */
			void createBaseBuffers(const std::vector<PAIN::Vertex>& vertices, const std::vector<unsigned int>& indices, Model& model);


			void createBatchedBaseBuffers(Model& model);

			/**
			 * creates a vertex array object for base opengl shaders.
			 *
			 * \param vertices
			 * \param indices
			 * \param model		vao will be stored here
			 */
			void createTextureBuffers(const std::vector<PAIN::Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<PAIN::Vertex>& tex_coords, Model& model);

			void createBatchedTextureBuffers(Model& model);

			// Helper functions to load binary texture files
			bool loadFileToMemory(const std::string& filepath, std::vector<unsigned char>& out_buffer);

			bool loadASTCTexture(const unsigned char* data, size_t data_size, GLuint& out_texID, int& out_width, int& out_height);

			bool loadDDSTexture(const std::string& filepath, GLuint& out_texID, int& out_width, int& out_height, int& out_depth);

			bool loadCompressedTextureFromFile(const std::string& filepath, Texture& outTexture);


		public:
			RenderLoader() = default;
			~RenderLoader() = default;


			/**
			 * all .tex files should be 256x256 in RGBA8 format.
			 *
			 * \param path_to_texture
			 * \param [out] width
			 * \param [out] height
			 * \param [out] tex_size
			 * @param [out] is_tex_ext
			 *
			 * @returns dynamically allocated char*
			 */
			static unsigned char* prepareImageData(const std::string& path_to_texture, int& width, int& height, int& size, bool& is_tex_or_png_ext);

			/**
			 * free images loaded with `prepareImageData`.
			 *
			 * for buffers created with stbi_image_load
			 *
			 * \param data
			 */
			static void freeImageData(unsigned char* data);


			/**
			 * creates vertex array object. from mesh data and registers it to meshes.
			 *
			 * mesh data format: newline separated values for each vertex, each value is a float.
			 * `n` prefix indicates name attribute
			 * `v` prefix indicates vertex attribute
			 * `t` prefix indicates index(triangle) attribute. (indexed rendering with element buffer object)
			 * top of the file indicates vertex count, index count. int format.
			 *
			 * important to note that anticlockwise generated shapes are front facing and vice versa.
			 * back facing triangles will be culled.
			 *
			 * example square mesh:
				n square
				v 0.5 -0.5
				v 0.5 0.5
				v -0.5 0.5
				v -0.5 -0.5
				t 0 1 2
				t 2 3 0
			 *
			 *
			 * \param mesh_ref
			 * \param path_to_mesh
			 * \return success
			 */
			std::shared_ptr<Mesh> loadMesh(const std::string& path_to_mesh);


			Texture loadTexture(const std::string& path_to_texture);
		};
	}
}

#endif
#endif