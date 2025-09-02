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

// Comprehensive test case for TypeNode::Clone() implementations
// This test covers all TypeNode subclasses that were missing Clone implementations
// and caused assertion failures in ValidateGenericTypeAliasForClonedNode

// Test 1: TSTypeLiteral Clone
type ObjectType = { n: number, s: string, b: boolean };

// Test 2: TSNumberKeyword, TSStringKeyword, TSBooleanKeyword Clone
type NumType = number;
type StrType = string;
type BoolType = boolean;

// Test 3: TSVoidKeyword Clone (in function types)
type VoidFunc = () => void;

// Test 4: TSTypeOperator Clone (keyof operations)
type KeyofType<T extends keyof ObjectType> = T;

// Test 5: TSIndexedAccessType Clone
type IndexedType<K extends keyof ObjectType> = ObjectType[K];

// Test 6: TSTupleType Clone
type TupleType = [a: number, b: string];

// Test 7: TSMappedType Clone
type MappedType = { [K in keyof ObjectType]: string };

// Test 8: TSFunctionType Clone
type FuncType<K extends keyof ObjectType> = (v: ObjectType[K]) => void;

// Test 9: TSLiteralType Clone (string literals)
type LiteralType<K extends 'n' | 's' | 'b'> = K;

// Test 10: TSUnionType Clone
type UnionType = string | number | boolean;

// Test 11: BrokenTypeNode Clone (created for invalid syntax)
// This will create BrokenTypeNode instances that need proper cloning
type InvalidType<K extends keyof InvalidMap> = K;

// Test 12: Complex nested type with multiple Clone implementations
type ComplexType<K extends keyof ObjectType = keyof ObjectType> = {
    kind: K,
    value: ObjectType[K],
    handler: (v: ObjectType[K]) => void,
    tuple: [K, ObjectType[K]],
    mapped: { [P in K]: ObjectType[P] }
};

// Test instantiation to trigger ValidateGenericTypeAliasForClonedNode
declare const test1: ComplexType<'n'>;
declare const test2: ComplexType<'s'>;
declare const test3: ComplexType<'b'>;

/* @@? 1:3 Error TypeError: Class 'TupleType' is already defined with different type. */
/* @@? 1:3 Error TypeError: Class 'UnionType' is already defined with different type. */
/* @@? 21:19 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 21:30 Error SyntaxError: Unexpected token ','. */
/* @@? 21:41 Error SyntaxError: Unexpected token ','. */
/* @@? 32:42 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 35:44 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 35:59 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 35:59 Error SyntaxError: Unexpected token ']'. */
/* @@? 35:60 Error SyntaxError: Unexpected token ']'. */
/* @@? 38:19 Error TypeError: Cannot find type 'a'. */
/* @@? 38:20 Error SyntaxError: Unexpected token ':'. */
/* @@? 38:20 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 38:22 Error SyntaxError: Unexpected token 'number'. */
/* @@? 38:22 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 38:28 Error SyntaxError: Unexpected token ','. */
/* @@? 38:30 Error SyntaxError: Unexpected token 'b'. */
/* @@? 38:33 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 38:33 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 38:39 Error SyntaxError: Unexpected token ']'. */
/* @@? 41:19 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 41:22 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 41:26 Error SyntaxError: Field type annotation expected. */
/* @@? 41:32 Error SyntaxError: Field type annotation expected. */
/* @@? 41:43 Error SyntaxError: Unexpected token ']'. */
/* @@? 41:43 Error SyntaxError: Field type annotation expected. */
/* @@? 41:44 Error SyntaxError: Unexpected token ':'. */
/* @@? 41:46 Error SyntaxError: string is a predefined type, cannot be used as an identifier  */
/* @@? 44:41 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 44:60 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 54:34 Error TypeError: Cannot find type 'InvalidMap'. */
/* @@? 54:44 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 57:45 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 57:63 Error TypeError: The `keyof` keyword can only be used for class or interface type. */
/* @@? 57:67 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 58:12 Error SyntaxError: Unexpected token ','. */
/* @@? 59:23 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 59:23 Error SyntaxError: Unexpected token ']'. */
/* @@? 59:24 Error SyntaxError: Unexpected token ']'. */
/* @@? 59:25 Error SyntaxError: Unexpected token ','. */
/* @@? 60:29 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 60:40 Error SyntaxError: Unexpected token ','. */
/* @@? 61:27 Error SyntaxError: Unexpected token, expected ']'. */
/* @@? 61:28 Error SyntaxError: Unexpected token ']'. */
/* @@? 61:29 Error SyntaxError: Unexpected token ']'. */
/* @@? 61:30 Error SyntaxError: Unexpected token ','. */
/* @@? 62:13 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 62:16 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 62:20 Error SyntaxError: Field type annotation expected. */
/* @@? 62:22 Error SyntaxError: Unexpected token ']'. */
/* @@? 62:22 Error SyntaxError: Field type annotation expected. */
/* @@? 62:23 Error SyntaxError: Unexpected token ':'. */
/* @@? 62:35 Error SyntaxError: Field type annotation expected. */
/* @@? 62:36 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 62:37 Error SyntaxError: Unexpected token ']'. */
