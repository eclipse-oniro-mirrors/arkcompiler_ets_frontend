/*
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

// Focused test case for BrokenTypeNode::Clone() implementation
// This test specifically exercises the BrokenTypeNode cloning that was causing
// the "ASSERTION FAILED: clonedNode != typeAliasNode->TypeAnnotation()" crash

// This creates BrokenTypeNode instances during parsing due to invalid TypeScript syntax
type BrokenType<K extends keyof UndefinedMap> = {
    kind: K
};

// Multiple instances to test different patterns
type AnotherBroken<T extends keyof NonExistent> = T;
type YetAnother<U extends InvalidInterface> = U;

// Nested broken types
type NestedBroken<K extends keyof UndefinedType> = {
    nested: K,
    value: UndefinedType[K]
};

// Function with broken parameter types
type BrokenFunc<T extends UndefinedInterface> = (param: T) => void;

// Test instantiation to trigger ValidateGenericTypeAliasForClonedNode
declare const broken1: BrokenType<any>;
declare const broken2: AnotherBroken<any>;

/* @@? 21:33 Error TypeError: Cannot find type 'UndefinedMap'. */
/* @@? 21:45 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 21:49 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 26:36 Error TypeError: Cannot find type 'NonExistent'. */
/* @@? 26:47 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 27:27 Error TypeError: Cannot find type 'InvalidInterface'. */
/* @@? 30:52 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 31:14 Error SyntaxError: Unexpected token ','. */
/* @@? 32:26 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 32:26 Error SyntaxError: Unexpected token ']'. */
/* @@? 32:27 Error SyntaxError: Unexpected token ']'. */
/* @@? 36:27 Error TypeError: Cannot find type 'UndefinedInterface'. */
/* @@? 39:35 Error TypeError: Cannot find type 'any'. */
/* @@? 40:38 Error TypeError: Cannot find type 'any'. */
