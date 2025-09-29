# Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

add_custom_target(es2panda_relative_path_to_file)

function(copy_to_workdir filename)
    get_filename_component(filename_without_ext ${filename} NAME_WLE)
    add_custom_target("copy_test_${filename_without_ext}"
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${filename}
            ${CMAKE_CURRENT_BINARY_DIR}/${filename_without_ext}/${filename})
endfunction()

function(launch_with_reletaive_path_single_file filename)
    copy_to_workdir(${filename})
    get_filename_component(filename_without_ext ${filename} NAME_WLE)
    set(filename_abc "${filename_without_ext}.abc")
    add_custom_command(
        OUTPUT "${filename_abc}"
        DEPENDS es2panda copy_test_${filename_without_ext}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${filename_without_ext}
        COMMAND ${CMAKE_COMMAND} -E env
            LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${PANDA_RUN_PREFIX}
            $<TARGET_FILE:es2panda> --extension=ets --output=${filename_abc}
            ${filename}
    )
    add_custom_target(es2panda_relative_path_to_file_${filename_without_ext} DEPENDS ${filename_abc})
    add_dependencies(es2panda_relative_path_to_file es2panda_relative_path_to_file_${filename_without_ext})
endfunction()

add_dependencies(es2panda_tests es2panda_relative_path_to_file)

launch_with_reletaive_path_single_file(package_a_file_1.ets)
