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

#include "signature_help_items.h"
#include <optional>
#include <string>
#include <vector>
#include "libarkbase/utils/arena_containers.h"

namespace ark::es2panda::lsp {

SignatureHelpItems CreateSignatureHelpItems(std::vector<checker::Signature *> &signatures,
                                            checker::Signature *signature,
                                            std::optional<ArgumentListInfo> argumentListInfo)
{
    SignatureHelpItems items;

    size_t selectedItemIndex = -1;
    size_t itemsSeen = 0;
    const size_t one = 1;

    for (size_t i = itemsSeen; i < signatures.size(); i++) {
        auto &currentSignature = signatures[i];
        if (currentSignature->ToString() == signature->ToString()) {
            selectedItemIndex = itemsSeen;
            break;
        }
        itemsSeen++;
    }
    ES2PANDA_ASSERT(signatures.size() != 0);
    ES2PANDA_ASSERT(selectedItemIndex != static_cast<size_t>(-1));

    for (const auto &helpItem : GetSignatureHelpItem(signatures)) {
        items.SetItems(helpItem);
    }

    items.SetApplicableSpan(argumentListInfo->GetApplicableSpan().start, argumentListInfo->GetApplicableSpan().length);

    items.SetSelectedItemIndex(selectedItemIndex);

    size_t argumentIndex = argumentListInfo->GetArgumentIndex();
    const auto selectedSignature = signatures[selectedItemIndex];

    if (selectedSignature->HasRestParameter() && selectedSignature->ArgCount() > one) {
        argumentIndex = selectedSignature->ArgCount();
    } else {
        argumentIndex = std::min(argumentIndex, selectedSignature->ArgCount() - one);
    }

    items.SetArgumentIndex(argumentIndex);
    items.SetArgumentCount(argumentListInfo->GetArgumentCount());

    return items;
}

std::vector<SignatureHelpItem> GetSignatureHelpItem(const std::vector<checker::Signature *> &signatures)
{
    std::vector<SignatureHelpItem> items;
    if (signatures.empty()) {
        return items;
    }
    for (auto *signature : signatures) {
        const auto item = CreateSignatureHelpItem(*signature);
        items.push_back(item);
    }

    return items;
}

SignatureHelpItem CreateSignatureHelpItem(checker::Signature &signature)
{
    const checker::SignatureInfo *signatureInfo = signature.GetSignatureInfo();
    SignatureHelpItem item;
    if (!signatureInfo->typeParams.empty()) {
        item.SetPrefixDisplayParts(CreatePunctuation("<"));
        for (auto it = signatureInfo->typeParams.begin(); it != signatureInfo->typeParams.end(); ++it) {
            std::string typeName = (*it)->ToString();
            item.SetPrefixDisplayParts(CreateTypeName(typeName));
            if (std::next(it) != signatureInfo->typeParams.end()) {
                item.SetPrefixDisplayParts(CreatePunctuation(","));
                item.SetSeparatorDisplayParts(CreatePunctuation(","));
            }
        }
        item.SetPrefixDisplayParts(CreatePunctuation(">"));
    }
    item.SetPrefixDisplayParts(CreatePunctuation("("));

    SetSignatureHelpParameter(signatureInfo, item);

    item.SetSuffixDisplayParts(CreatePunctuation(")"));

    if (signature.HasFunction() ||
        (signature.OwnerVar() != nullptr && signature.OwnerVar()->HasFlag(varbinder::VariableFlags::METHOD))) {
        item.SetSuffixDisplayParts(CreatePunctuation(":"));
    } else {
        item.SetSuffixDisplayParts(CreatePunctuation("=>"));
    }

    std::string returnType = signature.ReturnType()->ToString();
    item.SetSuffixDisplayParts(CreateTypeName(returnType));

    return item;
}

void SetSignatureHelpParameter(const checker::SignatureInfo *signatureInfo, SignatureHelpItem &signatureHelpItem)
{
    for (auto it = signatureInfo->params.begin(); it != signatureInfo->params.end(); it++) {
        SignatureHelpParameter param;

        std::string paramName =
            (!(*it)->Name().StartsWith(GENSYM_CORE) ? std::string((*it)->Name().Utf8()) : std::string(DUMMY_ID));
        param.SetName(paramName);
        param.SetDisplayParts(SignatureCreateParameterName(paramName));

        if ((*it)->HasFlag(varbinder::VariableFlags::OPTIONAL)) {
            param.SetDisplayParts(CreatePunctuation("?"));
        }

        param.SetDisplayParts(CreatePunctuation(":"));

        std::string paramType = (*it)->TsType()->ToString();
        param.SetDisplayParts(CreateTypeName(paramType));

        if (std::next(it) != signatureInfo->params.end()) {
            param.SetDisplayParts(CreatePunctuation(","));
        }

        signatureHelpItem.SetParameters(param);
    }

    if (signatureInfo->restVar == nullptr) {
        return;
    }

    SignatureHelpParameter param;

    if (!signatureInfo->params.empty()) {
        param.SetDisplayParts(CreatePunctuation(","));
    }
    param.SetDisplayParts(CreatePunctuation("..."));

    std::string paramName =
        (!signatureInfo->restVar->Name().StartsWith(GENSYM_CORE) ? std::string(signatureInfo->restVar->Name().Utf8())
                                                                 : std::string(DUMMY_ID));
    param.SetDisplayParts(SignatureCreateParameterName(paramName));

    param.SetDisplayParts(CreatePunctuation(":"));

    std::string paramType = signatureInfo->restVar->TsType()->ToString();
    param.SetDisplayParts(CreateTypeName(paramType));

    signatureHelpItem.SetParameters(param);
}

}  // namespace ark::es2panda::lsp