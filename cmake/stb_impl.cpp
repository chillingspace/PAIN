// cmake/stb_impl.cpp
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION  // This was missing!
#define STB_IMAGE_WRITE_IMPLEMENTATION   // Optional: for writing images

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"
