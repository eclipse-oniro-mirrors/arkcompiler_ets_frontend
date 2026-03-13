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

#include "converters-ani.h"
#include "interop-logging.h"

#include "ani.h"

#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

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

std::vector<std::string> Exports::GetModules()
{
    std::vector<std::string> result;
    for (auto &it : implementations_) {
        result.push_back(it.first);
    }
    return result;
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
        LOG_ERROR("Module", module.c_str(), "is not registered");
        throw std::runtime_error("Fatal error");
    }
    return it->second;
}

static std::map<std::string, std::string> g_ModuleClasses = {
    {"Es2pandaNativeModule", "@arkts-bindings.Es2pandaNativeModule.Es2pandaNativeModule"},
    {"GeneratedEs2pandaNativeModule", "@arkts-bindings.generated.Es2pandaNativeModule.Es2pandaNativeModule"},
    {"InteropNativeModule", "@arkts-bindings.InteropNativeModule.InteropNativeModule"},
};

static const std::string GetModuleClass(const std::string &module)
{
    auto moduleClass = g_ModuleClasses.find(module);
    if (moduleClass != g_ModuleClasses.end()) {
        return moduleClass->second;
    }
    return "";
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

static bool registerModules(ani_env *env)
{
    Exports *inst = Exports::GetInstance();
    for (const auto &module : inst->GetModules()) {
        std::string moduleClass = GetModuleClass(module);
        if (moduleClass.empty()) {
            LOG_ERROR("Class for module ", module.c_str(), " is not registered");
            return false;
        }
        ani_class cls = nullptr;
        env->FindClass(moduleClass.c_str(), &cls);
        if (cls == nullptr) {
            LOG_ERROR("Cannot find managed class ", moduleClass.c_str());
            return false;
        }
        if (!registerNativeMethods(env, cls, inst->GetMethods(module))) {
            LOG_ERROR("Failed to register methods for class ", moduleClass.c_str());
            return false;
        }
    }
    return true;
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

    if (!registerModules(env)) {
        return ANI_ERROR;
    }

    g_env = env;
    *result = ANI_VERSION_1;

    AniCleanUp();

    return ANI_OK;
}
