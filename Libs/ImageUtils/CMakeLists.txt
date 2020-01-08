

# Generate as a static library 	
ADD_LIBRARY(IMAGE STATIC "")

# ================ compiler ===========================

TARGET_COMPILE_OPTIONS(IMAGE PRIVATE ${OMEGA_CXX_FLAGS})

# =============== source files =============================

TARGET_SOURCES(IMAGE PRIVATE
	
	KtxParser.cpp KtxParser.h
	ImageLoader.cpp ImageLoader.h
	Image2D.cpp Image2D.h
	CubeImage.cpp CubeImage.h
    	IblUtils.h

)


# ================= tests ===========================
