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

export declare namespace B { 
    function foo(): void;

    function foo(x: string): void;
}
export declare class B {
    setSceneParams(map: Map<string, Object>): B;
    setSceneParams(params: number): B;
    foo(): void;

    constructor(x: number, y: number); 

    constructor(magnitude: number); 

    foo(x: string): void;
}

declare function foo(): void;
declare function foo(x: string): void;



function xfoo(x ?: string): void {

}