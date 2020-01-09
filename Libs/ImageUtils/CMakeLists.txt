

# Generate as a static library 	
ADD_LIBRARY(IMAGE STATIC "")

# ================ compiler ===========================

TARGET_COMPILE_OPTIONS(IMAGE PRIVATE ${OMEGA_CXX_FLAGS})

# =============== source files =============================

TARGET_INCLUDE_DIRECTORIES(IMAGE PRIVATE
    ${OELIBRARY_DIR}
    ${EXTERNAL_DIR}/stb
)

TARGET_SOURCES(IMAGE PRIVATE
	
	MappedTexture.cpp MappedTexture.h
	KtxParser.cpp KtxParser.h
	ImageLoader.cpp ImageLoader.h
	Image2D.cpp Image2D.h
	CubeImage.cpp CubeImage.h
    IblUtils.h

)

TARGET_LINK_LIBRARIES(IMAGE PRIVATE
    UTILITY
)

# ================= tests ===========================
