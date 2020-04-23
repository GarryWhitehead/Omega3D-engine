

# Generate as a static library 	
ADD_LIBRARY(UTILITY STATIC "")

# ================ compiler ===========================

TARGET_COMPILE_OPTIONS(UTILITY PRIVATE ${OMEGA_CXX_FLAGS})

# =============== source files =============================

TARGET_INCLUDE_DIRECTORIES(UTILITY PRIVATE
    ${OELIBRARY_DIR}
)

TARGET_SOURCES(UTILITY PRIVATE
	
	MurmurHash.h
	EventManager.cpp EventManager.h
	BVH.cpp BVH.hpp
	FileUtil.cpp FileUtil.h
	GeneralUtil.cpp GeneralUtil.h
	Logger.h
	RandomNumber.cpp RandomNumber.h
	result.h
	CString.cpp CString.h
	Timer.h
    BitSetEnum.h
	AlignedAlloc.h
	Compiler.h
)

TARGET_LINK_LIBRARIES(UTILITY PRIVATE
	OEMATHS
)


# ================= tests ===========================
