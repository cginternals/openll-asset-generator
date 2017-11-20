#pragma once

#include <llassetgen/llassetgen_api.h>
#include <llassetgen/llassetgen.h>

#include <vector>
#include <string>

#include <ft2build.h>
#include FT_IMAGE_H

namespace llassetgen {
	template <typename T>
	class Bitmap {
		public:
			LLASSETGEN_API Bitmap(FT_Bitmap &ft_bitmap);
			//Bitmap(std::string filepath);
			LLASSETGEN_API T& operator[](std::size_t index);
			LLASSETGEN_API const T& operator[](std::size_t index) const;
			//void exportPng(std::string filepath);
			//LLASSETGEN_API void print();
		private:
			unsigned int height;
			unsigned int width;
			int pitch;
			std::vector<T> data;
	};
}