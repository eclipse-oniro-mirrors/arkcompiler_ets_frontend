/*
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

// Tests constant folding of template literals in ConstantExpressionLowering
// (constantExpressionLowering.cpp, NodeCalculator::Calculate(ir::TemplateLiteral*)):
// every literal kind accepted as an interpolation input must be folded into the
// resulting string with the same value the runtime would produce.
//
// Spec reference: plugins/ets/doc/spec_700/7_expressions.rst, "String Interpolation
// Expressions": an embedded expression must be of type string, otherwise the implicit
// conversion to string takes place in the same way as with the string concatenation
// operator (see "String Operator Contexts" in 6_conversions.rst).

function foldEachLiteralKind() {
    // char literal input
    arktest.assertEQ(`${c'a'}`, "a");
    // number literal inputs
    arktest.assertEQ(`${42}`, "42");
    arktest.assertEQ(`${1.5}`, "1.5");
    arktest.assertEQ(`${-7}`, "-7");
    // boolean literal input
    arktest.assertEQ(`${true}`, "true");
    arktest.assertEQ(`${false}`, "false");
    // string literal input
    arktest.assertEQ(`${"str"}`, "str");
    // bigint literal input: no trailing 'n' suffix, matches BigInt.toString()
    arktest.assertEQ(`${1n}`, "1");
    arktest.assertEQ(`${-12n}`, "-12");
    arktest.assertEQ(`${123456789012345678901234567890n}`, "123456789012345678901234567890");
}

function foldMultipleInputsAndQuasis() {
    // several interpolation inputs mixed with quasi strings
    arktest.assertEQ(`i=${42},b=${true},c=${c'x'},s=${"s"},n=${1n}`, "i=42,b=true,c=x,s=s,n=1");
    // leading, middle and trailing quasis around a bigint input
    arktest.assertEQ(`a${1n}b`, "a1b");
    // constant expression folded to a bigint before template folding
    arktest.assertEQ(`${true ? 1n : 2n}`, "1");
    arktest.assertEQ(`${false ? 1n : 2n}`, "2");
    // empty quasi parts: only the folded bigint value remains
    arktest.assertEQ(`${7n}`, "7");
    // multiple embedded expressions in a row without separating quasis
    arktest.assertEQ(`${1}${2n}${true}`, "12ntrue".replace("n", ""));
}

// Spec: the result of each embedded expression substitutes that embedded
// expression; embedded expressions are arbitrary expressions, including
// arithmetic ones (spec example: `The result of ${a} * ${b} is ${a * b}`).
function foldEmbeddedExpressions() {
    // spec 7_expressions.rst example, constant operands
    let a = 2;
    let b = 2;
    arktest.assertEQ(`The result of ${a} * ${b} is ${a * b}`, "The result of 2 * 2 is 4");
    // fully constant equivalent must fold to the same string
    arktest.assertEQ(`The result of ${2} * ${2} is ${2 * 2}`, "The result of 2 * 2 is 4");
    // string concatenation inside an embedded expression
    arktest.assertEQ(`${"foo" + "bar"}`, "foobar");
    // numeric constant expression inside an embedded expression
    arktest.assertEQ(`${(6 * 7) / 2}`, "21");
    // conditional (ternary) embedded expression with constant condition
    arktest.assertEQ(`${true ? "yes" : "no"}`, "yes");
    arktest.assertEQ(`${false ? "yes" : "no"}`, "no");
}

// Spec: an embedded expression can contain nested multiline strings.
function nestedMultilineStrings() {
    // nested template literal inside an embedded expression
    arktest.assertEQ(`outer ${`inner ${1n}`} end`, "outer inner 1 end");
    // nested template with its own embedded expression, both constant
    arktest.assertEQ(`a${`b${`c${1n}`}`}d`, "abc1d");
    // nested template as a non-constant operand
    let x = 5n;
    arktest.assertEQ(`v=${`n=${x}`}`, "v=n=5");
}

// Spec (6_conversions.rst, "String Operator Contexts"): implicit conversion of
// a non-string operand follows the same rules as string concatenation:
// integer in decimal form, floating-point without loss of information,
// boolean as true/false, null as "null", undefined as "undefined",
// enumeration member value, and toString() for reference types.
function conversionSemantics() {
    // integer operand in decimal form
    arktest.assertEQ(`${42}`, "42");
    arktest.assertEQ(`${0}`, "0");
    // floating-point operand without loss of information
    arktest.assertEQ(`${2.5}`, "2.5");
    arktest.assertEQ(`${0.1 + 0.2}`, `${0.1 + 0.2}`);
    // bigint operand matches the spec example: `"BigInt is " + 123n` prints "BigInt is 123"
    arktest.assertEQ(`BigInt is ${123n}`, "BigInt is 123");
    // consistency: template folding and the string concatenation operator
    // produce identical strings for the same operand
    arktest.assertEQ(`BigInt is ${123n}`, "BigInt is " + 123n);
    arktest.assertEQ(`value is ${123}`, "value is " + 123);
    arktest.assertEQ(`${15} steps`, 15 + " steps");
    // nullish values converted per spec (runtime, not constant-folded)
    let x: string | null | undefined = null;
    arktest.assertEQ(`string is ${x}`, "string is null");
    x = undefined;
    arktest.assertEQ(`string is ${x}`, "string is undefined");
}

// Spec (2_lexical.rst, "Multiline String Literal"): the multiline string used as
// a template can contain escape sequences; a backslash escapes the next
// character; the end of a line is handled as a newline character and leading
// spaces are neither squeezed nor trimmed.
function multilineQuasisAndEscapes() {
    // escaped backtick inside a quasi
    arktest.assertEQ(`\`${1n}\``, "`1`");
    // escaped dollar-brace is not an embedded expression (constant folding must keep it verbatim)
    arktest.assertEQ(`\${14/2} = ${14/2}`, "${14/2} = 7");
    // escape sequences inside quasis survive folding
    arktest.assertEQ(`a\tb${1n}c\\d`, "a\tb1c\\d");
    // unicode escape inside a quasi
    arktest.assertEQ(`\u0041${1n}`, "A1");
    // a newline inside a quasi is preserved by folding
    arktest.assertEQ(`a
b${1n}`, "a\nb1");
}

// Spec (7_expressions.rst, "Ternary Conditional Expression"): if the condition
// value is known at compile time, only the taken branch participates in the
// result type and value.
function conditionalEmbeddedExpressions() {
    // both branches bigint, constant condition: folds to the taken branch
    arktest.assertEQ(`${true ? 1n : 2n}`, "1");
    arktest.assertEQ(`${false ? 1n : 2n}`, "2");
    // constant condition selecting between different literal kinds
    arktest.assertEQ(`${true ? 1 : "s"}`, "1");
    arktest.assertEQ(`${false ? 1 : "s"}`, "s");
    // non-constant condition: interpolated at runtime
    let cond = 1 < 2;
    arktest.assertEQ(`${cond ? 1n : 2n}`, "1");
}

// Spec: template literal is of type string; non-constant embedded expressions
// are converted at runtime with the same semantics as compile-time folding.
function runtimeInterpolationUnaffected() {
    let a = 3n;
    let cond = false;
    let b = cond ? 1n : 45n;
    arktest.assertEQ(`x${a}y`, "x3y");
    arktest.assertEQ(`${b}`, "45");
    // mixed constant/non-constant inputs: folding stops, runtime assembles the whole string
    let n = 10;
    arktest.assertEQ(`n=${n},k=${1n}`, "n=10,k=1");
    // arithmetic embedded expression evaluated at runtime
    let m = 6;
    arktest.assertEQ(`${m} * 7 = ${m * 7}`, "6 * 7 = 42");
    // toString() of a reference-type operand at runtime
    let obj = new BigInt(9n);
    arktest.assertEQ(`${obj}`, "9");
}

function main() {
    foldEachLiteralKind();
    foldMultipleInputsAndQuasis();
    foldEmbeddedExpressions();
    nestedMultilineStrings();
    conversionSemantics();
    multilineQuasisAndEscapes();
    conditionalEmbeddedExpressions();
    runtimeInterpolationUnaffected();
}
