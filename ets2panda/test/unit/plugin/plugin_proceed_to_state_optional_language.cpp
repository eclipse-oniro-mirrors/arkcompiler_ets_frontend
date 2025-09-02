/**
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

#include "util.h"
#include "public/es2panda_lib.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;
static auto source = std::string("function main() { \nlet a = 5;\n arktest.assertEQ(a, 5);\n  }");

static bool CheckLanguageOptionalArg(es2panda_Context *context)
{
    auto ident = impl->CreateIdentifier(context);
    auto implicitEtsClassDef =
        impl->CreateClassDefinition2(context, ident, Es2pandaClassDefinitionModifiers::CLASS_DEFINITION_MODIFIERS_NONE,
                                     Es2pandaModifierFlags::MODIFIER_FLAGS_NONE);
    auto explicitEtsClassDef =
        impl->CreateClassDefinition5(context, ident, Es2pandaClassDefinitionModifiers::CLASS_DEFINITION_MODIFIERS_NONE,
                                     Es2pandaModifierFlags::MODIFIER_FLAGS_NONE, Es2pandaLanguage::LANGUAGE_ETS);
    auto explicitJSClassDef =
        impl->CreateClassDefinition5(context, ident, Es2pandaClassDefinitionModifiers::CLASS_DEFINITION_MODIFIERS_NONE,
                                     Es2pandaModifierFlags::MODIFIER_FLAGS_NONE, Es2pandaLanguage::LANGUAGE_JS);
    if (implicitEtsClassDef == nullptr || explicitEtsClassDef == nullptr) {
        return false;
    }
    if (impl->ClassDefinitionLanguageConst(context, explicitEtsClassDef) != Es2pandaLanguage::LANGUAGE_ETS ||
        impl->ClassDefinitionLanguageConst(context, implicitEtsClassDef) != Es2pandaLanguage::LANGUAGE_ETS ||
        impl->ClassDefinitionLanguageConst(context, explicitJSClassDef) != Es2pandaLanguage::LANGUAGE_JS) {
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    std::map<es2panda_ContextState, std::vector<std::function<bool(es2panda_Context *)>>> testFunctions;
    testFunctions[ES2PANDA_STATE_CHECKED] = {CheckLanguageOptionalArg};
    ProccedToStatePluginTestData data = {argc, argv, &impl, testFunctions, true, source};
    return RunAllStagesWithTestFunction(data);
}

// NOLINTEND
