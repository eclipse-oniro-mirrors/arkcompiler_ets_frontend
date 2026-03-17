/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef INTEROP_LOGGING_H
#define INTEROP_LOGGING_H

#include <iostream>

template <typename... Args>
void LOG_INFO(Args &&...args)
{
    (std::cout << ... << args);
    std::cout << "\n";
}

template <typename... Args>
void LOG_ERROR(Args &&...args)
{
    (std::cerr << ... << args);
    std::cerr << "\n";
}

#endif  // INTEROP_LOGGING_H
