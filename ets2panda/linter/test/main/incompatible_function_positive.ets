/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

type FuncType = (p: string) => void;


 let f2: FuncType = (p: string): void => {
    ((p) => {
        return 0;
    })(p);
 }

 f2 = (p: string): void => {
    ((p: string): number => {
        return 0;
    })(p);
 }

 let f3: (p: string) => number = p => {
    return 0;
 }

 f2 = (p: string): void => {
    f3(p);
 };

 type FuncType2 = (p: string) => number;

 let f4 : FuncType | FuncType2 = p => {
    return 0;
 }


type FuncTypeWithOptionalParam = (p?: string, q: number = 0) => void;

type BaseType = (p: string) => number;
type WrappedType = BaseType;
type FinalType = WrappedType;

let f7: FinalType = (p) => {
    return 0;
}

