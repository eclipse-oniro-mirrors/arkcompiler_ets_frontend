/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class A {
    native final foo() : void;
    static final /* @@ label */foo2() : void {};
}

/* @@@ label Error TypeError: Invalid method modifier(s): a final method can't have abstract or static modifier.  */
/* @@@ label Error TypeError: Invalid method modifier(s): a static method can't have abstract, final or override modifier.  */