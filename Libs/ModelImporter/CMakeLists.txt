

# Generate as a static library 	
ADD_LIBRARY(ModelImporter STATIC "")

# ================ compiler ===========================

TARGET_COMPILE_OPTIONS(ModelImporter PRIVATE ${OMEGA_CXX_FLAGS})

# =============== source files =============================

TARGET_INCLUDE_DIRECTORIES(ModelImporter PUBLIC
    ${OELIBRARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/Formats
	${EXTERNAL_DIR}
	${OMEGA_ROOT}
)

TARGET_SOURCES(ModelImporter PRIVATE
	
	AnimInstance.cpp AnimInstance.h
	MaterialInstance.cpp MaterialInstance.h 
	MeshInstance.cpp MeshInstance.h
	SkinInstance.cpp SkinInstance.h
	NodeInstance.cpp NodeInstance.h

	Formats/GltfModel.cpp Formats/GltfModel.h
	Formats/AssimpModel.cpp Formats/AssimpModel.h

)

TARGET_LINK_LIBRARIES(ModelImporter PRIVATE
    OEMATHS
    ${ASSIMP_LIBRARY}
)

# ================= tests ===========================
