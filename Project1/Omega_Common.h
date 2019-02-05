#pragma once

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

// tiny gltf uses window.h - this stops the macros messing with std::min
#define NOMINMAX
#include "tiny_gltf.h"

// sets whether omega engine should be executed in threaded mode - turning this off is mainly for debugging purposes!
#define OMEGA_ENGINE_THREADED 1

// number of threads allocated to various tasks
#define ASSET_LOAD_THREAD_COUNT 2
#define SCENE_LOAD_THREAD_COUNT 2