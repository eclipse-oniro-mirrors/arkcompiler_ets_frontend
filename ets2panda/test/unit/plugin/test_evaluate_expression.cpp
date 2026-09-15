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

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "public/es2panda_lib.h"
#include "util.h"
#include "util/base64.h"

namespace {

constexpr size_t MIN_ABC_SIZE = 64;
constexpr char ABC_MAGIC[] = "PAND";

bool DecodeBase64(const std::string &base64, std::string &out)
{
    out = ark::es2panda::util::Base64Decode(base64);
    return !out.empty();
}

bool IsValidAbc(const std::string &abcBytes)
{
    return abcBytes.size() >= MIN_ABC_SIZE && memcmp(abcBytes.data(), ABC_MAGIC, strlen(ABC_MAGIC)) == 0;
}

bool ContainsString(const std::string &abcBytes, const std::string &target)
{
    return abcBytes.find(target) != std::string::npos;
}

// Exact string-pool signatures from real artifacts. An external method's
// signature enters the pool only when referenced: presence == call emitted.
namespace sig {
constexpr char GET[] = "std.debug.DebuggerAPI.get:i32;i32;std.core.String;std.core.Object;";
constexpr char DEFINE_VARIABLE[] =
    "std.debug.DebuggerAPI.defineVariable:i32;i32;std.core.String;std.core.Object;u1;void;";
constexpr char SET[] = "std.debug.DebuggerAPI.set:i32;i32;std.core.String;std.core.Object;void;";
constexpr char SET_ELEMENT[] = "std.debug.DebuggerAPI.setElement:i32;i32;std.core.String;i32;std.core.Object;void;";
constexpr char WRAP[] = "std.debug.DebuggerAPI.wrap:std.core.Object;std.debug.DebugProxy;";
constexpr char GET_THIS[] = "std.debug.DebuggerAPI.getThis:i32;i32;std.core.Object;";
constexpr char CALL_FUNCTION[] =
    "std.debug.DebuggerAPI.callFunction:i32;i32;std.core.String;std.core.String;std.core.Object[];"
    "std.debug.DebugProxy;";
constexpr char CALL_SUPER[] =
    "std.debug.DebuggerAPI.callSuper:i32;i32;std.core.String;std.core.Object[];std.debug.DebugProxy;";
constexpr char NEW_INSTANCE[] =
    "std.debug.DebuggerAPI.newInstance:i32;i32;std.core.String;std.core.Object[];std.core.Object;";
constexpr char CAST_AS[] = "std.debug.DebuggerAPI.castAs:i32;i32;std.core.Object;std.core.String;std.core.Object;";
constexpr char INSTANCEOF[] = "std.debug.DebuggerAPI.instanceofOp:i32;i32;std.core.Object;std.core.String;u1;";
constexpr char TYPEOF[] = "std.debug.DebuggerAPI.typeofValue:std.core.Object;std.core.String;";
constexpr char TO_INT[] = "std.debug.DebuggerAPI.toInt:std.core.Object;i32;";
constexpr char TO_DOUBLE[] = "std.debug.DebuggerAPI.toDouble:std.core.Object;f64;";
constexpr char TO_LONG[] = "std.debug.DebuggerAPI.toLong:std.core.Object;i64;";
constexpr char TO_FLOAT[] = "std.debug.DebuggerAPI.toFloat:std.core.Object;f32;";
constexpr char TO_BYTE[] = "std.debug.DebuggerAPI.toByte:std.core.Object;i8;";
constexpr char TO_SHORT[] = "std.debug.DebuggerAPI.toShort:std.core.Object;i16;";
constexpr char TO_CHAR[] = "std.debug.DebuggerAPI.toChar:std.core.Object;u16;";
constexpr char TO_BOOLEAN[] = "std.debug.DebuggerAPI.toBoolean:std.core.Object;u1;";
// Binary operators all share the (Object, Object) -> Object shape.
constexpr char ADD[] = "std.debug.DebuggerAPI.add:std.core.Object;std.core.Object;std.core.Object;";
constexpr char SUB[] = "std.debug.DebuggerAPI.sub:std.core.Object;std.core.Object;std.core.Object;";
constexpr char MUL[] = "std.debug.DebuggerAPI.mul:std.core.Object;std.core.Object;std.core.Object;";
constexpr char DIV[] = "std.debug.DebuggerAPI.div:std.core.Object;std.core.Object;std.core.Object;";
constexpr char MOD[] = "std.debug.DebuggerAPI.mod:std.core.Object;std.core.Object;std.core.Object;";
constexpr char POW[] = "std.debug.DebuggerAPI.pow:std.core.Object;std.core.Object;std.core.Object;";
constexpr char SHL[] = "std.debug.DebuggerAPI.shl:std.core.Object;std.core.Object;std.core.Object;";
constexpr char SHR[] = "std.debug.DebuggerAPI.shr:std.core.Object;std.core.Object;std.core.Object;";
constexpr char USHR[] = "std.debug.DebuggerAPI.ushr:std.core.Object;std.core.Object;std.core.Object;";
constexpr char BIT_AND[] = "std.debug.DebuggerAPI.bitAnd:std.core.Object;std.core.Object;std.core.Object;";
constexpr char BIT_OR[] = "std.debug.DebuggerAPI.bitOr:std.core.Object;std.core.Object;std.core.Object;";
constexpr char BIT_XOR[] = "std.debug.DebuggerAPI.bitXor:std.core.Object;std.core.Object;std.core.Object;";
constexpr char LT[] = "std.debug.DebuggerAPI.lt:std.core.Object;std.core.Object;std.core.Object;";
constexpr char GT[] = "std.debug.DebuggerAPI.gt:std.core.Object;std.core.Object;std.core.Object;";
constexpr char LE[] = "std.debug.DebuggerAPI.le:std.core.Object;std.core.Object;std.core.Object;";
constexpr char GE[] = "std.debug.DebuggerAPI.ge:std.core.Object;std.core.Object;std.core.Object;";
constexpr char NEG[] = "std.debug.DebuggerAPI.neg:std.core.Object;std.core.Object;";
constexpr char POS[] = "std.debug.DebuggerAPI.pos:std.core.Object;std.core.Object;";
constexpr char BIT_NOT[] = "std.debug.DebuggerAPI.bitNot:std.core.Object;std.core.Object;";
constexpr char PROXY_GET_FIELD[] = "std.debug.DebugProxy.getField:std.core.String;std.debug.DebugProxy;";
constexpr char PROXY_SET_VALUE[] = "std.debug.DebugProxy.setValue:std.debug.DebugProxy;void;";
constexpr char PROXY_VALUE[] = "std.debug.DebugProxy.value:std.core.Object;";
constexpr char PROXY_INVOKE[] = "std.debug.DebugProxy.invoke:std.core.Object[];std.debug.DebugProxy;";
constexpr char PROXY_CALL[] = "std.debug.DebugProxy.call:std.core.String;std.core.Object[];std.debug.DebugProxy;";
constexpr char ARRAY_GET[] = "std.core.Array.$_get:i32;std.core.Object;";
constexpr char STRING_GET[] = "std.core.String.$_get:i32;std.core.String;";
constexpr char ARRAY_SET[] = "std.core.Array.$_set:i32;std.core.Object;void;";
constexpr char ARRAY_CREATE[] = "std.core.Array.create:i32;std.core.Object;std.core.Array;";
constexpr char RUNTIME_EVALUATE[] = "runtime_evaluate";
// Module record name: eval_E<pid>_<n>_eval.ETSGLOBAL by default, plain
// ETSGLOBAL under --ets-unnamed.
constexpr char ETSGLOBAL_RECORD[] = "ETSGLOBAL;";
}  // namespace sig

struct TestCase {
    const char *expression;
    const char *name;
    // Signatures that must be present in the string pool (= API calls emitted).
    std::vector<const char *> expected = {};
    // Signatures/strings that must be absent (guards against wrong dispatch).
    std::vector<const char *> forbidden = {};
};

bool Verify(const std::string &abcBytes, const TestCase &testCase)
{
    if (!IsValidAbc(abcBytes)) {
        return false;
    }
    for (auto *signature : testCase.expected) {
        if (!ContainsString(abcBytes, signature)) {
            std::cerr << "  missing: " << signature << std::endl;
            return false;
        }
    }
    for (auto *signature : testCase.forbidden) {
        if (ContainsString(abcBytes, signature)) {
            std::cerr << "  unexpected: " << signature << std::endl;
            return false;
        }
    }
    return true;
}

bool TestEvaluateExpression(es2panda_Impl *impl, es2panda_Config *config, const TestCase &testCase)
{
    std::string base64Input = ark::es2panda::util::Base64Encode(testCase.expression);

    char *outBase64 = impl->EvaluateExpression(config, base64Input.c_str());
    if (outBase64 == nullptr) {
        std::cerr << "[" << testCase.name << "] FAILED: returned null" << std::endl;
        return false;
    }

    std::string abcBytes;
    if (!DecodeBase64(outBase64, abcBytes)) {
        std::cerr << "[" << testCase.name << "] FAILED: base64 decode failed" << std::endl;
        impl->FreeExpressionResult(outBase64);
        return false;
    }
    impl->FreeExpressionResult(outBase64);

    if (!Verify(abcBytes, testCase)) {
        std::cerr << "[" << testCase.name << "] FAILED: verification failed" << std::endl;
        return false;
    }

    std::cout << "[" << testCase.name << "] PASS" << std::endl;
    return true;
}

// Only genuine parse failures return nullptr (unsupported-but-parseable
// constructs degrade to NullLiteral; see K_TEST_CASES).
int RunUnsupportedCases(es2panda_Impl *impl, es2panda_Config *config)
{
    const char *kUnsupported[] = {
        "import(\"mod\")",
        // ETS parses comma sequence expressions only inside for-headers
        // (ETSParser::ParsePotentialExpressionSequence requires IN_FOR), so a
        // top-level 'a, b' is a syntax error -> nullptr (the transformer's
        // sequence handler is unreachable from this entry point).
        "a, b",
        // negative dimension: ESE0247 in the checker, compile failure
        "new int[-1](0)",
        // optional chaining on this is rejected by the parser (ESY115549)
        "this?.x",
        // plain syntax error -> nullptr
        "a +",
        // 'new T[n]' without an initializer parses as a class instantiation
        // of an array type and fails the checker (ESY63526) -> nullptr
        "new int[5]",
        // multi-dimensional new-array hits the same ESY63526 path -> nullptr
        "new int[2][3]",
        // yield outside a generator function is a syntax error (ESY0227)
        "yield srcVal",
        // compound-operator destructuring parses the LHS as an
        // ArrayExpression and fails the parser (INVALID_LEFT_SIDE_IN_
        // ASSIGNMENT) -> nullptr (only the plain '=' form is ETSDestructuring)
        "[a, b] += srcArr",
        // var is rejected by the parser (ESY0297) -> nullptr
        "var c = 3",
        // declaration without initializer: ESY0105 error recovery leaves
        // diagnostics that fail the pipeline -> nullptr
        "let a;",
        // multi-statement declaration input: extraction requires exactly one
        // declaration statement -> nullptr (no silent drop of the declaration)
        "let a = 1; a + 1",
        // constant literal casts keep the native `as` node: the standard
        // checker applies the mainline rules (ESE1050320/ESE123811/ESE0326)
        "256 as byte",
        "3.99 as int",
        "65 as char",
        "1 as boolean",
        // char literal in arithmetic/bitwise/shift: mainline ESE0107/ESE0108
        "c'A' & 0x1F",
        // char in string concatenation: mainline ESE4201 (implicit
        // char->string conversion disallowed)
        "\"\" + c'A'",
        // negative bigint literal exponent: mainline ESE655064
        "2n ** -1n",
        // instantiated generic on instanceof RHS: mainline ESY18871
        "box instanceof EvalBox<number>",
        // ValueArray requires a primitive element type (ESE1547180)
        "new ValueArray<Object>(3, o)",
        // builtin array type reference without type arguments:
        // FIXED_ARRAY_PARAM_ERROR, mainline-identical
        "new FixedArray(10, 0)",
        "x as FixedArray",
        nullptr,
    };
    int failed = 0;
    for (int i = 0; kUnsupported[i] != nullptr; i++) {
        std::string base64 = ark::es2panda::util::Base64Encode(kUnsupported[i]);
        char *result = impl->EvaluateExpression(config, base64.c_str());
        if (result != nullptr) {
            std::cerr << "[unsupported_" << i << "] FAILED: expected nullptr for '" << kUnsupported[i] << "'"
                      << std::endl;
            impl->FreeExpressionResult(result);
            failed++;
        } else {
            std::cout << "[unsupported_" << i << "] PASS (returned null as expected)" << std::endl;
        }
    }
    return failed;
}

const TestCase K_TEST_CASES[] = {
    // Entry-point contract: module record ETSGLOBAL with a static
    // runtime_evaluate entry.
    {"a", "entry_contract", {sig::RUNTIME_EVALUATE, sig::ETSGLOBAL_RECORD}, {sig::SET, sig::PROXY_GET_FIELD}},

    // Literals: no API signature at all in the pool.
    {"\"hello world\"", "literal_string", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"42", "literal_number", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"true", "literal_boolean", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"null", "literal_null", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"undefined", "literal_undefined", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"123n", "literal_bigint", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"'a'", "literal_char", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // wrapper-injection regression: wrapper-breaking punctuation must stay
    // inert inside a string literal
    {"\"{(;)}\"", "literal_wrapper_punctuation", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},

    // Template literals: quasi/expr interleaving via DebuggerAPI.add
    // (log-point style formatting).
    {"`a=${x} b=${y}`", "template_literal", {sig::GET, sig::ADD}},
    {"`v=${a.x} w=${this.f}`",
     "template_literal_members",
     {sig::GET, sig::GET_THIS, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ADD}},

    // Identifier: get() only.
    {"a", "identifier", {sig::GET}, {sig::WRAP, sig::PROXY_VALUE, sig::SET}},

    // Unary operators.
    {"-a", "unary_neg", {sig::GET, sig::NEG}},
    {"+a", "unary_pos", {sig::GET, sig::POS}},
    {"~a", "unary_bitnot", {sig::GET, sig::BIT_NOT}},
    // Logical NOT: native ets.istrue, no API dispatch.
    {"!a", "unary_not", {sig::GET}, {sig::NEG, sig::BIT_NOT}},
    {"!!flagVar", "double_logical_not", {sig::GET}, {sig::NEG, sig::ADD, sig::PROXY_GET_FIELD}},
    {"!(flagOne && flagTwo)", "parenthesized_logical_not", {sig::GET}, {sig::NEG, sig::ADD, sig::PROXY_GET_FIELD}},
    // Unary/typeof applied to non-identifier operands.
    {"-a.x", "unary_neg_member", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::NEG}},
    {"-a.foo()", "unary_neg_call", {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::NEG}},
    {"typeof a.x", "typeof_member", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::TYPEOF}},
    {"typeof this", "typeof_this", {sig::GET_THIS, sig::TYPEOF}, {sig::GET}},

    // String building chains (IDE watch/log formatting).
    {"a + \"x\" + b", "string_concat_chain", {sig::GET, sig::ADD}},

    // Arithmetic operators.
    {"a + b", "arithmetic_add", {sig::GET, sig::ADD}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a - b", "arithmetic_sub", {sig::GET, sig::SUB}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a * b", "arithmetic_mul", {sig::GET, sig::MUL}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a / b", "arithmetic_div", {sig::GET, sig::DIV}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a % b", "arithmetic_mod", {sig::GET, sig::MOD}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a ** b", "arithmetic_pow", {sig::GET, sig::POW}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a << b", "arithmetic_shl", {sig::GET, sig::SHL}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a >> b", "arithmetic_shr", {sig::GET, sig::SHR}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a >>> b", "arithmetic_ushr", {sig::GET, sig::USHR}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a & b", "arithmetic_bitand", {sig::GET, sig::BIT_AND}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a | b", "arithmetic_bitor", {sig::GET, sig::BIT_OR}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},
    {"a ^ b", "arithmetic_bitxor", {sig::GET, sig::BIT_XOR}, {sig::PROXY_GET_FIELD, sig::PROXY_CALL}},

    // Relational comparisons.
    {"a < b", "compare_lt", {sig::GET, sig::LT}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"a > b", "compare_gt", {sig::GET, sig::GT}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"a <= b", "compare_le", {sig::GET, sig::LE}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"a >= b", "compare_ge", {sig::GET, sig::GE}, {sig::ADD, sig::PROXY_GET_FIELD}},

    // native short-circuit; only get() is dispatched
    {"a && b", "logical_and", {sig::GET}, {sig::ADD, sig::SUB, sig::PROXY_GET_FIELD}},
    {"a || b", "logical_or", {sig::GET}, {sig::ADD, sig::SUB, sig::PROXY_GET_FIELD}},
    // compound conditions over member/call operands (conditional
    // breakpoints): native ets.istrue on the proxy results
    {"objAlpha.fieldOne && objBeta.fieldTwo",
     "logical_members",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE},
     {sig::ADD, sig::LT}},
    {"objAlpha.methodOne() || objBeta.methodTwo()",
     "logical_calls",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::ADD, sig::LT}},
    {"valueOne < valueTwo && valueTwo < valueThree",
     "chained_comparison",
     {sig::GET, sig::LT},
     {sig::ADD, sig::GT, sig::PROXY_GET_FIELD}},
    {"valueOne >= lowBound && valueOne <= highBound",
     "range_check",
     {sig::GET, sig::GE, sig::LE},
     {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},

    // native comparison; operands still load via get()
    {"a == b", "equality_eq", {sig::GET}, {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},
    {"a === b", "equality_strict_eq", {sig::GET}, {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},
    {"a != b", "equality_neq", {sig::GET}, {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},
    {"a !== b", "equality_strict_neq", {sig::GET}, {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},
    // null equality check: native comparison on the get() result
    {"maybeNull == null", "null_equality", {sig::GET}, {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},
    // Null/undefined equality keeps a REAL runtime comparison: the
    // Object-typed producers (bare get / proxy $_get) can deliver null at
    // runtime, and the `as Any` operand cast prevents the checker from
    // folding the comparison to constant false (W65001).
    {"null == x", "equality_null_lhs", {sig::GET}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"x != null", "equality_null_neq", {sig::GET}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"x === null", "equality_null_strict", {sig::GET}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"maybeNull == undefined", "equality_null_undefined", {sig::GET}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"arr[0] == null", "equality_null_subscript", {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}, {sig::ADD}},
    {"objAlpha.fieldOne == null",
     "equality_null_member",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE},
     {sig::ADD, sig::SET}},

    // typeof.
    {"typeof a", "typeof_expr", {sig::GET, sig::TYPEOF}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"typeof valueOne == \"number\"",
     "typeof_string_compare",
     {sig::GET, sig::TYPEOF},
     {sig::ADD, sig::LT, sig::PROXY_GET_FIELD}},

    // wrap(get()).getField().value()
    {"a.x",
     "member_access",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE},
     {sig::PROXY_SET_VALUE, sig::SET}},
    {"a.x.y", "member_chain", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    {"a.b.c.d", "member_chain_deep", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    // conditional-expression receivers route through the generic object path
    {"(condVar ? objOne : objTwo).fieldAlpha",
     "conditional_member_receiver",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE},
     {sig::GET_THIS, sig::CALL_SUPER}},
    {"(condVar ? objOne : objTwo).methodAlpha()",
     "conditional_call_receiver",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::GET_THIS}},
    {"this.innerObj.deepField",
     "this_deep_member",
     {sig::GET_THIS, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE},
     {sig::GET}},
    {"this.getMethod().resultField",
     "this_call_then_member",
     {sig::GET_THIS, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::PROXY_GET_FIELD},
     {sig::GET}},
    // subscript on this: getThis + Array<Object> cast + $_get
    {"this[0]",
     "this_subscript",
     {sig::GET_THIS, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::GET, sig::TO_INT}},
    // super call result chained into another member call
    {"super.foo().bar()",
     "super_call_chain",
     {sig::CALL_SUPER, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::GET, sig::GET_THIS}},
    // super property read (vs the super.foo() call): the object transform
    // degrades super to a null literal wrapped into the proxy chain
    {"super.stateField",
     "super_property_degrades",
     {sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, "stateField"},
     {sig::GET, sig::GET_THIS, sig::CALL_SUPER}},
    {"this", "this_expr", {sig::GET_THIS}, {sig::GET}},
    {"this.x", "this_member", {sig::GET_THIS, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    {"a!", "non_null_assertion", {sig::GET}},

    // Subscript reads: literal / nested / member-base / string-key /
    // conditional-index forms.
    {"arr[0]", "subscript_read_literal", {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}, {sig::TO_INT}},
    {"a.b[0]",
     "member_then_subscript",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::PROXY_CALL},
     {sig::TO_INT}},
    {"a[b[i]]", "nested_subscript", {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},
    {"obj[\"x\"]", "string_subscript_read", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    {"arr[cond ? i : j]",
     "subscript_conditional_index",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},
    // String indexing (spec_700 "String Indexing Expression"): literal
    // receivers keep the native String.$_get lowering; Any-typed receivers
    // (variables, members, call results) dispatch at runtime through
    // DebugProxy.call reflection.
    {"\"abc\"[1]", "string_literal_subscript", {sig::STRING_GET}, {sig::ARRAY_GET, sig::WRAP, sig::TO_INT}},
    {"\"abc\"[idxVar]",
     "string_literal_subscript_var_index",
     {sig::GET, sig::TO_INT, sig::STRING_GET},
     {sig::ARRAY_GET, sig::WRAP}},
    {"strVar[2]",
     "string_var_subscript",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::ARRAY_GET, sig::TO_INT}},
    {"strVar[idxVar]",
     "string_var_subscript_var_index",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::ARRAY_GET}},
    // 2D subscript chains: each dimension goes through its own runtime
    // $_get dispatch
    {"matrixVar[rowIdx][colIdx]",
     "subscript_2d",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::SET, sig::SET_ELEMENT}},
    {"matrixVar[0][1]",
     "subscript_2d_literal_index",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::TO_INT, sig::SET, sig::SET_ELEMENT}},
    // negative literal folds to a NumberLiteral at parse: direct index,
    // no toInt bridge
    {"arrIndex[-1]",
     "subscript_negative_literal",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::TO_INT, sig::NEG, sig::SET}},
    // cast operand in index position: toInt inside the toInt bridge
    {"arrIndex[x as int]",
     "cast_as_subscript_index",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::SET, sig::PROXY_GET_FIELD}},

    // Calls.
    {"a.foo(b, c)", "method_call_multiarg", {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},
    {"foo(a)", "func_call_module", {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE}},
    // Explicit generic instantiation: the 4th callFunction argument carries
    // the comma-joined type-arg names ("" when absent) so the runtime can
    // reproduce the compiler's instantiation-boundary conversions.
    {"f<double>(x)", "generic_call_primitive", {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE, "double"}},
    {"g<long,String>(a, b)", "generic_call_multi", {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE, "long,String"}},
    // qualified type args are unencodable -> "?" placeholder literal
    {"h<a.b.Foo>(x)", "generic_call_qualified_unknown", {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE, "?"}},
    // nested generics keep the outer name only (inner params dropped)
    {"h<Array<Int>>(x)", "generic_call_nested_outer_name", {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE, "Array"}},
    {"arr[i]()",
     "elem_callee_call",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_INVOKE, sig::PROXY_VALUE}},
    // restArgsLowering expands the spread at the call site
    {"a.foo(...arr)", "spread_arg", {sig::GET, sig::WRAP, sig::PROXY_CALL, "%%get-length"}},
    {"foo(...arr)", "spread_func", {sig::CALL_FUNCTION, "%%get-length"}},

    // Zero-argument calls (toString()/size() style accessors).
    {"a.foo()",
     "method_call_zeroarg",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::TO_INT, sig::ARRAY_GET, sig::SET}},
    {"foo()", "func_call_zeroarg", {sig::CALL_FUNCTION, sig::PROXY_VALUE}, {sig::GET}},
    {"this.foo()", "this_method_call", {sig::GET_THIS, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}, {sig::GET}},

    // super.foo(): ALLOW_SUPER holds for any function body, so the wrapper
    // parses it; the transform routes through runtime callSuper.
    {"super.foo()", "super_method_call", {sig::CALL_SUPER, sig::PROXY_VALUE}, {sig::GET, sig::WRAP}},
    {"super.foo(a, b)", "super_method_call_args", {sig::CALL_SUPER, sig::GET, sig::PROXY_VALUE}, {sig::WRAP}},

    // Chained calls and members on call results (fluent style).
    {"a.foo().bar()", "method_call_chain", {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},
    {"a.foo().x",
     "member_of_call_result",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::PROXY_GET_FIELD}},
    {"foo(bar(x))", "nested_func_call", {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE}},

    // subscript / new / array-literal operands in argument position
    {"takeFunc(arrIndex[idxVar])",
     "subscript_as_argument",
     {sig::CALL_FUNCTION, sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {sig::NEW_INSTANCE}},
    {"objAlpha.methodOne(arrIndex[idxVar])",
     "subscript_as_method_argument",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::TO_INT}},
    {"takeFunc(new KlassOne())",
     "new_as_argument",
     {sig::CALL_FUNCTION, sig::NEW_INSTANCE, sig::PROXY_VALUE},
     {sig::GET}},
    {"takeFunc(new int[3](0))",
     "newarray_as_argument",
     {sig::CALL_FUNCTION, sig::ARRAY_CREATE, sig::PROXY_VALUE},
     {sig::GET, sig::NEW_INSTANCE}},
    // non-empty array literal lowers through Array.create + $_set fill
    {"takeFunc([valOne, valTwo])",
     "array_literal_as_argument",
     {sig::CALL_FUNCTION, sig::GET, sig::PROXY_VALUE, sig::ARRAY_CREATE, sig::ARRAY_SET},
     {sig::NEW_INSTANCE}},
    // regular argument followed by a spread in the same call
    {"objAlpha.methodOne(elemAlpha, ...restArr)",
     "mixed_args_and_spread",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ARRAY_GET, "%%get-length"}},

    // identifier LHS writes back via set, not setElement
    {"a = 5", "assign_simple", {sig::SET}, {sig::SET_ELEMENT, sig::PROXY_SET_VALUE}},
    {"a.x = 5",
     "assign_member",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE},
     {sig::SET, sig::SET_ELEMENT}},
    // deeper LHS chains recurse through BuildMemberLHS
    {"a.b.c = 5",
     "assign_member_deep",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE},
     {sig::SET, sig::SET_ELEMENT}},
    {"this.x = 5",
     "assign_this_member",
     {sig::GET_THIS, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE},
     {sig::GET, sig::SET, sig::SET_ELEMENT}},
    {"this.innerObj.deepField = 5",
     "assign_this_deep_member",
     {sig::GET_THIS, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE},
     {sig::GET, sig::SET, sig::SET_ELEMENT}},
    // member read on the RHS of a member write (self-referencing object)
    {"objAlpha.fieldOne = objAlpha.fieldTwo",
     "assign_member_from_member",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::PROXY_SET_VALUE},
     {sig::SET, sig::SET_ELEMENT}},
    // member-value arrays keep the $_set path (setElement is name-based)
    {"a.b[i] = v",
     "assign_member_subscript",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::TO_INT, sig::PROXY_CALL},
     {sig::SET, sig::SET_ELEMENT, sig::ARRAY_SET}},
    {"arr[i] = arr[j]",
     "subscript_read_write",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::SET_ELEMENT}},
    {"arrIndex[0] = new KlassOne()",
     "new_as_subscript_rhs",
     {sig::NEW_INSTANCE, sig::SET_ELEMENT},
     {sig::GET, sig::ARRAY_SET}},
    // setElement resolves the array by name; the orphaned get("arr") emits
    // no call.
    {"arr[0] = 42", "subscript_write", {sig::SET_ELEMENT}, {sig::GET, sig::ARRAY_SET, sig::PROXY_SET_VALUE}},
    {"arr[i] = 42", "subscript_write_var", {sig::GET, sig::TO_INT, sig::SET_ELEMENT}, {sig::ARRAY_SET}},

    // Compound assignments / updates.
    {"arr[i] += 1",
     "subscript_compound_var",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ADD, sig::SET_ELEMENT}},
    {"arr[0] += 1", "subscript_compound", {sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ADD, sig::SET_ELEMENT}},
    {"obj[\"x\"] = 5",
     "string_subscript_write",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE},
     {sig::SET, sig::SET_ELEMENT}},
    {"obj[\"x\"] += 1",
     "string_subscript_compound",
     {sig::PROXY_GET_FIELD, sig::ADD, sig::PROXY_SET_VALUE},
     {sig::SET_ELEMENT}},
    {"a += 1", "assign_compound_id", {sig::GET, sig::ADD, sig::SET}},
    {"a.x += 1",
     "assign_compound_mem",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ADD, sig::PROXY_SET_VALUE}},
    // compound with member read on the RHS as well
    {"a.x += b.y",
     "compound_member_both_sides",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ADD, sig::PROXY_SET_VALUE}},
    {"a **= 2", "assign_compound_pow", {sig::GET, sig::POW, sig::SET}},
    // remaining GetCompoundOpMethod mappings (-= *= /= %= &= |= ^= <<= >>= >>>=)
    {"x -= 2", "assign_compound_sub", {sig::GET, sig::SUB, sig::SET}},
    {"x *= 2", "assign_compound_mul", {sig::GET, sig::MUL, sig::SET}},
    {"x /= 2", "assign_compound_div", {sig::GET, sig::DIV, sig::SET}},
    {"x %= 2", "assign_compound_mod", {sig::GET, sig::MOD, sig::SET}},
    {"x &= 1", "assign_compound_bitand", {sig::GET, sig::BIT_AND, sig::SET}},
    {"x |= 1", "assign_compound_bitor", {sig::GET, sig::BIT_OR, sig::SET}},
    {"x ^= 1", "assign_compound_bitxor", {sig::GET, sig::BIT_XOR, sig::SET}},
    {"x <<= 1", "assign_compound_shl", {sig::GET, sig::SHL, sig::SET}},
    {"x >>= 1", "assign_compound_shr", {sig::GET, sig::SHR, sig::SET}},
    {"x >>>= 1", "assign_compound_ushr", {sig::GET, sig::USHR, sig::SET}},
    // identifier RHS (both sides dispatched via get/set)
    {"counterVar += stepVar", "assign_compound_id_rhs", {sig::GET, sig::ADD, sig::SET}},
    // subscript compound with the same array read on the RHS
    {"arrIndex[idxVar] += arrIndex[idxTwo]",
     "subscript_compound_self",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ADD, sig::SET_ELEMENT},
     {sig::ARRAY_SET}},
    {"arrIndex[idxVar] + arrIndex[idxTwo]",
     "subscript_arith",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ADD},
     {sig::SET, sig::SET_ELEMENT}},
    // chained assignment: both writes settle; the identifier-LHS read of a
    // plain '=' is orphaned, so no get is emitted at all
    {"targetVar = sourceVar = 77",
     "assign_chained",
     {sig::SET, "targetVar", "sourceVar"},
     {sig::GET, sig::ADD, sig::PROXY_GET_FIELD}},
    // update on non-identifier operands: member field and array element
    {"a.x++",
     "update_member_postfix",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ADD, sig::PROXY_SET_VALUE}},
    {"arr[i]++",
     "update_subscript",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ADD, sig::SET_ELEMENT},
     {sig::ARRAY_SET}},
    // prefix update on member / subscript operands
    {"++a.x",
     "update_prefix_member",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ADD, sig::PROXY_SET_VALUE}},
    {"++arr[i]",
     "update_prefix_subscript",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ADD, sig::SET_ELEMENT},
     {sig::ARRAY_SET}},
    // update expression in argument position: writeback settles before the call
    {"foo(a++)", "update_as_argument", {sig::CALL_FUNCTION, sig::GET, sig::ADD, sig::SET, sig::PROXY_VALUE}},
    // updates inside both branches settle per-branch (no lift needed: the
    // native conditional keeps the writebacks in its own branches)
    {"cond ? (i++) : (j--)", "update_in_conditional", {sig::GET, sig::ADD, sig::SUB, sig::SET}},
    {"--a", "update_prefix_dec", {sig::GET, sig::SUB, sig::SET}},
    {"++a", "update_prefix_inc", {sig::GET, sig::ADD, sig::SET}},
    {"a--", "update_postfix_dec", {sig::GET, sig::SUB, sig::SET}},
    {"a++", "update_postfix_inc", {sig::GET, sig::ADD, sig::SET}},
    {"--a + a++", "update_mixed", {sig::SUB, sig::ADD, sig::SET}},

    // Casts: primitives via toXxx, named types via castAs.
    {"d as int", "as_primitive_int", {sig::GET, sig::TO_INT}, {sig::CAST_AS, sig::TO_DOUBLE, sig::TO_LONG}},
    {"d as double", "as_primitive_double", {sig::GET, sig::TO_DOUBLE}, {sig::CAST_AS, sig::TO_INT, sig::TO_LONG}},
    {"d as long", "as_primitive_long", {sig::GET, sig::TO_LONG}, {sig::CAST_AS, sig::TO_INT, sig::TO_DOUBLE}},
    {"d as float", "as_primitive_float", {sig::GET, sig::TO_FLOAT}, {sig::CAST_AS, sig::TO_INT, sig::TO_DOUBLE}},
    {"d as byte", "as_primitive_byte", {sig::GET, sig::TO_BYTE}, {sig::CAST_AS, sig::TO_INT, sig::TO_DOUBLE}},
    {"d as short", "as_primitive_short", {sig::GET, sig::TO_SHORT}, {sig::CAST_AS, sig::TO_INT, sig::TO_DOUBLE}},
    {"d as char", "as_primitive_char", {sig::GET, sig::TO_CHAR}, {sig::CAST_AS, sig::TO_INT, sig::TO_DOUBLE}},
    {"d as boolean", "as_primitive_boolean", {sig::GET, sig::TO_BOOLEAN}, {sig::CAST_AS, sig::TO_INT, sig::TO_DOUBLE}},
    {"obj as String", "as_ref_string", {sig::GET, sig::CAST_AS}},
    {"obj as MyObj", "as_ref_user", {sig::CAST_AS}},
    {"a as Object", "as_object", {sig::GET, sig::CAST_AS}},
    // cast result used as a call receiver
    {"(x as MyObj).method()",
     "cast_receiver_call",
     {sig::GET, sig::CAST_AS, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},
    // cast operands inside arithmetic
    {"(a as int) + (b as int)", "cast_operands_arith", {sig::GET, sig::TO_INT, sig::ADD}},
    // `as void` degrades to null (valid ABC, no dispatch)
    {"x as void", "as_void_degrades", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // parenthesized type annotation: UnparenthesizeType strips to the primitive
    {"x as (int)", "as_paren_type", {sig::GET, sig::TO_INT}, {sig::CAST_AS}},
    // constant literal casts keep the native `as` node (no toXxx dispatch):
    // legal in-range forms compile and evaluate natively, identical result
    {"5 as int", "as_literal_identity_native", {}, {sig::TO_INT, sig::CAST_AS}},
    {"5 as byte", "as_literal_inrange_native", {}, {sig::TO_BYTE, sig::CAST_AS}},
    // char relational is spec-legal and stays on the DebuggerAPI dispatch
    {"c'A' < c'B'", "char_literal_relational_dispatch", {sig::LT}, {}},
    // positive bigint exponent keeps the pow dispatch
    {"2n ** 2n", "bigint_pow_positive_dispatch", {sig::POW}, {}},
    // generic Array<T> reference (ETSTypeReference) routes through castAs,
    // unlike the native path of the T[] form (as_array_boxed_elem_native)
    {"x as Array<Int>", "as_generic_array_ref", {sig::GET, sig::CAST_AS}, {sig::TO_INT}},
    // consecutive casts compose through the castAs path
    {"x as MyOne as MyTwo", "as_consecutive", {sig::GET, sig::CAST_AS}},
    // KNOWN LIMITATION snapshot: the ETS parser produces ETSUnionType here,
    // which IsBuiltinTypeNode's whitelist (TS_UNION_TYPE only) does not
    // recognize, so the cast degrades to null as a "user-defined type".
    {"x as String | undefined", "as_ets_union_degrades", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},

    // Qualified names: dotted references route through runtime castAs.
    {"x as a.b.Foo", "as_qualified_name", {sig::GET, sig::CAST_AS}},
    {"x as std.core.String", "as_std_qualified_name", {sig::GET, sig::CAST_AS}},
    {"x as a.b.List<Int>", "as_qualified_generic_erased", {sig::GET, sig::CAST_AS}},

    // Array-type casts: bindable elements stay native; unbindable elements
    // erase to `as Array<Object>` (element types erased in the checkcast).
    {"x as Int[]", "as_array_boxed_elem_native", {sig::GET}, {sig::CAST_AS}},
    {"x as MyClass[]", "as_array_user_elem_erased", {sig::GET}, {sig::CAST_AS}},
    {"x as a.b.Foo[]", "as_array_qualified_elem_erased", {sig::GET}, {sig::CAST_AS}},
    // nested / generic / keyword / readonly elements stay native
    // (regression guards for the erasure logic)
    {"x as int[][]", "as_array_nested_bindable_native", {sig::GET}, {sig::CAST_AS}},
    {"x as Array<Int>[]", "as_array_generic_elem_native", {sig::GET}, {sig::CAST_AS}},
    {"x as undefined[]", "as_array_keyword_elem_native", {sig::GET}, {sig::CAST_AS}},
    {"x as readonly int[]", "as_readonly_bindable_native", {sig::GET}, {sig::CAST_AS}},
    // unbindable type argument: erasure (native would fail BIND on MyClass)
    {"x as Array<MyClass>[]", "as_array_generic_arg_user_erased", {sig::GET}, {sig::CAST_AS}},

    // readonly arrays with unbindable elements: erasure would checkcast the
    // wrong runtime class -- degrade gracefully.
    {"x as readonly MyClass[]", "as_readonly_array_rejected", {}, {"std.debug.DebuggerAPI."}},

    // instanceof.
    {"obj instanceof Obj", "instanceof_user_type", {sig::GET, sig::INSTANCEOF}},
    {"obj instanceof String", "instanceof_builtin", {sig::GET, sig::INSTANCEOF}},
    {"obj instanceof int", "instanceof_primitive", {sig::GET}, {sig::INSTANCEOF}},
    {"obj instanceof a.b.Foo", "instanceof_qualified_name", {sig::GET, sig::INSTANCEOF}},

    // new X().
    {"new Object()", "new_instance", {sig::NEW_INSTANCE}},
    {"new Array<Int>(5)", "newarray_class_ctor", {sig::NEW_INSTANCE}, {sig::ARRAY_CREATE}},
    {"new a.b.C()", "new_qualified_name", {sig::NEW_INSTANCE}},
    // Builtin fixed/value array creation kept native: newarr intrinsic via
    // FixedArrayLowering, zero DebuggerAPI calls for literal arguments
    {"new FixedArray<int>(10, 0)", "new_fixedarray_native", {}, {sig::NEW_INSTANCE, sig::CAST_AS, sig::TO_INT}},
    {"new FixedArray<int>(10)", "new_fixedarray_no_elem_native", {}, {sig::NEW_INSTANCE}},
    {"new ValueArray<double>(3, 7.0)", "new_valuearray_native", {}, {sig::NEW_INSTANCE}},
    {"new FixedArray<String>(3, \"a\")", "new_fixedarray_string_native", {}, {sig::NEW_INSTANCE}},
    // variable arguments bridge: len via toInt, elem via the element-type
    // bridge; still no newInstance dispatch
    {"new FixedArray<int>(n, e)", "new_fixedarray_bridged", {sig::GET, sig::TO_INT}, {sig::NEW_INSTANCE}},
    // builtin array type cast: native array-descriptor checkcast, no castAs
    {"x as FixedArray<int>", "as_fixedarray_native", {sig::GET}, {sig::CAST_AS}},
    {"x as ValueArray<double>", "as_valuearray_native", {sig::GET}, {sig::CAST_AS}},
    // constructor result chained into a method call
    {"new MyClass(a, b).method()",
     "new_then_method_call",
     {sig::NEW_INSTANCE, sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},

    // Array.create desugaring, boot classes only
    {"new int[5](0)", "newarray_int", {sig::ARRAY_CREATE}, {"std.debug.DebuggerAPI."}},
    {"new int[n](0)", "newarray_dim_var", {sig::GET, sig::TO_INT, sig::ARRAY_CREATE}},
    {"new int[5](fill)", "newarray_init_primitive", {sig::GET, sig::TO_INT, sig::ARRAY_CREATE}},
    {"new String[3](s)", "newarray_init_reference", {sig::GET, sig::ARRAY_CREATE}},
    {"new Object[3](o)", "newarray_init_object", {sig::GET, sig::ARRAY_CREATE}},

    // Conditional / nullish / optional chain.
    {"a ? b : c", "conditional_plain", {sig::GET}, {sig::ADD, sig::SUB, sig::PROXY_GET_FIELD, sig::WRAP}},
    {"flagOne ? (flagTwo ? valOne : valTwo) : valThree",
     "nested_conditional",
     {sig::GET},
     {sig::ADD, sig::SUB, sig::WRAP, sig::PROXY_GET_FIELD}},
    {"a ?? b", "nullish_coalesce", {sig::GET}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"a ?? b ?? c", "nullish_nested", {sig::GET}, {sig::ADD, sig::PROXY_GET_FIELD}},
    {"(a ?? b).foo()", "nullish_receiver_call", {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE}},
    // calls inside branches stay native (no settling, no lift)
    {"cond ? foo(i) : bar(i)", "conditional_call_branches", {sig::GET, sig::CALL_FUNCTION, sig::PROXY_VALUE}},
    // member writebacks settle INSIDE the taken branch (LiftConditionalBranches)
    {"cond ? (a.x = 1) : (a.y = 2)",
     "conditional_member_writeback",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE},
     {sig::SET_ELEMENT}},
    {"a?.b", "optional_chain", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    {"a?.b()", "optional_call", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_INVOKE, sig::PROXY_VALUE}},
    // short-circuit extends through NON-optional chain segments
    {"a?.b.c", "optional_chain_extends", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    // optional call carrying arguments
    {"a?.b(c)",
     "optional_call_with_args",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_INVOKE, sig::PROXY_VALUE}},
    // subscript after the optional segment; element-access base before it
    {"a?.b[0]",
     "optional_then_subscript",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::PROXY_CALL}},
    {"arr[i]?.foo",
     "subscript_then_optional",
     {sig::GET, sig::TO_INT, sig::WRAP, sig::PROXY_CALL, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    // consecutive optional segments (defensive navigation): one null
    // checked temp per optional link
    {"objAlpha?.fieldOne?.fieldTwo",
     "optional_consecutive",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE},
     {sig::ADD, sig::SET}},
    {"objAlpha.fieldOne?.fieldTwo",
     "optional_mid_chain",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    // optional method link chained into a member read of the result
    {"objAlpha?.methodOne()?.resultField",
     "optional_method_chain",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::PROXY_INVOKE}},
    {"targetVar = flagOne ? valOne : valTwo",
     "assign_conditional_rhs",
     {sig::GET, sig::SET},
     {sig::ADD, sig::PROXY_GET_FIELD}},

    // Array-literal spreads: unknown-typed arguments bridge to Array<Object>
    // (the native spread loop operates on Array.$_get regardless of element type).
    {"[1, ...arr]", "spread_literal_identifier", {sig::GET, sig::ARRAY_GET}},
    {"[...this]", "spread_literal_this", {sig::GET_THIS, sig::ARRAY_GET}},
    {"[...a.b]",
     "spread_literal_member",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ARRAY_GET}},
    {"[...foo()]", "spread_literal_call", {sig::CALL_FUNCTION, sig::PROXY_VALUE, sig::ARRAY_GET}},
    {"[...arr[0]]",
     "spread_literal_elemaccess",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::ARRAY_GET}},
    {"[...(x as Array<Int>)]", "spread_literal_castas_array", {sig::GET, sig::CAST_AS, sig::ARRAY_GET}},
    // native spread semantics must be preserved (wrongful bridging would
    // flip String.$_get to Array.$_get)
    {"[...\"abc\"]", "spread_string_native", {sig::STRING_GET}, {sig::ARRAY_GET, "std.debug.DebuggerAPI."}},
    {"[...[1, 2]]", "spread_nested_literal_native", {sig::ARRAY_GET}, {"std.debug.DebuggerAPI."}},
    {"[...new int[3](0)]", "spread_newarray_native", {sig::ARRAY_CREATE}, {"std.debug.DebuggerAPI."}},
    {"[...typeof x]", "spread_typeof_native", {sig::GET, sig::TYPEOF, sig::STRING_GET}, {sig::ARRAY_GET}},

    // graceful degrade to null (valid ABC, no calls)
    {"await p", "unsupported_await", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"[]", "unsupported_empty_array", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"({a:1})", "unsupported_object_literal", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // destructuring assignment (array form, identifier elements + holes):
    // expands to a single RHS snapshot (get) plus per-element $_get reads and
    // set writebacks; the value is the RHS. Presence-only signature checks
    // (string-pool semantics); element multiplicity is covered by e2e tests.
    {"[xOne, yTwo] = srcArr",
     "destructuring_identifiers",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::SET},
     {sig::ADD}},
    // hole: the skipped element contributes no read/writeback
    {"[xOne, , yTwo] = srcArr",
     "destructuring_hole",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::SET},
     {sig::ADD}},
    // complex RHS: member call snapshot precedes the element reads
    {"[xOne, yTwo] = c.getArr()",
     "destructuring_member_rhs",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, sig::SET},
     {}},
    // empty pattern: value = RHS, zero writebacks
    {"[] = srcArr", "destructuring_empty", {sig::GET}, {sig::SET, sig::PROXY_CALL}},
    // rejected element forms degrade to null (valid ABC, no calls), matching
    // the normal pipeline's REST/DEFAULT/NESTED destructuring diagnostics
    {"[xOne, ...rest] = srcArr",
     "unsupported_destructuring_rest",
     {},
     {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"[xOne = 1] = srcArr",
     "unsupported_destructuring_default",
     {},
     {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    {"[xOne, [yTwo]] = srcArr",
     "unsupported_destructuring_nested",
     {},
     {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // object form remains unsupported (ObjectPattern LHS, no expansion)
    {"({xOne: tgtOne} = srcObj)",
     "unsupported_destructuring_object",
     {},
     {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // Wrapper-injection snapshot: '}' closes runtime_evaluate early and the
    // rest parses as a top-level 'evil' function that lands in the artifact.
    // The evaluation path itself stays clean (first function declaration,
    // literal result, no API calls); evil is never invoked. Snapshot guards
    // this contract -- a change to either side should be a conscious one.
    {"0; } function evil() { return 1",
     "wrapper_injection_snapshot",
     {},
     {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // only the lambda node degrades; the call chain is preserved
    {"arr.filter(x => x > threshold)",
     "unsupported_lambda_capture",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE},
     {"%%lambda", "registerEvalModule"}},
    // the lambda callee degrades to null; the invoke machinery remains
    {"((n: int): int => { let s: int = 0; for (let i: int = 0; i < n; i++) { s += i } return s })(5)",
     "unsupported_lambda_iife",
     {sig::WRAP, sig::PROXY_INVOKE, sig::PROXY_VALUE},
     {"%%lambda", "registerEvalModule"}},
    {"x = arr.filter(v => v > threshold)",
     "unsupported_lambda_in_assignment",
     {sig::SET},
     {"%%lambda", "registerEvalModule"}},
    // user-defined element types: erased to Object (bytecode-identical
    // to the native lowering of either element type)
    {"new MyClass[3](o)", "newarray_user_elem_erased", {sig::GET, sig::ARRAY_CREATE}},
    // null initializer on an erased element type: erasure targets Any so the
    // null fill value stays assignable (Object would fail ESE0046)
    {"new MyClass[3](null)", "newarray_user_elem_null_init", {sig::ARRAY_CREATE}, {sig::GET}},
    {"new MyClass[n](i)", "newarray_user_elem_dim_var", {sig::GET, sig::TO_INT, sig::ARRAY_CREATE}},
    {"new a.b.Foo[3](o)", "newarray_qualified_elem_erased", {sig::GET, sig::ARRAY_CREATE}},
    {"new int[3](i => i * 2)", "newarray_lambda_rejected", {}, {sig::ARRAY_CREATE}},

    // Parameter-level verification: API name arguments surface as string
    // pool entries, so distinctive long names assert WHAT is addressed,
    // not only which API is dispatched.
    {"longVarNameA1.longFieldB2.longFieldC3",
     "param_member_chain",
     {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, "longVarNameA1", "longFieldB2", "longFieldC3"}},
    {"longVarNameA1.longMethodD4()",
     "param_method_call",
     {sig::GET, sig::WRAP, sig::PROXY_CALL, sig::PROXY_VALUE, "longVarNameA1", "longMethodD4"}},
    {"longVarNameA1 as LongKlassE5", "param_cast_class", {sig::GET, sig::CAST_AS, "LongKlassE5"}},
    {"new LongKlassF6()", "param_new_class", {sig::NEW_INSTANCE, "LongKlassF6"}},
    {"longVarNameA1 = 7", "param_set_name", {sig::SET, "longVarNameA1"}, {sig::GET}},
    {"longArrNameG7[0] = 9", "param_set_element_name", {sig::SET_ELEMENT, "longArrNameG7"}, {sig::GET}},
    {"longFuncH8()", "param_call_function", {sig::CALL_FUNCTION, sig::PROXY_VALUE, "longFuncH8"}, {sig::GET}},
    {"super.longMethodI9()",
     "param_super_method",
     {sig::CALL_SUPER, sig::PROXY_VALUE, "longMethodI9"},
     {sig::GET, sig::WRAP}},
    {"`result=${valueVar} finished`", "param_template_quasis", {sig::GET, sig::ADD, "result=", " finished"}},

    // Multi-statement input: ExtractExpressionFromAst keeps ONLY the last
    // statement, so earlier side effects are dropped (behavior snapshot:
    // "x = 1" emits no set at all).
    {"x = 1; x + 1", "multi_statement_last_wins", {sig::GET, sig::ADD}, {sig::SET}},

    // let/const declarations: one defineVariable per declarator, the value
    // argument is the raw temp (no Object cast: null must stay assignable),
    // and the result is the last declarator's value.
    {"let a = 1", "let_single", {sig::DEFINE_VARIABLE}, {sig::GET, sig::SET, sig::PROXY_GET_FIELD, "as Object"}},
    {"const b = 2", "const_single", {sig::DEFINE_VARIABLE}, {sig::GET, sig::SET}},
    // multi declarator: b's initializer references a via the temp (pendingDecls
    // direct binding), so no get() for "a" -- only the add dispatch
    {"let a = 1, b = a + 1", "let_multi_declarator", {sig::DEFINE_VARIABLE, sig::ADD}, {sig::GET, sig::SET}},
    // null initializer passes the temp raw to the Any-typed parameter
    {"let a = null", "let_null_init", {sig::DEFINE_VARIABLE}, {sig::GET, sig::SET, "as Object"}},
    // type annotation ignored in v1 (runtime-typed store)
    {"let x: int = 5", "let_typed", {sig::DEFINE_VARIABLE}, {sig::GET, sig::SET}},
    // wrapper parameter names stay bindable: the declared name only ever
    // becomes a string literal, never an eval-function local
    {"let thread = 1", "let_param_name", {sig::DEFINE_VARIABLE}, {sig::GET, sig::SET}},
    // destructuring DECLARATION parses, so the transformer must reject it
    // (graceful degrade: valid ABC, no calls)
    {"let [p, q] = [1, 2]", "let_destructuring_degrades", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},
    // Wrapper-injection snapshot (declaration flavor of the expression-mode
    // wrapper_injection_snapshot below): '}' closes runtime_evaluate early,
    // the rest parses as a top-level 'evil' function that lands in the
    // artifact but is never invoked. The evaluation path itself stays clean
    // (single declaration, defineVariable registration, literal return).
    {"let a = 1 } function evil() { return 1",
     "let_wrapper_injection_snapshot",
     {sig::DEFINE_VARIABLE},
     {sig::GET, sig::SET}},
    {"a; b", "multi_statement_plain", {sig::GET}, {sig::ADD, sig::SET}},
    // nullish guarded comparison (threshold-check idiom)
    {"(x ?? 0) < 5", "nullish_guarded_compare", {sig::GET, sig::LT}, {sig::ADD, sig::PROXY_GET_FIELD}},
    // typeof over a call result
    {"typeof foo()", "typeof_call", {sig::CALL_FUNCTION, sig::PROXY_VALUE, sig::TYPEOF}, {sig::GET}},
    // mixed Int box + BigInt literal arithmetic (runtime add dispatches by
    // instanceof; the emitted call is the same add)
    {"x + 10n", "mixed_bigint_literal", {sig::GET, sig::ADD}},
    // quasi-only template literal: a plain StringLiteral, zero dispatch
    {"`plain text`", "template_quasi_only", {}, {"std.debug.DebuggerAPI.", "std.debug.DebugProxy."}},

    // Combined shapes.
    {"a.x + b.y", "complex_member_arith", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE, sig::ADD}},
    // manual narrowing idiom (no smart-cast in evaluate): instanceof guard +
    // castAs + member read
    {"x instanceof MyObj ? (x as MyObj).field : null",
     "narrowing_pattern",
     {sig::GET, sig::INSTANCEOF, sig::CAST_AS, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_VALUE}},
    {"a + b * c", "complex_precedence", {sig::GET, sig::ADD, sig::MUL}},
    {"true?i:(i=99)", "cond_assign_const_test", {sig::GET, sig::SET}},
    {"cond?(i=2):(i=1)", "cond_assign_both", {sig::GET, sig::SET}},
    {"(cond?(i=1):i)+1", "cond_in_arith", {sig::GET, sig::ADD, sig::SET}},
    {"-(cond?(i=1):i)", "cond_in_unary", {sig::GET, sig::NEG, sig::SET}},
    {"(cond?(i=1):i) as int", "cond_in_cast", {sig::GET, sig::TO_INT, sig::SET}},
    {"foo(cond?(i=1):i)", "cond_in_args", {sig::CALL_FUNCTION, sig::GET, sig::SET}},
    {"(a??b).x = 5", "nullish_member_lhs", {sig::GET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE}},
    {"(cond?(x=1):y).f = 5",
     "cond_member_lhs",
     {sig::GET, sig::SET, sig::WRAP, sig::PROXY_GET_FIELD, sig::PROXY_SET_VALUE}},
};

int RunTests(es2panda_Impl *impl, es2panda_Config *config)
{
    int failed = 0;
    for (const auto &tc : K_TEST_CASES) {
        if (!TestEvaluateExpression(impl, config, tc)) {
            failed++;
        }
    }
    return failed;
}

}  // namespace

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }
    if (GetImpl() == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }
    es2panda_Impl *impl = GetImpl();
    impl->MemInitialize();

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    if (config == nullptr) {
        std::cerr << "FAILED TO CREATE CONFIG" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }

    int failed = RunTests(impl, config);
    failed += RunUnsupportedCases(impl, config);

    {
        char *result = impl->EvaluateExpression(config, nullptr);
        if (result != nullptr) {
            std::cerr << "[null_input] FAILED: expected nullptr" << std::endl;
            impl->FreeExpressionResult(result);
            failed++;
        }
    }

    // CAPI entry robustness: malformed base64 and empty input must fail
    // cleanly (nullptr) without touching the compilation pipeline.
    {
        const char *kBadInputs[] = {"!!!not-base64!!!", "abcde", ""};
        for (const char *bad : kBadInputs) {
            char *result = impl->EvaluateExpression(config, bad);
            if (result != nullptr) {
                std::cerr << "[bad_input] FAILED: expected nullptr for '" << bad << "'" << std::endl;
                impl->FreeExpressionResult(result);
                failed++;
            } else {
                std::cout << "[bad_input '" << bad << "'] PASS (returned null as expected)" << std::endl;
            }
        }
    }

    impl->DestroyConfig(config);
    impl->MemFinalize();

    if (failed > 0) {
        std::cerr << failed << " test(s) FAILED" << std::endl;
        return TEST_ERROR_CODE;
    }
    std::cout << "All tests PASSED" << std::endl;
    return 0;
}
