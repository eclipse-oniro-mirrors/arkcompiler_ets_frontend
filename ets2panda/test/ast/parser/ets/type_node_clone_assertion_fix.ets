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

// Minimal test case for TypeNode::Clone() fix
// Previously caused: ASSERTION FAILED: clonedNode != typeAliasNode->TypeAnnotation() 
// in ValidateGenericTypeAliasForClonedNode at validateHelpers.cpp:242

// This specific pattern triggered the assertion failure because TypeNode::Clone()
// was returning 'this' instead of creating a proper clone when TsType() was nullptr
type MyMap<K extends keyof RecordMap> = {
    kind: K
};

type RecordMap = { n: number };

// Expected compilation errors for unsupported TypeScript features in ETS:
/* @@? 22:37 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 22:41 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 26:18 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */