INCLUDE(TribitsPackageMacros)
INCLUDE(TribitsAddOptionAndDefine)
INCLUDE(CombinedOption)
 
#
# A) Define the package
#

TRIBITS_PACKAGE(MueLu)

#
# B) Set up package-specific options
#

TRIBITS_ADD_EXPLICIT_INSTANTIATION_OPTION()
TRIBITS_ADD_DEBUG_OPTION()
TRIBITS_ADD_SHOW_DEPRECATED_WARNINGS_OPTION()

# Profiling
TRIBITS_ADD_OPTION_AND_DEFINE(${PACKAGE_NAME}_ENABLE_PROFILING
  HAVE_${PACKAGE_NAME_UC}_PROFILING
  "Enable detailed timing and memory profiling of MueLu."
  OFF)

# Profiling (expert users only. There is no TPL for Google Perftools)
TRIBITS_ADD_OPTION_AND_DEFINE(${PACKAGE_NAME}_ENABLE_GOOGLE_PERFTOOLS
  HAVE_${PACKAGE_NAME_UC}_GOOGLE_PERFTOOLS
  "Enable detailed memory profiling using Google Perftools."
  OFF)

# Explicit instantiation options:
# - MueLu_ENABLE_EXPLICIT_INSTANTIATION
# - MueLu_INST_DOUBLE_INT_INT         (= ON by default)
# - MueLu_INST_DOUBLE_INT_LONGLONGINT (= ON by default if TEUCHOS_ENABLE_LONG_LONG_INT enabled) TODO: do not work if Tpetra_ENABLE_EXPLICIT_INSTANTIATION=ON
# - MueLu_INST_COMPLEX_INT_INT        (= ON by default if TEUCHOS_ENABLE_COMPLEX enabled)       TODO: we also need to check Tpetra_COMPLEX if EI of Tpetra=ON
IF(${PACKAGE_NAME}_ENABLE_EXPLICIT_INSTANTIATION)

  # <double, int, int>
  # Examples and tests need at least this one
  TRIBITS_ADD_OPTION_AND_DEFINE(
    ${PACKAGE_NAME}_INST_DOUBLE_INT_INT
    HAVE_${PACKAGE_NAME_UC}_INST_DOUBLE_INT_INT
    "Enable the use of explicit template instantiation with <double, int, int>."
    ON
    )
  IF(${PACKAGE_NAME}_ENABLE_EXAMPLES OR ${PACKAGE_NAME}_ENABLE_TESTS) 
    IF (NOT ${PACKAGE_NAME}_INST_DOUBLE_INT_INT)
      MESSAGE(FATAL_ERROR "Error: MueLu examples and tests needs explicit template instantation with <double, int, int>. Turn on the option MueLu_INST_DOUBLE_INT_INT or disable examples and tests.")
    ENDIF()
  ENDIF()

  # <double, int, long long int>
  IF(Teuchos_ENABLE_LONG_LONG_INT)
    SET(${PACKAGE_NAME}_INST_DOUBLE_INT_LONGLONGINT_DEFAULT ON)
  ELSE()
    SET(${PACKAGE_NAME}_INST_DOUBLE_INT_LONGLONGINT_DEFAULT OFF)
  ENDIF()
  TRIBITS_ADD_OPTION_AND_DEFINE(
    ${PACKAGE_NAME}_INST_DOUBLE_INT_LONGLONGINT
    HAVE_${PACKAGE_NAME_UC}_INST_DOUBLE_INT_LONGLONGINT
    "Enable the use of explicit template instantiation with <double, int, long long int>."
    ${${PACKAGE_NAME}_INST_DOUBLE_INT_LONGLONGINT_DEFAULT}
    )

  # <complex, int, int>
# Teuchos_ENABLE_COMPLEX is ON by default (in release mode or when SS is enabled) but we don't want MueLu to be instantiated explicitly with complex by default as it is not frequently used.
#  IF(Teuchos_ENABLE_COMPLEX)
#    SET(${PACKAGE_NAME}_INST_COMPLEX_INT_INT_DEFAULT ON)
#  ELSE()
    SET(${PACKAGE_NAME}_INST_COMPLEX_INT_INT_DEFAULT OFF)
#  ENDIF()
  TRIBITS_ADD_OPTION_AND_DEFINE(
    ${PACKAGE_NAME}_INST_COMPLEX_INT_INT
    HAVE_${PACKAGE_NAME_UC}_INST_COMPLEX_INT_INT
    "Enable the use of explicit template instantiation with <complex, int, int>."
    ${${PACKAGE_NAME}_INST_COMPLEX_INT_INT_DEFAULT}
    )

  # sacado
  IF(${PACKAGE_NAME}_ENABLE_PCE_SUPPORT)
    TRIBITS_ADD_OPTION_AND_DEFINE(
      ${PACKAGE_NAME}_INST_PCE_INT_INT
      HAVE_${PACKAGE_NAME_UC}_INST_PCE_INT_INT
      "Enable the use of explicit template instantiation with Scalar=Sacado orthogonal polynomials and LO=GO=int."
      ON
    )
  ENDIF()

ENDIF()

# HAVE_MUELU_COMPLEX=ON iff:
# - Teuchos_ENABLE_COMPLEX=ON and Explicit Instantiation = OFF
# or
# - HAVE_MUELU_INST_COMPLEX_INT_INT
#
# TODO: do we want also a MueLu_ENABLE_Complex option that turns ON automatically what we need?
IF(((NOT ${PACKAGE_NAME}_ENABLE_EXPLICIT_INSTANTIATION) AND Teuchos_ENABLE_COMPLEX) 
    OR 
    (HAVE_MUELU_INST_COMPLEX_INT_INT))
  SET(HAVE_${PACKAGE_NAME_UC}_COMPLEX ON)
ELSE()
  SET(HAVE_${PACKAGE_NAME_UC}_COMPLEX OFF)
ENDIF()

#
# C) Add the libraries, tests, and examples
#

ADD_SUBDIRECTORY(gallery) # iff EXAMPLE or TEST ENABLED
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(adapters)
ADD_SUBDIRECTORY(doc)

TRIBITS_ADD_EXAMPLE_DIRECTORIES(example)
TRIBITS_ADD_TEST_DIRECTORIES(test)

#
# Exclude files for source package.
#

TRIBITS_EXCLUDE_AUTOTOOLS_FILES()

#
# D) Do standard postprocessing
#

TRIBITS_PACKAGE_POSTPROCESS()