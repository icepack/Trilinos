
GET_PROPERTY( PTR_STRING GLOBAL PROPERTY PTR_IMPL )
GET_PROPERTY( PARAMETERLIST_STRING GLOBAL PROPERTY PARAMETERLIST_IMPL )
 
IF( PTR_STRING STREQUAL "Teuchos::RCP" )
IF( PARAMETERLIST_STRING STREQUAL "Teuchos::ParameterList" )

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  example_01
  SOURCES example_01.cpp
  ARGS PrintItAll
  NUM_MPI_PROCS 1
  PASS_REGULAR_EXPRESSION "TEST PASSED"
  ADD_DIR_TO_NAME
)

ENDIF( PARAMETERLIST_STRING STREQUAL "Teuchos::ParameterList" )
ENDIF( PTR_STRING STREQUAL "Teuchos::RCP" )

TRIBITS_ADD_EXECUTABLE_AND_TEST(
  example_02
  SOURCES example_02.cpp
  ARGS PrintItAll
  NUM_MPI_PROCS 1
  PASS_REGULAR_EXPRESSION "TEST PASSED"
  ADD_DIR_TO_NAME
)

TRIBITS_COPY_FILES_TO_BINARY_DIR(
  PoissonControlDataCopy
  SOURCE_FILES
    input.xml
    example_02.xml
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}"
)
