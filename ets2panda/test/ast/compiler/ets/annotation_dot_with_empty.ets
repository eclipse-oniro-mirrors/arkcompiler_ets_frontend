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

class MyTest {
    @Consume.
    private property1;
}

/* @@? 17:6 Error SyntaxError: Invalid annotation name.  */
/* @@? 18:5 Error SyntaxError: Identifier expected.  */
/* @@? 18:22 Error SyntaxError: Field type annotation expected.  */
