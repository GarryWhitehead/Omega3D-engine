

# Generate as a static library 	
ADD_LIBRARY(OETHREAD STATIC "")

# ================ compiler ===========================

TARGET_COMPILE_OPTIONS(OETHREAD PRIVATE ${OMEGA_CXX_FLAGS})

# =============== source files =============================

TARGET_SOURCES(OETHREAD PRIVATE
	
	ThreadedQueue.h
	ThreadPool.cpp ThreadPool.h
	ThreadUtil.cpp ThreadUtil.h

)

# ================= tests ===========================
