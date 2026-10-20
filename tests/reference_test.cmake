execute_process(COMMAND ${FLUID} --course-reference ${STEPS} ${INPUT} ${OUTPUT} RESULT_VARIABLE exit_code)
if(NOT exit_code EQUAL 0)
  message(FATAL_ERROR "fluid exited with code ${exit_code}")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUTPUT} ${EXPECTED} RESULT_VARIABLE different)
if(different)
  message(FATAL_ERROR "${OUTPUT} is not identical to ${EXPECTED}")
endif()
