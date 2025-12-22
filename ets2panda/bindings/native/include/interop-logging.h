/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstdint>

inline void Log(const char *msg)
{
    std::cout << msg << "\n";
}

template <typename... Args>
void LogI(Args &&...args)
{
    (std::cout << ... << args);
    std::cout << "\n";
}

template <typename... Args>
void LogE(Args &&...args)
{
    (std::cerr << ... << args);
    std::cerr << "\n";
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_PUBLIC ""

#if defined(PANDA_TARGET_WINDOWS)
#define INTEROP_API_EXPORT __declspec(dllexport)
#else
#define INTEROP_API_EXPORT __attribute__((visibility("default")))
#endif

#endif  // INTEROP_LOGGING_H
