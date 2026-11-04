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

declare interface I1 {
    foo(value: string): this;
}

declare interface I2 extends I1{
    default anotherFoo():void;
}

declare class A implements I2 {
}

/* @@? 24:31 Error Semantic error ESE0190: A is not abstract and does not override abstract method foo(value: String): I1 in I1 */
