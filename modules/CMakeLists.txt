# Add Core library for compile
if(LZ_ONLY_CORE)
   add_subdirectory(core)
else()
   add_subdirectory(lz)

   if(LZ_DISTANCE)
      add_subdirectory(distance)
   endif()
endif()