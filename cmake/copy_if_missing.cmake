if(NOT DEFINED src OR NOT DEFINED dst)
  message(FATAL_ERROR "copy_if_missing.cmake requires src and dst")
endif()

if(NOT EXISTS "${dst}")
  file(COPY_FILE "${src}" "${dst}")
endif()
