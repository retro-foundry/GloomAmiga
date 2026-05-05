if(NOT DEFINED HTML)
  message(FATAL_ERROR "HTML path is required")
endif()

if(NOT DEFINED VERSION)
  message(FATAL_ERROR "VERSION value is required")
endif()

file(READ "${HTML}" html_content)
string(REPLACE "src=\"gloom_pc.js\"" "src=\"gloom_pc.js?v=${VERSION}\"" html_content "${html_content}")
file(WRITE "${HTML}" "${html_content}")
