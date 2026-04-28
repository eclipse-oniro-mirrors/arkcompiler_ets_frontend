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

#ifndef ES2PANDA_COMPILER_TEST_METADATA_UTILS_ASSERTIONS_H
#define ES2PANDA_COMPILER_TEST_METADATA_UTILS_ASSERTIONS_H

#include "schemaMetadataGenerated.h"

namespace ark::es2panda::compiler::test::metadata_test {

using namespace Metadata;

class MetadataAssertions {
public:
    static void AssertClassPresented(const parser::MetadataCacheType &metadata, const std::string &expectedClassName)
    {
        const auto root = GetRoot(metadata->data());

        ASSERT_NE(root, nullptr);

        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() == expectedClassName) {
                return;
            }
        }

        FAIL() << "Class " + expectedClassName + " not found in the metadata";
    }

    static void AssertClassPresented(const pandasm::Program *program, const std::string &expectedClassName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() == expectedClassName) {
                return;
            }
        }

        FAIL() << "Class " + expectedClassName + " not found in the metadata";
    }

    static void AssertMethodPresented(const pandasm::Program *program, const std::string &className,
                                      const std::string &expectedMethodName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (auto method : *classDecl->methods()) {
                if (method->name()->str() == expectedMethodName) {
                    return;
                }
            }
        }

        FAIL() << "Method " + expectedMethodName + " not found in the metadata of class " << className;
    }

    static void AssertBuiltinReturnTypeForMethod(const pandasm::Program *program, const std::string &className,
                                                 const std::string &methodName,
                                                 const BuiltinTypeKind expectedReturnType)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (const auto method : *classDecl->methods()) {
                if (method->name()->str() != methodName) {
                    continue;
                }
                if (method->return_type_as_Builtin()->kind() == expectedReturnType) {
                    return;
                }
            }
        }

        FAIL() << "Method return type " << static_cast<int16_t>(expectedReturnType)
               << " not found in the metadata of method " << methodName << " for class " << className;
    }

    static void AssertRefReturnTypeForMethod(const pandasm::Program *program, const std::string &className,
                                             const std::string &methodName, const std::string &fqname)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (auto method : *classDecl->methods()) {
                if (method->name()->str() != methodName) {
                    continue;
                }
                if (method->return_type_as_Ref()->fqname()->string_view() == fqname) {
                    return;
                }
            }
        }

        FAIL() << "Method return type " << fqname << " not found in the metadata of method " << methodName
               << " for class " << className;
    }

    static bool HasTypeParam(const flatbuffers::Vector<::flatbuffers::Offset<TypeParamDecl>> &typeParams,
                             const std::string &expectedTypeParamName)
    {
        for (const auto typeParam : typeParams) {
            if (typeParam->name()->str() == expectedTypeParamName) {
                return true;
            }
        }
        return false;
    }

    static void AssertTypeParamPresented(const pandasm::Program *program, const std::string &className,
                                         const std::string &methodName, const std::string &expectedTypeParamName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (const auto method : *classDecl->methods()) {
                if (method->name()->str() == methodName &&
                    HasTypeParam(*method->type_params(), expectedTypeParamName)) {
                    return;
                }
            }
        }

        const auto msg = "Type parameter " + expectedTypeParamName + " of method " + methodName +
                         " not found in the metadata for class " + className;
        FAIL() << msg << std::endl;
    }

    static void AssertAnnotationPresented(const pandasm::Program *program, const std::string &expectedAnnotationName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        if (root->annotations() == nullptr) {
            FAIL() << "No annotations found in the metadata";
        }

        for (const auto annotationDecl : *root->annotations()) {
            if (annotationDecl->name()->str() == expectedAnnotationName) {
                return;
            }
        }

        FAIL() << "Annotation " + expectedAnnotationName + " not found in the metadata";
    }

    static void AssertAnnotationNotPresented(const pandasm::Program *program, const std::string &annotationName)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        if (root->annotations() == nullptr) {
            return;
        }

        for (const auto annotationDecl : *root->annotations()) {
            if (annotationDecl->name()->str() == annotationName) {
                FAIL() << "Annotation " + annotationName + " should not be found in the metadata";
            }
        }
    }

    static void AssertAnnotationsCount(const pandasm::Program *program, size_t expectedCount)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);

        if (root->annotations() == nullptr) {
            ASSERT_EQ(expectedCount, 0) << "Expected " << expectedCount << " annotations but found none";
            return;
        }

        ASSERT_EQ(root->annotations()->size(), expectedCount)
            << "Expected " << expectedCount << " annotations but found " << root->annotations()->size();
    }

    static void AssertMethodReturnTypeStringLiteral(const pandasm::Program *program, const std::string &className,
                                                    const std::string &methodName,
                                                    const std::string &expectedStringValue)
    {
        const auto root = GetRoot(program->metadata.data());

        ASSERT_NE(root, nullptr);
        for (const auto classDecl : *root->classes()) {
            if (classDecl->name()->str() != className) {
                continue;
            }
            for (const auto method : *classDecl->methods()) {
                if (method->name()->str() != methodName) {
                    continue;
                }
                const auto stringLiteralType = method->return_type_as_StringLiteral();
                ASSERT_NE(stringLiteralType, nullptr)
                    << "Method " << methodName << " should have string literal return type";
                ASSERT_EQ(stringLiteralType->value()->str(), expectedStringValue)
                    << "Method " << methodName << " should return string literal with value " << expectedStringValue;
                return;
            }
        }

        FAIL() << "Method " << methodName << " not found in metadata of class " << className;
    }
};

}  // namespace ark::es2panda::compiler::test::metadata_test

#endif  // ES2PANDA_COMPILER_TEST_METADATA_UTILS_ASSERTIONS_H