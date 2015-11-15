# NTL needs GMP 3.1.1 or newer, this script will fail if an old version is
# detected

find_package( GMP REQUIRED )

find_path(NTL_INCLUDE_DIR
          NAMES NTL/ZZ.h
          HINTS ENV NTL_INC_DIR
                ENV NTL_DIR
          PATHS /export/crypto/linux.x86_64/ntl-9.2.0
          PATH_SUFFIXES include
          DOC "The directory containing the NTL include files"
         )

find_library(NTL_LIBRARY
             NAMES ntl
             HINTS ENV NTL_LIB_DIR
                   ENV NTL_DIR
             PATHS /export/crypto/linux.x86_64/ntl-9.2.0
             PATH_SUFFIXES lib
             DOC "Path to the NTL library"
            )

if ( NTL_INCLUDE_DIR AND NTL_LIBRARY )

   #check version

   set( NTL_VERSION_H "${NTL_INCLUDE_DIR}/NTL/version.h" )

   if ( EXISTS ${NTL_VERSION_H} )

     file( READ "${NTL_VERSION_H}" NTL_VERSION_H_CONTENTS )

     string( REGEX MATCH "[0-9]+(\\.[0-9]+)+" CGAL_NTL_VERSION "${NTL_VERSION_H_CONTENTS}" )


   endif (EXISTS ${NTL_VERSION_H} )

endif ( NTL_INCLUDE_DIR AND NTL_LIBRARY )

if ( NTL_FOUND )

  #message( STATUS "Found NTL in version '${CGAL_NTL_VERSION}'" )
  set ( NTL_INCLUDE_DIRS ${NTL_INCLUDE_DIR} )
  set ( NTL_LIBRARIES ${NTL_LIBRARY} )

  get_filename_component(NTL_LIBRARIES_DIR ${NTL_LIBRARIES} PATH CACHE )

  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args( NTL
                                     DEFAULT_MSG
                                     NTL_LIBRARY
                                     NTL_INCLUDE_DIR )

  mark_as_advanced( NTL_INCLUDE_DIR NTL_LIBRARY )

  # TODO add flag to CGAL Polynomials

endif( NTL_FOUND )

if ( NTL_FOUND )
#  if ( NOT NTL_FIND_QUIETLY )
#    message(STATUS "Found NTL: ${NTL_LIBRARY}")
#  endif (NOT NTL_FIND_QUIETLY )
else ( NTL_FOUND )
  if ( NTL_FIND_REQUIRED )
    message( FATAL_ERROR "Could not find NTL" )
  endif ( NTL_FIND_REQUIRED )
endif ( NTL_FOUND )
