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

#ifndef ES2PANDA_TEST_UNIT_IMPORT_PATH_MANAGER_IMPORT_ARCHIVE_TEST_H
#define ES2PANDA_TEST_UNIT_IMPORT_PATH_MANAGER_IMPORT_ARCHIVE_TEST_H

#include <optional>

#include "import_path_manager_test.h"
#include "libarkfile/file_item_container.h"
#include "libarkfile/file_writer.h"
#include "schemaMetadataGenerated.h"

namespace ark::es2panda::util::test {

struct ArchiveModule {
    std::string descriptor;
    std::optional<std::string> declaration;
};

// Build the same class descriptors and string annotation that the declaration reader consumes.
// Controlling each record independently lets an archive contain both text-only and metadata modules.
class ImportArchiveTest : public ImportPathManagerTest {
protected:
    void BuildArchive(const std::vector<ArchiveModule> &modules, panda_file::MetadataByPackages metadata = {},
                      std::string_view archiveName = "bundle.abc")
    {
        using panda_file::AnnotationItem;
        panda_file::ItemContainer container;
        auto *annotationClass =
            container.GetOrCreateForeignClassItem(std::string(ImportPathManager::ANNOTATION_MODULE_DECLARATION));
        auto *name = container.GetOrCreateStringItem("declaration");
        for (const auto &module : modules) {
            auto *record = container.GetOrCreateClassItem(module.descriptor);
            if (module.declaration.has_value()) {
                auto *text = container.GetOrCreateStringItem(*module.declaration);
                std::vector<AnnotationItem::Elem> elements {{name, container.GetOrCreateIdValueItem(text)}};
                // Ark bytecode uses C as the scalar string annotation tag.
                std::vector<AnnotationItem::Tag> tags {AnnotationItem::Tag('C')};
                record->AddAnnotation(container.CreateItem<AnnotationItem>(annotationClass, elements, tags));
            }
        }
        // Metadata indices refer to strings in the constant pool by their final file offsets.
        for (const auto &[packageName, records] : metadata) {
            container.GetOrCreateStringItem(packageName);
            for (const auto &record : records) {
                container.GetOrCreateStringItem(record.first);
            }
        }
        ASSERT_TRUE(container.SetMetadataItems(std::move(metadata)));
        panda_file::FileWriter writer(Path(archiveName).string());
        ASSERT_TRUE(container.Write(&writer));
    }

    static panda_file::EncodedMetadata EmptyDeclarations()
    {
        flatbuffers::FlatBufferBuilder builder;
        Metadata::FinishDeclsBuffer(builder, Metadata::CreateDecls(builder));
        return {builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize()};
    }
};

}  // namespace ark::es2panda::util::test

#endif
