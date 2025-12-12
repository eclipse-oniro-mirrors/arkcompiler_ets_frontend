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

const GeneratorFunction = Object.getPrototypeOf(function*(){}).constructor;
const AsyncGeneratorFunction = Object.getPrototypeOf(async function*(){}).constructor;
const AsyncFunction = Object.getPrototypeOf(async function(){}).constructor;
const NormalFunction = Object.getPrototypeOf(function(){}).constructor;

function check(name, fn) {
    print(`--- ${name} ---`);
    print(`Is GeneratorFunction? ${fn instanceof GeneratorFunction}`);
    print(`Is AsyncGeneratorFunction? ${fn instanceof AsyncGeneratorFunction}`);
    print(`Is AsyncFunction? ${fn instanceof AsyncFunction}`);
    print(`Is NormalFunction? ${fn instanceof NormalFunction}`);
    print(`Constructor Name: ${fn.constructor.name}`);
    print(`Prototype Chain Name: ${Object.getPrototypeOf(fn).constructor.name}`);
    print('');
}

// 1. Generator Declaration
function* genDecl() {}
check('Generator Declaration', genDecl);

// 2. Async Generator Declaration
async function* asyncGenDecl() {}
check('Async Generator Declaration', asyncGenDecl);

// 3. Class Methods
class MyClass {
    *genMethod() {}
    async *asyncGenMethod() {}
    async asyncMethod() {}
    normalMethod() {}
}

const instance = new MyClass();
check('Class Generator Method', instance.genMethod);
check('Class Async Generator Method', instance.asyncGenMethod);
check('Class Async Method', instance.asyncMethod);
check('Class Normal Method', instance.normalMethod);