# GNSS-SDR is a Global Navigation Satellite System software-defined receiver.
# This file is part of GNSS-SDR.
#
# SPDX-FileCopyrightText: 2010-2020 C. Fernandez-Prades cfernandez(at)cttc.es
# SPDX-License-Identifier: BSD-3-Clause


set(SIGNAL_PROCESSING_TESTING_LIB_SOURCES
    acquisition_dump_reader.cc
    acquisition_msg_rx.cc
    tracking_dump_reader.cc
    tlm_dump_reader.cc
    observables_dump_reader.cc
    tracking_true_obs_reader.cc
    true_observables_reader.cc
)

file(GLOB SIGNAL_PROCESSING_TESTING_LIB_HEADERS "*.h")
list(SORT SIGNAL_PROCESSING_TESTING_LIB_HEADERS)

if(USE_CMAKE_TARGET_SOURCES)
    add_library(signal_processing_testing_lib STATIC)
    target_sources(signal_processing_testing_lib
        PRIVATE
            ${SIGNAL_PROCESSING_TESTING_LIB_SOURCES}
        PUBLIC
            ${SIGNAL_PROCESSING_TESTING_LIB_HEADERS}
    )
else()
    source_group(Headers FILES ${SIGNAL_PROCESSING_TESTING_LIB_HEADERS})
    add_library(signal_processing_testing_lib
        ${SIGNAL_PROCESSING_TESTING_LIB_SOURCES}
        ${SIGNAL_PROCESSING_TESTING_LIB_HEADERS}
    )
endif()

target_link_libraries(signal_processing_testing_lib
    PUBLIC
        Armadillo::armadillo
        Gnuradio::runtime
        Gnuradio::pmt
    PRIVATE
        Boost::headers
        Matio::matio
)

if(ENABLE_GLOG_AND_GFLAGS)
    target_link_libraries(signal_processing_testing_lib PRIVATE Gflags::gflags Glog::glog)
    target_compile_definitions(signal_processing_testing_lib PRIVATE -DUSE_GLOG_AND_GFLAGS=1)
else()
    target_link_libraries(signal_processing_testing_lib PRIVATE absl::flags absl::log)
endif()

target_include_directories(signal_processing_testing_lib
    PUBLIC
        ${GNSSSDR_SOURCE_DIR}/src/core/interfaces
    INTERFACE
        ${GNSSSDR_SOURCE_DIR}/tests/common-files
)

if(USE_GENERIC_LAMBDAS)
    set(has_generic_lambdas HAS_GENERIC_LAMBDA=1)
    set(no_has_generic_lambdas HAS_GENERIC_LAMBDA=0)
    target_compile_definitions(signal_processing_testing_lib
        PRIVATE
            "$<$<COMPILE_FEATURES:cxx_generic_lambdas>:${has_generic_lambdas}>"
            "$<$<NOT:$<COMPILE_FEATURES:cxx_generic_lambdas>>:${no_has_generic_lambdas}>"
    )
else()
    target_compile_definitions(signal_processing_testing_lib
        PRIVATE
            -DHAS_GENERIC_LAMBDA=0
    )
endif()

if(USE_BOOST_BIND_PLACEHOLDERS)
    target_compile_definitions(signal_processing_testing_lib
        PRIVATE
            -DUSE_BOOST_BIND_PLACEHOLDERS=1
    )
endif()

if(GNURADIO_USES_STD_POINTERS)
    target_compile_definitions(signal_processing_testing_lib
        PUBLIC -DGNURADIO_USES_STD_POINTERS=1
    )
endif()

if(PMT_USES_BOOST_ANY)
    target_compile_definitions(signal_processing_testing_lib
        PRIVATE
            -DPMT_USES_BOOST_ANY=1
    )
endif()

if(ENABLE_CLANG_TIDY)
    if(CLANG_TIDY_EXE)
        set_target_properties(signal_processing_testing_lib
            PROPERTIES
                CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
        )
    endif()
endif()

set_property(TARGET signal_processing_testing_lib
    APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
