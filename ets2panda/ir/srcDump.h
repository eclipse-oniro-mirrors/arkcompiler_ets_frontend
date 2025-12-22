/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd. All rights reserved.
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

#ifndef ES2PANDA_IR_SRCDUMP_H
#define ES2PANDA_IR_SRCDUMP_H

#include "ir/astNode.h"
#include "parser/JsdocHelper.h"

namespace ark::es2panda::ir {

// Forward declarations
class ClassDefinition;
class TSTypeAliasDeclaration;
class ClassProperty;
class TSInterfaceDeclaration;
class TSEnumDeclaration;

class SrcDumper;

class Declgen {
public:
    using NodeVariant =
        std::variant<std::monostate, const ark::es2panda::ir::ClassDefinition *,
                     const ark::es2panda::ir::TSTypeAliasDeclaration *, const ark::es2panda::ir::ClassProperty *,
                     const ark::es2panda::ir::TSInterfaceDeclaration *, const ark::es2panda::ir::TSEnumDeclaration *>;

    explicit Declgen(public_lib::Context *ctx) : ctx_ {ctx} {}

    void DumpNode(SrcDumper *dumper, const std::string &key);

    void Dump(SrcDumper *dumper, const checker::Type *type);

    void CollectImport(const ir::ImportDeclaration *import);

    class Lock {
    private:
        class Releaser {
        public:
            NO_COPY_SEMANTIC(Releaser);
            Releaser &operator=(Releaser &&other) = delete;
            Releaser(Releaser &&other) : lock_ {other.lock_}, prevAcquired_ {other.prevAcquired_}
            {
                other.lock_ = nullptr;
            }

            Releaser() : lock_ {nullptr}, prevAcquired_ {false} {}
            explicit Releaser(Lock *lock) : lock_ {lock}, prevAcquired_ {lock->acquired_}
            {
                if (lock_ != nullptr) {
                    lock_->releasers_++;
                }
            }

            ~Releaser()
            {
                if (lock_ != nullptr) {
                    lock_->acquired_ = prevAcquired_;
                    lock_->releasers_--;
                }
            }

        private:
            Lock *lock_;
            bool prevAcquired_;
        };

    public:
        void Acquire()
        {
            ES2PANDA_ASSERT(releasers_ != 0);
            acquired_ = true;
        }

        bool IsAcquired()
        {
            return acquired_;
        }

        [[nodiscard]] auto BuildReleaser()
        {
            return Releaser {this};
        }

        [[nodiscard]] static auto BuildEmptyReleaser()
        {
            return Releaser {};
        }

    private:
        bool acquired_ {false};
        size_t releasers_ {};
    };

    // Ambient
    auto BuildAmbientContextGuard()
    {
        return ambientDeclarationLock_.BuildReleaser();
    }

    void TryDeclareAmbientContext(SrcDumper *srcDumper);

    void DumpImports(std::string &res);

    // Postdump
    auto BuildPostDumpIndirectDepsPhaseLockGuard()
    {
        return postDumpIndirectDepsPhaseLock_.BuildReleaser();
    }

    void SetPostDumpIndirectDepsPhase()
    {
        postDumpIndirectDepsPhaseLock_.Acquire();
    }

    bool IsPostDumpIndirectDepsPhase()
    {
        return postDumpIndirectDepsPhaseLock_.IsAcquired();
    }

    template <typename T>
    void AddNode(const std::string &key, T *value)
    {
        unExportNode_[key] = NodeVariant(value);
    }

    template <typename T>
    void PushTask(T &&task)
    {
        taskQueue_.emplace(std::forward<T>(task));
    }

    void Run();

    auto GetCtx() const
    {
        return ctx_;
    }

private:
    public_lib::Context *ctx_;

    // track 'declare' keyword:
    Lock ambientDeclarationLock_;

    /* "pre-dump": */
    std::vector<const ir::ImportDeclaration *> imports_;

    /* "post-dump": */
    Lock postDumpIndirectDepsPhaseLock_;
    // queued nodes that need to be post-dumped:
    std::queue<std::function<void()>> taskQueue_ {};
    // a dictionary with "hidden" nodes that may be dumped at post-dump.
    // NOTE(dkofanov): it should be aware of names collision, 'string' is to be changed to 'Variable *'.
    std::unordered_map<std::string, NodeVariant> unExportNode_ {};
};

class SrcDumper {
public:
    // Delete after the bindings problem solved:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    explicit SrcDumper([[maybe_unused]] const ir::AstNode *node)
    {
        ES2PANDA_UNREACHABLE();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    explicit SrcDumper([[maybe_unused]] const ir::AstNode *node, [[maybe_unused]] bool isDeclgen)
    {
        ES2PANDA_UNREACHABLE();
    }

    explicit SrcDumper(Declgen *dg = nullptr);
    explicit SrcDumper(const ir::AstNode *node, bool enableJsdocDump, Declgen *dg);

    void Add(std::string_view str);
    void Add(char ch);
    void Add(int8_t i);
    void Add(int16_t i);
    void Add(int32_t i);
    void Add(int64_t l);
    void Add(float f);
    void Add(double d);

    std::string Str() const;

    void IncrIndent();
    void DecrIndent();
    void Endl(size_t num = 1);

    bool IsDeclgen() const;
    Declgen *GetDeclgen() const;

    auto BuildAmbientContextGuard()
    {
        if (IsDeclgen()) {
            return GetDeclgen()->BuildAmbientContextGuard();
        }
        return Declgen::Lock::BuildEmptyReleaser();
    }

    void DumpJsdocBeforeTargetNode(const ir::AstNode *inputNode);

    void DumpExports();

    void SetDefaultExport() noexcept;
    [[nodiscard]] bool HasDefaultExport() const noexcept;

private:
    std::stringstream ss_;
    std::string indent_;

    /* declgen-specific: */
    Declgen *dg_;
    std::unique_ptr<parser::JsdocHelper> jsdocGetter_ {};
    // Flag to avoid duplicate default export declarations
    bool hasDefaultExport_ = false;
};

}  // namespace ark::es2panda::ir

#endif  // ES2PANDA_IR_SRCDUMP_H
