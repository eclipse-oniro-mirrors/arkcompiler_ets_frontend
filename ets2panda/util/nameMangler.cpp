/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nameMangler.h"

namespace ark::es2panda::util {
std::string NameMangler::CreateMangledNameByTypeAndName(LangFeatureType type, const util::StringView &nodeName)
{
    ES2PANDA_ASSERT(!nodeName.Empty());

    int lastPos = nodeName.Mutf8().find_last_of('.') + 1;
    std::string mangledName = nodeName.Mutf8().substr(0, lastPos);

    switch (type) {
        case ASYNC: {
            mangledName += "%%async-";
            break;
        }
        case GET: {
            mangledName += "<get>";
            break;
        }
        case PARTIAL: {
            return nodeName.Mutf8() + "$partial";
        }
        case PROPERTY: {
            mangledName += "<property>";
            break;
        }
        case SET: {
            mangledName += "<set>";
            break;
        }
        default:
            ES2PANDA_UNREACHABLE();
    }

    mangledName += nodeName.Mutf8().substr(lastPos);
    return mangledName;
}

std::string NameMangler::CreateMangledNameForLambdaInvoke(size_t invokeCounter)
{
    std::string mangledName = "lambda_invoke";

    mangledName += "-" + std::to_string(invokeCounter);
    return mangledName;
}

std::string NameMangler::CreateMangledNameForLambdaObject(const util::StringView &lambdaInvokeName)
{
    ES2PANDA_ASSERT(!lambdaInvokeName.Empty());

    std::string mangledName = "%%lambda-";

    mangledName += lambdaInvokeName.Mutf8();

    return mangledName;
}

std::string NameMangler::CreateMangledNameForUnionProperty(const std::string &propTypeName)
{
    ES2PANDA_ASSERT(!propTypeName.empty());

    std::string mangledName = "%%union_prop-";
    mangledName += propTypeName;
    std::replace(mangledName.begin(), mangledName.end(), '.', '_');

    return mangledName;
}

std::string NameMangler::CreateMangledNameForAnnotation(const std::string &baseName, const std::string &annotationName)
{
    ES2PANDA_ASSERT(!baseName.empty() && !annotationName.empty());

    std::string mangledName = "%%annotation-";
    mangledName += annotationName;

    return baseName + mangledName;
}

std::string NameMangler::AppendToAnnotationName(const std::string &annotationName, const std::string &secondPart)
{
    // Note (oeotvos) This ES2PANDA_ASSERT might be a bit too much here. Just create the name, or not?
    ES2PANDA_ASSERT(annotationName.find("%%annotation") != 0);

    return annotationName + "-" + secondPart;
}

std::string NameMangler::GetOriginalClassNameFromPartial(const std::string &partialName)
{
    const std::string partialSuffix = "$partial";

    if (partialName.length() <= partialSuffix.length()) {
        return "";
    }

    size_t suffixPos = partialName.rfind(partialSuffix);
    if (suffixPos == std::string::npos) {
        return "";
    }

    // Check if the suffix is at the end of the string
    if (suffixPos + partialSuffix.length() != partialName.length()) {
        return "";
    }

    return partialName.substr(0, suffixPos);
}
}  // namespace ark::es2panda::util
