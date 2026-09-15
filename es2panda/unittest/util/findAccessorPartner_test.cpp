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

#include <gtest/gtest.h>

#include <macros.h>
#include <mem/arena_allocator.h>
#include <mem/pool_manager.h>
#include <util/helpers.h>
#include <util/ustring.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace panda::es2panda::util {

// Minimal stand-in for a class member / object-literal property: the scan rule under test
// lives in FindAccessorPartner itself, and the lambdas below mirror the two call sites
// (ClassDefinition::CompileMissingProperties / ObjectExpression::CompileRemainingProperties).
struct MockElem {
    std::string name;           // property key; "" doubles as "has no name" (a field)
    bool isMethod {false};      // class side: MethodDefinition vs field / index signature
    bool computed {false};      // computed key (class and object sides)
    bool spread {false};        // object side only
    bool oppositeKind {false};  // opposite accessor kind of the pair
    bool staticMismatch {false};
    bool consumed {false};      // models the call sites' compiled.Test(j) bitset entry
    bool noStaticName {false};  // key LiteralToPropName cannot name (BigInt, private identifier)
};

namespace {

// The accessor whose partner is being scanned (props[start]); its own fields are irrelevant.
MockElem Start(std::string name)
{
    MockElem elem;
    elem.name = std::move(name);
    return elem;
}

MockElem Named(std::string name)  // same-named method on the class side, data/init on the object side
{
    MockElem elem;
    elem.name = std::move(name);
    elem.isMethod = true;
    return elem;
}

MockElem Computed(std::string name)
{
    MockElem elem = Named(std::move(name));
    elem.computed = true;
    return elem;
}

MockElem Partner(std::string name)
{
    MockElem elem = Named(std::move(name));
    elem.oppositeKind = true;
    return elem;
}

MockElem StaticMismatchPartner(std::string name)
{
    MockElem elem = Partner(std::move(name));
    elem.staticMismatch = true;
    return elem;
}

MockElem ComputedPartner(std::string name)
{
    MockElem elem = Partner(std::move(name));
    elem.computed = true;
    return elem;
}

MockElem ConsumedPartner(std::string name)
{
    MockElem elem = Partner(std::move(name));
    elem.consumed = true;
    return elem;
}

MockElem ConsumedNamed(std::string name)
{
    MockElem elem = Named(std::move(name));
    elem.consumed = true;
    return elem;
}

MockElem NoStaticName(std::string name)
{
    MockElem elem = Named(std::move(name));
    elem.noStaticName = true;
    return elem;
}

MockElem Field(std::string name = "")
{
    MockElem elem;
    elem.name = std::move(name);
    return elem;
}

MockElem Spread()
{
    MockElem elem;
    elem.spread = true;
    return elem;
}

util::StringView View(std::string_view name)
{
    return util::StringView(name);
}

// ArenaVector draws from the panda memory pool, which must be initialized per test.
class MemManager {
public:
    MemManager()
    {
        constexpr auto COMPILER_SIZE = 8192_MB;

        mem::MemConfig::Initialize(0, 0, COMPILER_SIZE, 0);
        PoolManager::Initialize(PoolType::MMAP);
    }

    NO_COPY_SEMANTIC(MemManager);
    NO_MOVE_SEMANTIC(MemManager);

    ~MemManager()
    {
        PoolManager::Finalize();
        mem::MemConfig::Finalize();
    }
};

size_t ScanClassSide(const std::vector<MockElem> &elems, size_t start, std::string_view propName)
{
    MemManager memManager;
    ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    ArenaAllocatorAdapter<const MockElem *> adapter(&allocator);
    ArenaVector<const MockElem *> props(adapter);
    for (const auto &elem : elems) {
        props.push_back(&elem);
    }

    auto mayAliasKey = [](const MockElem *elem) {
        // A key without a static name may alias the pair key at runtime
        // (ToString(1n) === "1"): it stops the scan, it is never skipped.
        return elem->isMethod && (elem->computed || elem->noStaticName);
    };
    auto nameOf = [](const MockElem *elem) -> util::StringView {
        return elem->isMethod ? util::StringView(std::string_view(elem->name)) : util::StringView();
    };
    auto isEligiblePartner = [](const MockElem *elem, size_t) {
        // Fields answer with "" and "" is a valid key: a non-method member reaching this
        // check must fail it instead of being cast (see CompileMissingProperties). The
        // consumed flag mirrors the call sites' !compiled.Test(j) guard.
        return !elem->consumed && elem->isMethod && elem->oppositeKind && !elem->staticMismatch;
    };

    return FindAccessorPartner(props, start, View(propName), mayAliasKey, nameOf, isEligiblePartner);
}

size_t ScanObjectSide(const std::vector<MockElem> &elems, size_t start, std::string_view propName)
{
    MemManager memManager;
    ArenaAllocator allocator(SpaceType::SPACE_TYPE_COMPILER, nullptr, true);
    ArenaAllocatorAdapter<const MockElem *> adapter(&allocator);
    ArenaVector<const MockElem *> props(adapter);
    for (const auto &elem : elems) {
        props.push_back(&elem);
    }

    auto mayAliasKey = [](const MockElem *elem) {
        return elem->spread || elem->computed || elem->noStaticName;
    };
    auto nameOf = [](const MockElem *elem) -> util::StringView {
        return util::StringView(std::string_view(elem->name));
    };
    auto isEligiblePartner = [](const MockElem *elem, size_t) {
        return !elem->consumed && elem->oppositeKind;
    };

    return FindAccessorPartner(props, start, View(propName), mayAliasKey, nameOf, isEligiblePartner);
}

struct ScanCase {
    const char *desc;
    std::vector<MockElem> elems;
    std::string propName;
    size_t expected;
    size_t start {0};
};

void RunScanCases(const std::function<size_t(const std::vector<MockElem> &, size_t, std::string_view)> &scan,
                  const std::vector<ScanCase> &cases)
{
    for (const auto &testCase : cases) {
        SCOPED_TRACE(testCase.desc);
        EXPECT_EQ(scan(testCase.elems, testCase.start, testCase.propName), testCase.expected);
    }
}

}  // namespace

TEST(FindAccessorPartnerTest, ClassSideGate)
{
    const std::vector<ScanCase> cases = {
        {"adjacent eligible partner", {Start("x"), Partner("x")}, "x", 1},
        {"differently-named members are skipped", {Start("x"), Named("y"), Partner("x")}, "x", 2},
        {"computed member stops the scan", {Start("x"), Computed("y"), Partner("x")}, "x", 3},
        {"same-named method stops the scan without probing further",
         {Start("x"), Named("x"), Partner("x")}, "x", 3},
        {"static-mismatched partner stops the scan", {Start("x"), StaticMismatchPartner("x"), Partner("x")}, "x", 3},
        {"no same-named member", {Start("x"), Named("y"), Partner("z")}, "x", 3},
        {"empty body", {}, "x", 0},
        {"start is the last member", {Start("x")}, "x", 1},
        {"first of two eligible partners wins", {Start("x"), Partner("x"), Partner("x")}, "x", 1},
        {"empty key with unnamed field stops the scan", {Start(""), Field(), Partner("")}, "", 3},
        {"empty key pair still merges", {Start(""), Partner("")}, "", 1},
        {"members before start are never examined, not even an aliasing one",
         {Computed("x"), Start("x"), Partner("x")}, "x", 2, 1},
        {"an alias after the partner does not block the merge",
         {Start("x"), Partner("x"), Computed("y")}, "x", 1},
        {"a computed same-named partner is stopped by the alias check before its name",
         {Start("x"), ComputedPartner("x")}, "x", 2},
        {"a consumed same-named partner stops the scan",
         {Start("x"), ConsumedPartner("x")}, "x", 2},
        {"consumed differently-named members are still skipped",
         {Start("x"), ConsumedNamed("y"), Partner("x")}, "x", 2},
        {"a member without a static name stops the scan",
         {Start("x"), NoStaticName("1n")}, "x", 2},
        {"a member without a static name after the partner does not block",
         {Start("x"), Partner("x"), NoStaticName("1n")}, "x", 1},
    };

    RunScanCases(ScanClassSide, cases);
}

TEST(FindAccessorPartnerTest, ObjectSideGate)
{
    const std::vector<ScanCase> cases = {
        {"adjacent eligible partner", {Start("a"), Partner("a")}, "a", 1},
        {"differently-named properties are skipped", {Start("a"), Named("b"), Partner("a")}, "a", 2},
        {"spread stops the scan", {Start("a"), Spread(), Partner("a")}, "a", 3},
        {"computed property stops the scan", {Start("a"), Computed("b"), Partner("a")}, "a", 3},
        {"same-named data property stops the scan", {Start("a"), Named("a"), Partner("a")}, "a", 3},
        {"no same-named property", {Start("a"), Named("b"), Partner("c")}, "a", 3},
        {"a computed same-named partner is stopped by the alias check before its name",
         {Start("a"), ComputedPartner("a")}, "a", 2},
        {"first of two eligible partners wins", {Start("a"), Partner("a"), Partner("a")}, "a", 1},
        {"empty body", {}, "a", 0},
        {"start is the last property", {Start("a")}, "a", 1},
        {"properties before start are never examined, not even a spread",
         {Spread(), Start("a"), Partner("a")}, "a", 2, 1},
        {"a spread after the partner does not block the merge",
         {Start("a"), Partner("a"), Spread()}, "a", 1},
        {"a consumed same-named partner stops the scan",
         {Start("a"), ConsumedPartner("a")}, "a", 2},
        {"consumed differently-named properties are still skipped",
         {Start("a"), ConsumedNamed("b"), Partner("a")}, "a", 2},
        {"empty key pair still merges", {Start(""), Partner("")}, "", 1},
        {"a property without a static name stops the scan",
         {Start("a"), NoStaticName("2n")}, "a", 2},
    };

    RunScanCases(ScanObjectSide, cases);
}

}  // namespace panda::es2panda::util
