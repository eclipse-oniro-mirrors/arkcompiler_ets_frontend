# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if(NOT PANDA_TARGET_LINUX OR NOT PANDA_WITH_ETS)
    return()
endif()

function (compile_arkts_bindings_test ABC_TARGET ETS_SOURCES OUTPUT_DIR ABC_FILES)
    set(oneValueArgs ARKTS_CONFIG)
    set(multiValueArgs COMPILER_EXTRA_OPTIONS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_custom_target(${ABC_TARGET})

    set(ABC_RESULT "")
    list(LENGTH ETS_SOURCES SOURCES_LIST_LENGTH)

    math(EXPR LAST_INDEX "${SOURCES_LIST_LENGTH} - 1")

    foreach(i RANGE ${LAST_INDEX})
        list(GET ETS_SOURCES ${i} SOURCE_ETS)
        get_filename_component(SOURCE_FNAME ${SOURCE_ETS} NAME_WE)
        set(SOURCE_ABC ${OUTPUT_DIR}/${ABC_TARGET}-${SOURCE_FNAME}.abc)
        set(SOURCE_ABC_TARGET ${ABC_TARGET}-${SOURCE_FNAME})

        set(ES2PANDA_ARGUMENTS
            --opt-level=2
            --thread=0
            --extension=ets
            --output=${SOURCE_ABC}
            --stdlib="${PANDA_ROOT}/plugins/ets/stdlib"
            ${ARG_COMPILER_EXTRA_OPTIONS}
            "${SOURCE_ETS}"
        )

        if (DEFINED ARG_ARKTS_CONFIG)
            list(APPEND ES2PANDA_ARGUMENTS "--arktsconfig=${ARG_ARKTS_CONFIG}")
        endif()

        set(COMPILE_DEPENDENCIES
            ${es2panda_target}
            etsstdlib
            ${SOURCE_ETS}
        )

        get_filename_component(ABC_OUTPUT_DIR ${SOURCE_ABC} DIRECTORY)
        file(MAKE_DIRECTORY ${ABC_OUTPUT_DIR})

        add_custom_target(${SOURCE_ABC_TARGET}
            COMMENT "Running es2panda for ets file: ${SOURCE_ETS}"
            COMMAND $<TARGET_FILE:es2panda> ${ES2PANDA_ARGUMENTS}
            DEPENDS ${COMPILE_DEPENDENCIES}
        )
        add_dependencies(${SOURCE_ABC_TARGET} arkts_bindings)

        add_dependencies(${ABC_TARGET} ${SOURCE_ABC_TARGET})
        set(ABC_RESULT ${ABC_RESULT} ${SOURCE_ABC})
    endforeach()

    set(${ABC_FILES} ${ABC_RESULT} PARENT_SCOPE)
endfunction()

function (run_arkts_bindings_test TARGET ETS_SOURCES OUTPUT_DIR)
    set(oneValueArgs ARKTS_CONFIG ENTRYPOINT)
    set(multiValueArgs ABC_LIBS RUNTIME_EXTRA_OPTIONS PROGRAM_ARGS)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(ABC_TARGET ${TARGET}-abc)

    compile_arkts_bindings_test(${ABC_TARGET} "${ETS_SOURCES}" ${OUTPUT_DIR} ABC_FILES
        ARKTS_CONFIG ${ARG_ARKTS_CONFIG}
    )

    if (DEFINED ARG_ABC_LIBS)
        list(APPEND ABC_FILES ${ARG_ABC_LIBS})
    endif()

    list(GET ABC_FILES 0 ENTRY_ABC)
    string(REPLACE ";" ":" ABC_FILES "${ABC_FILES}")

    if (NOT DEFINED ARG_ENTRYPOINT)
        list(GET ETS_SOURCES 0 ENTRY_SOURCE)
        get_filename_component(ARG_ENTRYPOINT ${ENTRY_SOURCE} NAME_WE)
    endif()

    set(BOOT_PANDA_FILES
        ${PANDA_BINARY_ROOT}/plugins/ets/etsstdlib.abc
    )

    set(RUNTIME_ARGUMENTS
        --boot-panda-files=${BOOT_PANDA_FILES}
        --panda-files=${ABC_FILES}
        --load-runtimes=ets
        --verification-mode=ahead-of-time
        ${ARG_RUNTIME_EXTRA_OPTIONS}
        ${ENTRY_ABC}
        "${ARG_ENTRYPOINT}.ETSGLOBAL::main"
    )

    if (DEFINED ARG_PROGRAM_ARGS)
        list(APPEND RUNTIME_ARGUMENTS "--" ${ARG_PROGRAM_ARGS})
    endif()

    add_custom_target(${TARGET}
        COMMAND ${PANDA_RUN_PREFIX} $<TARGET_FILE:ark> ${RUNTIME_ARGUMENTS}
    )

    add_dependencies(${TARGET} ark etsstdlib arkts_bindings ${ABC_TARGET})
endfunction()

add_custom_target(arkts_bindings_tests)
add_dependencies(es2panda_tests arkts_bindings_tests)

add_subdirectory(converters)
add_subdirectory(api)