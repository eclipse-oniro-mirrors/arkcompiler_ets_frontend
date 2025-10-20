/**
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

#include <cstddef>
#include <iostream>
#include <ostream>
#include <string>

#include "public/es2panda_lib.h"
#include "util.h"

// NOLINTBEGIN

static es2panda_Impl *impl = nullptr;

static std::string g_source = R"(
'use static'
/**
 * ==== import specifier jsdoc1 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
import { PI as PI, E } from "std/math/consts"

/**
 * ==== import specifier jsdoc2 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
import type E from "std/math/consts"

/**
 * ==== export specifier jsdoc1 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export { JsDocClassOutside, jsdocVal1, jsDocFunc }

/**
 * ==== export specifier jsdoc2 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export { PI, E } from "std/math/consts"

/**
 * ==== export specifier jsdoc3 ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export default jsdocVal2

/**
 * ==== export Annotation jsdoc ====
 * @param1 preStr { p } behindStr
 * @param2 preStr {} behindStr
*/
export declare @interface exportAnno {}

/**
 * ==== export declare class A ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export declare class A {
    /**
    * ==== classFoo ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    public classFoo():void;

    /**
    * ==== private optional classProp ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    private classProp?:number;

    /**
    * ==== test class getter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    get testGet():number;

    /**
    * ==== test class setter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    set testSet(n: number);

    /**
    * ==== ambient indexer jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    [idx: number] : string;
}

/**
 * ==== export declare struct myStruct ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
export declare struct myStruct {
  /**
  * ==== private optional Prop ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  Prop?:number;
}

/**
 * ==== export declare interface I ====
 * @param1 {} behindStr
 * @param2 preStr { p }
 */
export declare interface I {
    /**
    * ==== interfaceFoo jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    default interfaceFoo():void

    /**
    * ==== interfaceProp jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceProp: [
        a,
        b
    ]

    /**
    * ==== arrowFunc jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceArrowFunc: () => void;

    /**
    * ==== interface getter jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    get intefaceGet(): number

    /**
    * ==== interface setter jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    set intefaceSet(n: number)
}

/**
* ==== JsdocNS ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
declare namespace JsdocNS {
  /**
  * ==== JsdocInterface ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  export interface JsdocInterface {
    /**
    * ==== interfaceFoo ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceFoo():void

    /**
    * ==== private interfaceProp1 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    private interfaceProp1:number

    /**
    * ==== interfaceProp2 ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    interfaceProp2:number | string[]
  }

  /**
  * ==== JsDocClass ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  class JsDocClass {
    /**
    * ==== classFoo ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    public classFoo():void

    /**
    * ==== private classProp ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    private classProp:number;

    /**
    * ==== test class getter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    get testGet():number

    /**
    * ==== test class setter ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    set testSet(n: number)
  }

  /**
  * ==== JsDocStruct ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  struct JsDocStruct {
    /**
    * ==== private Prop ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    private Prop:number;
  }

  /**
  * ==== JsDocEnum1 ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  export enum E1 {
    A1 = 0,
    B1 = 0x0001,
    C1 = 0x0002,
    D1 = 0xFFFF
  }

  /**
  * ==== JsDocEnum2 ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  export enum E2 {
    A2 = 1,
    B2 = 2,
    C2 = 3,
    D2
  }

  /**
  * ==== JsDocEnum3 ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  export enum E3 {
    A3 = 1 << 0,
    B3 = 1 << 1,
    C3 = 1 << 2,
    D3 = 1 << 3
  }

  /**
  * ==== function jsdoc ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  */
  function foo(): void;

  /**
  * ==== JsdocInnerNS ====
  * @param1 {} behindStr
  * @param2 preStr { p }
  * @param3 preStr { p } behindStr
  */
  namespace JsdocInnerNS {}
}

/**
* ==== function decl jsdoc ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
function jsDocFunc(
    /**
    * ==== function param p1 jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    fooP1: number,

    /**
    * ==== function param p2 jsdoc ====
    * @param1 {} behindStr
    * @param2 preStr { p }
    */
    fooP2: string): void {}

/**
* ==== typeAlas with generic jsdoc ====
* @param1 {} behindStr
* @param2 preStr { p }
*/
export type typeAlias<T extends Error = BusinessError> = (err: T) => void;

/**
 * ==== variable decl ====
 * @param1 {} behindStr
 * @param2 preStr { p }
 */
export const jsdocVal1:string = "ssss"

/**
 * ==== function overload declaration jsdoc ====
 * @param1 {} behindStr
 * @param2 preStr { p }
*/
overload jsDocFunc {
  jsDocFunc1,
  jsDocFunc2,
  jsDocFunc3
}
)";

int main(int argc, char **argv)
{
    if (argc < MIN_ARGC) {
        return INVALID_ARGC_ERROR_CODE;
    }

    impl = GetImpl();
    if (impl == nullptr) {
        return NULLPTR_IMPL_ERROR_CODE;
    }

    const char **args = const_cast<const char **>(&(argv[1]));
    auto config = impl->CreateConfig(argc - 1, args);
    auto context = impl->CreateContextFromString(config, g_source.data(), argv[argc - 1]);
    if (context == nullptr) {
        std::cerr << "FAILED TO CREATE CONTEXT" << std::endl;
        return NULLPTR_CONTEXT_ERROR_CODE;
    }
    impl->ProceedToState(context, ES2PANDA_STATE_PARSED);
    CheckForErrors("PARSE", context);
    auto *program = impl->ContextProgram(context);
    auto *entryAst = impl->ProgramAst(context, program);
    std::cout << impl->AstNodeDumpEtsSrcWithJsdocConst(context, entryAst) << std::endl;
    return 0;
}

// NOLINTEND0