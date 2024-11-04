# Add Core library for compile
if(LZ_ONLY_CORE)
   add_subdirectory(core)
else()
   if(LZ_APP)
      add_subdirectory(lz)
   endif()

   if(LZ_DISTANCE)
      add_subdirectory(distance)
   endif()
endif()