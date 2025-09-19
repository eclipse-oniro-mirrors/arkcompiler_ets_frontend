/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_COMPILER_CHECKER_ETS_ASSIGN_ANALYZER_H
#define ES2PANDA_COMPILER_CHECKER_ETS_ASSIGN_ANALYZER_H

#include "checker/ETSchecker.h"
#include "checker/ets/baseAnalyzer.h"

#include "libarkbase/utils/arena_containers.h"
#include "libarkbase/utils/small_vector.h"

namespace ark::es2panda::ir {
class AstNode;
}  // namespace ark::es2panda::ir

namespace ark::es2panda::checker {

class SmallDynBitset {
public:
    explicit SmallDynBitset(size_t nbits = 0)
    {
        Resize(nbits);
    }

    void Resize(size_t nbits)
    {
        nbits_ = nbits;
        words_.resize(WordCount(nbits_), 0);
        MaskTail();
    }

    size_t Size() const
    {
        return nbits_;
    }

    void Clear()
    {
        std::fill(words_.begin(), words_.end(), 0);
    }

    void Incl(int i)  // legacy helper
    {
        if (i < 0) {
            return;
        }
        Incl(TransformIdx(i));
    }

    void Incl(size_t i)
    {
        Ensure(i + 1);
        SetBit(i);
        MaskTail();
    }

    void Excl(int i)  // legacy helper
    {
        if (i < 0) {
            return;
        }
        Excl(TransformIdx(i));
    }

    void Excl(size_t i)
    {
        if (i >= nbits_) {
            return;
        }
        ClearBit(i);
    }

    bool IsMember(int i) const  // legacy helper
    {
        if (i < 0) {
            return false;
        }
        return IsMember(TransformIdx(i));
    }

    bool IsMember(size_t i) const
    {
        if (i >= nbits_) {
            return false;
        }
        return TestBit(i);
    }

    void InclRange(int start, int limit)  // legacy helper
    {
        InclRange(TransformIdx(std::max(start, 0)), TransformIdx(std::max(limit, 0)));
    }

    void InclRange(size_t start, size_t limit)
    {
        if (limit <= start) {
            return;
        }
        Ensure(limit);

        size_t sW = WordIndex(start);
        size_t eW = WordIndex(limit - 1);
        size_t sB = BitIndex(start);
        size_t eB = BitIndex(limit - 1);

        if (sW == eW) {
            words_[sW] |= RangeMaskSameWord(sB, eB) & WordMask(sW);
            MaskTail();
            return;
        }

        words_[sW] |= HeadMask(sB);
        for (size_t w = sW + 1; w < eW; ++w) {
            words_[w] = BS_WORD_ALL_SET;
        }
        words_[eW] |= TailMask(eB) & WordMask(eW);
        MaskTail();
    }

    void ExcludeFrom(int start)  // legacy helper
    {
        ExcludeFrom(TransformIdx(std::max(start, 0)));
    }

    void ExcludeFrom(size_t start)
    {
        if (start == 0 || start >= nbits_) {
            Clear();
            return;
        }

        size_t sW = WordIndex(start);
        size_t sB = BitIndex(start);

        for (size_t w = 0; w < sW; ++w) {
            words_[w] = 0;
        }
        if (sB > 0) {
            uint64_t below = LowBitsMask(sB);
            words_[sW] &= ~below;
        }
        MaskTail();
    }

    SmallDynBitset &AndSet(const SmallDynBitset &o)
    {
        size_t minW = std::min(words_.size(), o.words_.size());
        for (size_t i = 0; i < minW; ++i) {
            words_[i] &= o.words_[i];
        }
        if (words_.size() > minW) {
            words_.resize(minW);
        }
        nbits_ = std::min(nbits_, o.nbits_);
        MaskTail();
        return *this;
    }

    SmallDynBitset &OrSet(const SmallDynBitset &o)
    {
        if (o.words_.size() > words_.size()) {
            words_.resize(o.words_.size(), 0);
        }
        for (size_t i = 0; i < o.words_.size(); ++i) {
            words_[i] |= o.words_[i];
        }
        nbits_ = std::max(nbits_, o.nbits_);
        MaskTail();
        return *this;
    }

    SmallDynBitset &DiffSet(const SmallDynBitset &o)
    {
        size_t minW = std::min(words_.size(), o.words_.size());
        for (size_t i = 0; i < minW; ++i) {
            words_[i] &= ~o.words_[i];
        }
        MaskTail();
        return *this;
    }

    int Next(int id) const
    {
        int i = id + 1;
        if (i < 0) {
            i = 0;
        }
        if (TransformIdx(i) >= nbits_) {
            return -1;
        }
        size_t w = WordIndex(TransformIdx(i));
        size_t b = BitIndex(TransformIdx(i));

        if (w < words_.size()) {
            uint64_t cur = words_[w] & (BS_WORD_ALL_SET << b) & WordMask(w);
            if (cur) {
                return static_cast<int>((w << BS_WORD_SHIFT) + Ctz64(cur));
            }
            for (size_t j = w + 1; j < words_.size(); ++j) {
                uint64_t ww = words_[j] & WordMask(j);
                if (ww) {
                    return static_cast<int>((j << BS_WORD_SHIFT) + Ctz64(ww));
                }
            }
        }
        return -1;
    }

    void Reset()
    {
        reset_ = true;
    }

    bool IsReset()
    {
        return reset_;
    }

private:
    static constexpr size_t BS_WORD_BITS = 64;
    static constexpr size_t BS_WORD_SHIFT = 6;
    static constexpr size_t BS_WORD_MASK = BS_WORD_BITS - 1;
    static constexpr uint64_t BS_WORD_ALL_SET = ~uint64_t(0);

    SmallVector<uint64_t, 4U> words_;
    size_t nbits_ = 0;
    bool reset_ = false;  // was provided by the older Set version and still used in the analysis

    static size_t TransformIdx(int i)
    {
        return static_cast<size_t>(i);
    }

    static size_t WordCount(size_t bits)
    {
        return (bits + BS_WORD_BITS - 1) >> BS_WORD_SHIFT;
    }

    static size_t WordIndex(size_t bit)
    {
        return bit >> BS_WORD_SHIFT;
    }

    static size_t BitIndex(size_t bit)
    {
        return bit & BS_WORD_MASK;
    }

    static uint64_t LowBitsMask(size_t count)
    {
        return count == 0 ? 0 : ((uint64_t(1) << count) - 1);
    }

    static uint64_t HeadMask(size_t fromBit)
    {
        return BS_WORD_ALL_SET << fromBit;
    }

    static uint64_t TailMask(size_t toBit)
    {
        return BS_WORD_ALL_SET >> (BS_WORD_BITS - 1 - toBit);
    }

    static uint64_t RangeMaskSameWord(size_t fromBit, size_t toBit)
    {
        return HeadMask(fromBit) & TailMask(toBit);
    }

    uint64_t WordMask(size_t idx) const
    {
        if (words_.empty()) {
            return 0;
        }
        if (idx + 1 < words_.size()) {
            return BS_WORD_ALL_SET;
        }
        size_t tail = nbits_ & BS_WORD_MASK;
        return tail == 0 ? BS_WORD_ALL_SET : LowBitsMask(tail);
    }

    static uint64_t EffWordMask(size_t idx, size_t bits)
    {
        size_t wc = WordCount(bits);
        if (wc == 0) {
            return 0;
        }
        if (idx + 1 < wc) {
            return BS_WORD_ALL_SET;
        }
        size_t tail = bits & BS_WORD_MASK;
        return tail == 0 ? BS_WORD_ALL_SET : LowBitsMask(tail);
    }

    void Ensure(size_t needBits)
    {
        if (needBits <= nbits_) {
            return;
        }
        nbits_ = needBits;
        words_.resize(WordCount(nbits_), 0);
        MaskTail();
    }

    void SetBit(size_t bit)
    {
        words_[WordIndex(bit)] |= (uint64_t(1) << BitIndex(bit));
    }

    void ClearBit(size_t bit)
    {
        words_[WordIndex(bit)] &= ~(uint64_t(1) << BitIndex(bit));
    }

    bool TestBit(size_t bit) const
    {
        return (words_[WordIndex(bit)] & (uint64_t(1) << BitIndex(bit))) != 0;
    }

    uint64_t WordAt(size_t idx) const
    {
        return idx < words_.size() ? words_[idx] : 0;
    }

    void MaskTail()
    {
        if (!words_.empty()) {
            words_.back() &= WordMask(words_.size() - 1);
        }
    }

    static int Ctz64(uint64_t x)
    {
        return Ctz(x);
    }
};

using Set = SmallDynBitset;

class AssignPendingExit : public PendingExit {
public:
    explicit AssignPendingExit(const ir::AstNode *node, Set &inits, Set &uninits, bool isInitialConstructor,
                               bool hasTryFinallyBlock)
        : PendingExit(node), inits_(&inits), uninits_(&uninits)
    {
        if (isInitialConstructor || hasTryFinallyBlock) {
            exitInits_ = inits;
        }
        if (hasTryFinallyBlock) {
            exitUninits_ = uninits;
        }
    }
    ~AssignPendingExit() override = default;

    DEFAULT_COPY_SEMANTIC(AssignPendingExit);
    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(AssignPendingExit);

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes,readability-identifier-naming)
    Set *inits_;
    Set *uninits_;
    Set exitInits_;
    Set exitUninits_;
    // NOLINTEND(misc-non-private-member-variables-in-classes,readability-identifier-naming)
};

using NodeId = int;
using NodeIdMap = std::map<const ir::AstNode *, NodeId>;

class AssignAnalyzer : public BaseAnalyzer<AssignPendingExit> {
public:
    explicit AssignAnalyzer(ETSChecker *checker);
    void Analyze(const ir::AstNode *node);

    void MarkDead() override;

private:
    // node visitors
    void AnalyzeNodes(const ir::AstNode *node);
    void AnalyzeNode(const ir::AstNode *node);
    bool AnalyzeStmtNode1(const ir::AstNode *node);
    bool AnalyzeStmtNode2(const ir::AstNode *node);
    bool AnalyzeExprNode1(const ir::AstNode *node);
    bool AnalyzeExprNode2(const ir::AstNode *node);
    void AnalyzeStat(const ir::AstNode *node);
    void AnalyzeStats(const ArenaVector<ir::Statement *> &stats);
    void AnalyzeBlock(const ir::BlockStatement *blockStmt);
    void AnalyzeStructDecl(const ir::ETSStructDeclaration *structDecl);
    void AnalyzeClassDecl(const ir::ClassDeclaration *classDecl);
    void AnalyzeClassDef(const ir::ClassDefinition *classDef);
    void ProcessClassDefStaticFields(const ir::ClassDefinition *classDef);
    void CheckAnonymousClassCtor(const ir::ClassDefinition *classDef);
    void AnalyzeMethodDef(const ir::MethodDefinition *methodDef);
    void AnalyzeVarDef(const ir::VariableDeclaration *varDef);
    void AnalyzeDoLoop(const ir::DoWhileStatement *doWhileStmt);
    void AnalyzeWhileLoop(const ir::WhileStatement *whileStmt);
    void AnalyzeForLoop(const ir::ForUpdateStatement *forStmt);
    void AnalyzeForOfLoop(const ir::ForOfStatement *forOfStmt);
    void AnalyzeIf(const ir::IfStatement *ifStmt);
    void AnalyzeLabelled(const ir::LabelledStatement *labelledStmt);
    void AnalyzeSwitch(const ir::SwitchStatement *switchStmt);
    void AnalyzeTry(const ir::TryStatement *tryStmt);
    void AnalyzeBreak(const ir::BreakStatement *breakStmt);
    void AnalyzeContinue(const ir::ContinueStatement *contStmt);
    void AnalyzeReturn(const ir::ReturnStatement *retStmt);
    void AnalyzeThrow(const ir::ThrowStatement *throwStmt);
    void AnalyzeExpr(const ir::AstNode *node);
    void AnalyzeExprs(const ArenaVector<ir::Expression *> &exprs);
    void AnalyzeCond(const ir::AstNode *node);
    void AnalyzeAssignExpr(const ir::AssignmentExpression *assignExpr);
    void AnalyzeBinaryExpr(const ir::BinaryExpression *binExpr);
    void AnalyzeCallExpr(const ir::CallExpression *callExpr);
    void AnalyzeCondExpr(const ir::ConditionalExpression *condExpr);
    void AnalyzeId(const ir::Identifier *id);
    void AnalyzeMemberExpr(const ir::MemberExpression *membExpr);
    void AnalyzeNewClass(const ir::ETSNewClassInstanceExpression *newClass);
    void AnalyzeUnaryExpr(const ir::UnaryExpression *unaryExpr);
    void AnalyzeUpdateExpr(const ir::UpdateExpression *updateExpr);
    void AnalyzeArrowFunctionExpr(const ir::ArrowFunctionExpression *arrowFuncExpr);

    // utils
    void Warning(const diagnostic::DiagnosticKind &kind, const util::DiagnosticMessageParams &list,
                 const lexer::SourcePosition &pos);
    bool Trackable(const ir::AstNode *node) const;
    bool IsConstUninitializedField(const ir::AstNode *node) const;
    bool IsConstUninitializedStaticField(const ir::AstNode *node) const;
    void NewVar(const ir::AstNode *node);
    void LetInit(const ir::AstNode *node);
    void CheckInit(const ir::AstNode *node);
    void Split(const bool setToNull);
    void Merge();
    void CheckPendingExits();
    NodeId GetNodeId(const ir::AstNode *node) const;
    util::StringView GetVariableType(const ir::AstNode *node) const;
    util::StringView GetVariableName(const ir::AstNode *node) const;
    lexer::SourcePosition GetVariablePosition(const ir::AstNode *node) const;
    const ir::AstNode *GetDeclaringNode(const ir::AstNode *node);
    varbinder::Variable *GetBoundVariable(const ir::AstNode *node);
    bool VariableHasDefaultValue(const ir::AstNode *node);

    ETSChecker *checker_;
    Set inits_ {};
    Set uninits_ {};
    Set uninitsTry_ {};
    Set initsWhenTrue_ {};
    Set initsWhenFalse_ {};
    Set uninitsWhenTrue_ {};
    Set uninitsWhenFalse_ {};
    std::vector<const ir::AstNode *> varDecls_;
    const ir::ClassDefinition *globalClass_ {};
    const ir::ClassDefinition *classDef_ {};
    int classFirstAdr_ {};
    int firstNonGlobalAdr_ {};
    int firstAdr_ {};
    int nextAdr_ {};
    int returnAdr_ {};
    bool isInitialConstructor_ {};
    bool hasTryFinallyBlock_ {};
    NodeIdMap nodeIdMap_;
    int numErrors_ {};
    std::unordered_set<const ir::AstNode *> foundErrors_;
};

}  // namespace ark::es2panda::checker

#endif
