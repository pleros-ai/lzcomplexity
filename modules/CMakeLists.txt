# Add Core library for compile
if(LZ_ONLY_CORE)
   add_subdirectory(core)
else()
   add_subdirectory(lz)
   add_subdirectory(distance)
endif()