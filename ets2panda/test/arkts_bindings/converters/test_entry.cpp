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

#include "test_helper.h"

#include "ani.h"

#include <stdexcept>

static ani_env *g_env {nullptr};

ani_env *GetAniEnv()
{
    if (!g_env) {
        throw std::runtime_error("FATAL: ANI environment is not available");
    }
    return g_env;
}

Exports *Exports::GetInstance()
{
    static Exports *instance = nullptr;
    if (instance == nullptr) {
        instance = new Exports();
    }
    return instance;
}

void Exports::AddMethod(const char *module, const char *name, void *impl)
{
    auto it = implementations_.find(module);
    if (it == implementations_.end()) {
        it = implementations_.insert(std::make_pair(module, std::vector<std::pair<std::string, void *>>())).first;
    }
    it->second.emplace_back(std::make_pair(name, impl));
}

const std::vector<std::pair<std::string, void *>> &Exports::GetMethods(const std::string &module)
{
    auto it = implementations_.find(module);
    if (it == implementations_.end()) {
        LOG_ERROR("Cannot find module with implementations: ", module);
        throw std::runtime_error("Failure");
    }
    return it->second;
}

static bool registerNativeMethods(ani_env *env, const ani_class cls,
                                  const std::vector<std::pair<std::string, void *>> impls)
{
    std::vector<ani_native_function> methods;
    methods.reserve(impls.size());
    for (const auto &[name, impl] : impls) {
        ani_native_function method;
        method.name = name.c_str();
        method.pointer = impl;
        method.signature = nullptr;
        methods.push_back(method);
    }
    return env->Class_BindNativeMethods(cls, methods.data(), methods.size()) == ANI_OK;
}

static void AniCleanUp()
{
    Exports *inst = Exports::GetInstance();
    delete inst;
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (vm->GetEnv(ANI_VERSION_1, &env) != ANI_OK) {
        return ANI_ERROR;
    }

    auto testClass = "native.TestNativeModule";
    ani_class cls = nullptr;
    env->FindClass(testClass, &cls);
    if (cls == nullptr) {
        LOG_ERROR("Cannot find managed class: ", testClass);
        return ANI_ERROR;
    }

    Exports *inst = Exports::GetInstance();
    if (!registerNativeMethods(env, cls, inst->GetMethods("TestNativeModule"))) {
        LOG_ERROR("Failed to register native methods");
        return ANI_ERROR;
    }

    g_env = env;

    AniCleanUp();

    *result = ANI_VERSION_1;
    return ANI_OK;
}