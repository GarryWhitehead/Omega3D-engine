

# Generate as a static library 	
ADD_LIBRARY(OEMATHS STATIC "")

# ================ compiler ===========================

TARGET_COMPILE_OPTIONS(OEMATHS PRIVATE ${OMEGA_CXX_FLAGS})

# =============== source files =============================

TARGET_SOURCES(OEMATHS PRIVATE
	
	OEMaths.h
	OEMaths_Mat2.h
	OEMaths_Mat3.h
	OEMaths_Mat4.h
    	OEMaths_MatN.h
	OEMaths_Quat.h
    	OEMaths_Vec2.h
	OEMaths_Vec3.h
	OEMaths_Vec4.h
    	OEMaths_VecN.h
	OEMaths_transform.cpp OEMaths_transform.h
)

# ================= tests ===========================
