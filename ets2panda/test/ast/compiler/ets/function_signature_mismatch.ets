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

function returnThisMember(this: { member: string }) {
    return this.member;
}

interface Container {
    member: string;
    returnThisMember: () => string;
}

let container: Container;
container = {
    member: "sample",
    returnThisMember: returnThisMember,
};

function main() {
    container.returnThisMember();
}

/* @@? 16:1 Error Semantic error ESE0082: Extension function can only defined for class, interface or array. */
/* @@? 16:33 Error Syntax error ESY177354: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 17:12 Error Semantic error ESE0203: Cannot reference 'this' in this context. */
/* @@? 26:13 Error Semantic error ESE0400: Non-optional property 'returnThisMember' in type 'Container' is missing in object literal. */
/* @@? 28:23 Error Semantic error ESE0319: Type '((p1: *ERROR_TYPE*) => void)' is not compatible with type '(() => String)' at property 'returnThisMember' */
