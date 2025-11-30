/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

interface AA {
    X :int;
}
class A implements AA{
    static X :int=1;
}
class extendA extends A{

}

/* @@? 19:22 Error TypeError: A is not abstract and does not implement getter for X property in AA */
/* @@? 22:24 Error TypeError: extendA is not abstract and does not implement getter for X property in AA */
