# Copyright (c) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.

project(arkts_build_system)

set(ETS_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/arktsconfig.json)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/arktsconfig.in.json ${ETS_CONFIG})

set(ABC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/arkts_build_system.abc)

set(COMPILER_OPTIONS
    --ets-module
    --extension=ets
    --simultaneous
    --arktsconfig ${ETS_CONFIG}
    --stdlib="${PANDA_ROOT}/plugins/ets/stdlib"
    --output ${ABC_OUTPUT}
)

add_custom_target(
    arkts_build_system_ets
    COMMENT "Compiling ArkTS build system ETS code"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND $<TARGET_FILE:es2panda> ${COMPILER_OPTIONS}
    DEPENDS es2panda etsstdlib arkts_bindings
)

add_custom_target(arkts_build_system)
add_dependencies(arkts_build_system arkts_build_system_ets)
