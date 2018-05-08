
# //////////////////////////////////////////////////////////////////////////
macro(SetupCMake)

	set(CMAKE_CXX_STANDARD 11)

	# this is important for python extension too!
	SET(CMAKE_DEBUG_POSTFIX "_d")
	
	# enable parallel building
	set(CMAKE_NUM_PROCS 8)          

	# use folders to organize projects                           
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)    

	# save libraries and binaries in the same directory        
	set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})           
	set(LIBRARY_OUTPUT_PATH     ${CMAKE_BINARY_DIR})	

	if (WIN32)

		# enable parallel building
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

		# huge file are generated by swig
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

		# see http://msdn.microsoft.com/en-us/library/windows/desktop/ms683219(v=vs.85).aspx
		add_definitions(-DPSAPI_VERSION=1)

		# increse number of file descriptors
		add_definitions(-DFD_SETSIZE=4096)

		# Enable PDB generation for all release build configurations.
		# VC++ compiler and linker options taken from this article:
		# see https://msdn.microsoft.com/en-us/library/fsk896zz.aspx
		set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}   /Zi")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")

		set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
		set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")

	elseif (APPLE)

		set(CMAKE_MACOSX_RPATH 1)

		# force executable to bundle
		set(CMAKE_MACOSX_BUNDLE YES)

		# suppress some warnings
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable -Wno-reorder")

	else ()

		# allow the user to choose between Release and Debug
		if(NOT CMAKE_BUILD_TYPE)
		  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release" FORCE)
		endif()

		if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		  add_definitions(-D_DEBUG=1)
		endif()

		# enable 64 bit file support (see http://learn-from-the-guru.blogspot.it/2008/02/large-file-support-in-linux-for-cc.html)
		add_definitions(-D_FILE_OFFSET_BITS=64)

		# -Wno-attributes to suppress spurious "type attributes ignored after type is already defined" messages see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39159
		set(CMAKE_C_FLAGS    "${CMAKE_C_FLAGS}   -fPIC -Wno-attributes")
		set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC -Wno-attributes")

		# add usual include directories
		include_directories("/usr/local/include")
		include_directories("/usr/include")

	endif()

	find_package(OpenMP)
	if (OpenMP_FOUND)
		set(CMAKE_C_FLAGS          "${CMAKE_C_FLAGS}          ${OpenMP_C_FLAGS}")
		set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        ${OpenMP_CXX_FLAGS}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
	endif()	
	
endmacro()



# //////////////////////////////////////////////////////////////////////////
macro(AddExternalApp name SourceDir BinaryDir extra_args)

	if (WIN32 OR APPLE)
	  set(CMAKE_GENERATOR_ARGUMENT -G"${CMAKE_GENERATOR}")
	else()
	  set(CMAKE_GENERATOR_ARGUMENT -G"\"${CMAKE_GENERATOR}\"")
	endif()

	add_custom_target(${name} 
		COMMAND "${CMAKE_COMMAND}"  "${CMAKE_GENERATOR_ARGUMENT}" -H"${SourceDir}/"  -B"${BinaryDir}/"  -DVisus_DIR="${CMAKE_INSTALL_PREFIX}/lib/cmake/visus" ${extra_args}
		COMMAND "${CMAKE_COMMAND}"  --build "${BinaryDir}/" --config ${CMAKE_BUILD_TYPE})
	set_target_properties(${name} PROPERTIES FOLDER CMakeTargets/)

endmacro()

# ///////////////////////////////////////////////////
macro(AddVisusSwigLibrary Name SwigFile)

	set(NamePy ${Name}Py)

	#prevents rebuild every time make is called
	set_property(SOURCE ${SwigFile} PROPERTY SWIG_MODULE_NAME ${NamePy})

	set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
	set_source_files_properties(${SwigFile} PROPERTIES SWIG_FLAGS  "${SWIG_FLAGS}")

	if (CMAKE_VERSION VERSION_LESS "3.8")
		swig_add_module(${NamePy} python ${SwigFile})
	else()
		swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile})
	endif()

	target_include_directories(_${NamePy} PUBLIC ${PYTHON_INCLUDE_DIRS})

	target_compile_definitions(_${NamePy} PRIVATE NUMPY_FOUND=${NUMPY_FOUND})

	if (NUMPY_FOUND)
		target_include_directories(_${NamePy} PRIVATE ${PYTHON_NUMPY_INCLUDE_DIR})
	endif()

	swig_link_libraries(${NamePy} PUBLIC ${Name})

	# anaconda is statically linking python library inside its executable, so I cannot link in order to avoid duplicated symbols
	# see https://groups.google.com/a/continuum.io/forum/#!topic/anaconda/057P4uNWyCU
	if (APPLE AND PYTHON_EXECUTABLE)
		string(FIND "${PYTHON_EXECUTABLE}" "anaconda" is_anaconda)
		if ("${is_anaconda}" GREATER -1)
			set_target_properties(_${NamePy} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
		else()
			target_link_libraries(_${NamePy} PUBLIC ${PYTHON_LIBRARY})
		endif()
	endif()

	set_target_properties(_${NamePy} PROPERTIES FOLDER Swig/)

	if (NOT WIN32)
		set_target_properties(_${NamePy} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()

endmacro()


# //////////////////////////////////////////////////////////////////////////
macro(AddPythonTest Name FileName WorkingDirectory)

	add_test(NAME ${Name} WORKING_DIRECTORY WorkingDirectory COMMAND $<TARGET_FILE:python> ${FileName})

	if (WIN32 OR APPLE)
		set_tests_properties(${Name} PROPERTIES ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1;PYTHONPATH=${CMAKE_BINARY_DIR}/$<CONFIG>")
	else()
		set_tests_properties(${Name} PROPERTIES ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1;PYTHONPATH=${CMAKE_BINARY_DIR}")
	endif()

endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(FindGitRevision)
	find_program(GIT_CMD git REQUIRED)
	find_package_handle_standard_args(GIT REQUIRED_VARS GIT_CMD)
	execute_process(COMMAND ${GIT_CMD} rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "Current GIT_REVISION ${GIT_REVISION}")
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(FindVCPKGDir)
	set(VCPKG_DIR ${CMAKE_TOOLCHAIN_FILE}/../../../installed/${VCPKG_TARGET_TRIPLET})
	get_filename_component(VCPKG_DIR ${VCPKG_DIR} REALPATH)	
endmacro()

# //////////////////////////////////////////////////////////////////////////
# see https://github.com/Microsoft/vcpkg/issues/1909
macro(FixWin32FindPackageCurl)
	if (WIN32)
		list(LENGTH CURL_LIBRARIES _len_)
		if (_len_ EQUAL 1)
			unset(CURL_LIBRARY CACHE)
			unset(CURL_LIBRARY)
			unset(CURL_LIBRARIES CACHE)
			unset(CURL_LIBRARIES)
			FindVCPKGDir()
			set(CURL_LIBRARIES "debug;${VCPKG_DIR}/debug/lib/libcurl.lib;optimized;${VCPKG_DIR}/lib/libcurl.lib")
			mark_as_advanced(CURL_LIBRARIES)
			add_custom_command(TARGET VisusKernel POST_BUILD 
				COMMAND ${CMAKE_COMMAND} -E copy 
					$<$<CONFIG:Debug>:${VCPKG_DIR}/debug/bin/libcurl-d.dll> 
					$<$<NOT:$<CONFIG:Debug>>:${VCPKG_DIR}/bin/libcurl.dll> 
					$<TARGET_FILE_DIR:VisusKernel>)
		endif()
	endif()			
endmacro()


# //////////////////////////////////////////////////////////////////////////
# see https://github.com/Microsoft/vcpkg/issues/2630
macro(FixWin32FindPackageFreeImage)
	if (WIN32)
		FindVCPKGDir()
		add_custom_command(TARGET VisusKernel POST_BUILD  COMMAND ${CMAKE_COMMAND} -E copy ${VCPKG_DIR}/bin/raw_r.dll $<TARGET_FILE_DIR:VisusKernel>)
		add_custom_command(TARGET VisusKernel POST_BUILD  COMMAND ${CMAKE_COMMAND} -E copy ${VCPKG_DIR}/bin/lcms2.dll $<TARGET_FILE_DIR:VisusKernel>)
	endif()
endmacro()


# /////////////////////////////////////////////////////////////
macro(DisableAllWarnings)

	set(CMAKE_C_WARNING_LEVEL   0)
	set(CMAKE_CXX_WARNING_LEVEL 0)
	
	if(WIN32)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /W0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
	else()
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -w")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	endif()
endmacro()


# ///////////////////////////////////////////////////////////////////////
macro(DisablePythonDebug)

	set(SWIG_PYTHON_INTERPRETER_NO_DEBUG 1)
	target_compile_definitions(VisusKernel PUBLIC SWIG_PYTHON_INTERPRETER_NO_DEBUG=1)

	list(LENGTH PYTHON_LIBRARY _len_)
	if(NOT _len_ EQUAL 1)
	
		# example optimized;C:/Python36/libs/python36.lib;debug;C:/Python36/libs/python36_d.lib
		if (NOT _len_ EQUAL 4)
			message(FATAL_ERROR "Internal error cannot extract debug optimized libraries")
		endif()
		
		list(GET PYTHON_LIBRARY 0 item0)
		list(GET PYTHON_LIBRARY 1 item1)
		list(GET PYTHON_LIBRARY 2 item2)
		list(GET PYTHON_LIBRARY 3 item3)
		
		if ((item0 STREQUAL "optimized") AND (item2 STREQUAL "debug"))
			set(PYTHON_LIBRARY ${item1} CACHE STRING "" FORCE)
		elseif ((item0 STREQUAL "debug") AND (item2 STREQUAL "optimized"))
			set(PYTHON_LIBRARY ${item3} CACHE STRING "" FORCE)
		else()
			message(FATAL_ERROR "Internal error cannot extract debug optimized libraries")
		endif()
		
		set(PYTHON_LIBRARIES ${PYTHON_LIBRARY})
		
		if (DEFINED PYTHON_DEBUG_LIBRARY)
			unset(PYTHON_DEBUG_LIBRARY CACHE)
			unset(PYTHON_DEBUG_LIBRARY)	
			set(PYTHON_DEBUG_LIBRARY ${PYTHON_LIBRARY})
		endif()
		
		if (DEFINED PYTHON_LIBRARY_DEBUG)
			unset(PYTHON_LIBRARY_DEBUG CACHE)
			unset(PYTHON_LIBRARY_DEBUG)		
			set(PYTHON_LIBRARY_DEBUG ${PYTHON_LIBRARY})
		endif()
		
	endif()	
endmacro()


# ///////////////////////////////////////////////////////////////////////
macro(FindZLib)

	if (NOT DEFINED VISUS_INTERNAL_ZLIB)
		find_package(ZLIB)
	endif()

	if (VISUS_INTERNAL_ZLIB OR NOT ZLIB_FOUND)
		set(ZLIB_BUILD_DIR    ${CMAKE_BINARY_DIR}/zlib-1.2.11)
		SET(ZLIB_INCLUDE_DIRS ${ZLIB_BUILD_DIR})
		SET(ZLIB_LIBRARIES    ${ZLIB_BUILD_DIR}/libz.a)  
		if (NOT EXISTS ${ZLIB_LIBRARIES})
			MESSAGE("Downloading and compiling zlib...")
			file(DOWNLOAD https://zlib.net/zlib-1.2.11.tar.gz ${CMAKE_BINARY_DIR}/zlib-1.2.11.tar.gz)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf zlib-1.2.11.tar.gz WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			execute_process(COMMAND cmake . -DCMAKE_POSITION_INDEPENDENT_CODE=1    WORKING_DIRECTORY ${ZLIB_BUILD_DIR})
			execute_process(COMMAND make                                           WORKING_DIRECTORY ${ZLIB_BUILD_DIR})
		endif()
	endif()
endmacro()
  

# ///////////////////////////////////////////////////////////////////////
macro(FindLZ4)

	if (NOT DEFINED VISUS_INTERNAL_LZ4)
		find_package(LZ4)
	endif()

	if (VISUS_INTERNAL_LZ4 OR NOT LZ4_FOUND)
		set(LZ4_BUILD_DIR   ${CMAKE_BINARY_DIR}/lz4-1.8.1.2)
		SET(LZ4_INCLUDE_DIR ${LZ4_BUILD_DIR}/lib)
		SET(LZ4_LIBRARY     ${LZ4_BUILD_DIR}/liblz4.so)  
		if (NOT EXISTS ${LZ4_LIBRARY})
			MESSAGE("Downloading and compiling lz4...")
			file(DOWNLOAD https://github.com/lz4/lz4/archive/v1.8.1.2.tar.gz ${CMAKE_BINARY_DIR}/lz4-1.8.1.2.tar.gz)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf lz4-1.8.1.2.tar.gz WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			execute_process(COMMAND cmake contrib/cmake_unofficial WORKING_DIRECTORY ${LZ4_BUILD_DIR})
			execute_process(COMMAND make                           WORKING_DIRECTORY ${LZ4_BUILD_DIR})
		endif()
	endif()

endmacro()
  

# ///////////////////////////////////////////////////////////////////////
macro(FindTinyXml)

	if (NOT DEFINED VISUS_INTERNAL_TINYXML)
		find_package(TinyXML)
	endif()

	if (VISUS_INTERNAL_TINYXML OR NOT TinyXML_FOUND)
		set(TINYXML_BUILD_DIR    ${CMAKE_BINARY_DIR}/tinyxml)
		SET(TinyXML_INCLUDE_DIRS ${TINYXML_BUILD_DIR})
		SET(TinyXML_LIBRARIES    ${TINYXML_BUILD_DIR}/libtinyxml.a) 
		if (NOT EXISTS ${TinyXML_LIBRARIES})
			MESSAGE("Downloading and compiling tinyxml...")
			file(DOWNLOAD https://downloads.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.zip ${CMAKE_BINARY_DIR}/tinyxml_2_6_2.zip)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf tinyxml_2_6_2.zip                                                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}) 
			execute_process(COMMAND g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinyxml.cpp -o tinyxml.o              WORKING_DIRECTORY ${TINYXML_BUILD_DIR})
			execute_process(COMMAND g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinyxmlparser.cpp -o tinyxmlparser.o  WORKING_DIRECTORY ${TINYXML_BUILD_DIR})
			execute_process(COMMAND g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   xmltest.cpp -o xmltest.o              WORKING_DIRECTORY ${TINYXML_BUILD_DIR})
			execute_process(COMMAND g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinyxmlerror.cpp -o tinyxmlerror.o    WORKING_DIRECTORY ${TINYXML_BUILD_DIR})
			execute_process(COMMAND g++ -c -Wall -Wno-unknown-pragmas -Wno-format -O3 -fPIC   tinystr.cpp -o tinystr.o              WORKING_DIRECTORY ${TINYXML_BUILD_DIR})
			execute_process(COMMAND ar rcs libtinyxml.a tinyxml.o tinyxmlparser.o xmltest.o tinyxmlerror.o tinystr.o                WORKING_DIRECTORY ${TINYXML_BUILD_DIR})
		endif()
	endif()

endmacro()

# ///////////////////////////////////////////////////////////////////////
macro(FindFreeImage)

	if (NOT DEFINED VISUS_INTERNAL_FREEIMAGE)
		find_package(FreeImage)
	endif()

	if (VISUS_INTERNAL_FREEIMAGE OR NOT FREEIMAGE_FOUND)
		set(FREEIMAGE_BUILD_DIR    ${CMAKE_BINARY_DIR}/FreeImage)
		SET(FREEIMAGE_INCLUDE_DIRS ${FREEIMAGE_BUILD_DIR}/Dist)
		SET(FREEIMAGE_LIBRARIES    ${FREEIMAGE_BUILD_DIR}/Dist/libfreeimage-3.16.0.so)
		if (NOT EXISTS ${FREEIMAGE_LIBRARIES})
			MESSAGE("Downloading and compiling FreeImage...")
			file(DOWNLOAD https://sourceforge.net/projects/freeimage/files/Source%20Distribution/3.16.0/FreeImage3160.zip ${CMAKE_BINARY_DIR}/FreeImage3160.zip)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf FreeImage3160.zip WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			execute_process(COMMAND make                                          WORKING_DIRECTORY ${FREEIMAGE_BUILD_DIR})
		endif()
	endif()
	FixWin32FindPackageFreeImage()
endmacro()

# ///////////////////////////////////////////////////////////////////////
macro(FindOpenSSL)

	if (NOT DEFINED VISUS_INTERNAL_OPENSSL)
		if (UNIX)
			set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../../CMake/FixFindOpenSSL)
		endif()
		find_package(OpenSSL)
	endif()

	if (VISUS_INTERNAL_OPENSSL OR NOT OPENSSL_FOUND)
		set(OPENSSL_BUILD_DIR      ${CMAKE_BINARY_DIR}/openssl-1.0.2d)
		SET(OPENSSL_INCLUDE_DIR    ${OPENSSL_BUILD_DIR}/include)
		SET(OPENSSL_SSL_LIBRARY    ${OPENSSL_BUILD_DIR}/libssl.so)
		SET(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_BUILD_DIR}/libcrypto.so)
		if (NOT EXISTS ${OPENSSL_SSL_LIBRARY})
			MESSAGE("Downloading and compiling openssl...")
			file(DOWNLOAD http://sourceforge.net/projects/openssl/files/openssl-1.0.2d-fips-2.0.10/openssl-1.0.2d-src.tar.gz ${CMAKE_BINARY_DIR}/openssl-1.0.2d-src.tar.gz)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf openssl-1.0.2d-src.tar.gz WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			execute_process(COMMAND ./config shared                                       WORKING_DIRECTORY ${OPENSSL_BUILD_DIR})
			execute_process(COMMAND make                                                  WORKING_DIRECTORY ${OPENSSL_BUILD_DIR})
		endif()
	endif()
endmacro()

# ///////////////////////////////////////////////////////////////////////
macro(FindCurl)
	if (NOT DEFINED VISUS_INTERNAL_CURL)
		find_package(CURL)
	endif()

	if (VISUS_INTERNAL_CURL OR NOT CURL_FOUND)
		set(CURL_BUILD_DIR    ${CMAKE_BINARY_DIR}/curl-7.59.0)
		SET(CURL_INCLUDE_DIRS ${CURL_BUILD_DIR}/lib)
		SET(CURL_LIBRARIES    ${CURL_BUILD_DIR}/lib/.libs/libcurl.so)
		if (NOT EXISTS ${CURL_LIBRARIES})
			MESSAGE("Downloading and compiling curl...")
			file(DOWNLOAD https://curl.haxx.se/download/curl-7.59.0.tar.gz ${CMAKE_BINARY_DIR}/curl-7.59.0.tar.gz)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf curl-7.59.0.tar.gz WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			execute_process(COMMAND ./configure                                    WORKING_DIRECTORY ${CURL_BUILD_DIR})
			execute_process(COMMAND make                                           WORKING_DIRECTORY ${CURL_BUILD_DIR})
		endif()
	endif()
	FixWin32FindPackageCurl()
endmacro()

# ///////////////////////////////////////////////////////////////////////
macro(FindSwig)

	if (NOT DEFINED VISUS_INTERNAL_SWIG)
		find_package(SWIG)
	endif()
	
	if (VISUS_INTERNAL_SWIG OR NOT SWIG_FOUND)
		SET(SWIG_BUILD_DIR   ${CMAKE_BINARY_DIR}/swig-3.0.12)
		SET(SWIG_EXECUTABLE  ${SWIG_BUILD_DIR}/install/bin/swig)
		SET(SWIG_DIR         ${SWIG_BUILD_DIR}/install/share/swig/3.0.12)
		if (NOT EXISTS ${SWIG_EXECUTABLE})
			MESSAGE("Downloading and compiling swig...")
			file(DOWNLOAD https://downloads.sourceforge.net/project/swig/swig/swig-3.0.12/swig-3.0.12.tar.gz ${CMAKE_BINARY_DIR}/swig-3.0.12.tar.gz)
			execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf swig-3.0.12.tar.gz  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
			file(DOWNLOAD https://sourceforge.net/projects/pcre/files/pcre/8.42/pcre-8.42.tar.gz ${SWIG_BUILD_DIR}/pcre-8.42.tar.gz)
			execute_process(COMMAND ./Tools/pcre-build.sh                           WORKING_DIRECTORY ${SWIG_BUILD_DIR})
			execute_process(COMMAND ./configure --prefix=${SWIG_BUILD_DIR}/install  WORKING_DIRECTORY ${SWIG_BUILD_DIR})
			execute_process(COMMAND make                                            WORKING_DIRECTORY ${SWIG_BUILD_DIR})
			execute_process(COMMAND make install                                    WORKING_DIRECTORY ${SWIG_BUILD_DIR})
		endif()
		find_package(SWIG REQUIRED)
	endif()
endmacro()


