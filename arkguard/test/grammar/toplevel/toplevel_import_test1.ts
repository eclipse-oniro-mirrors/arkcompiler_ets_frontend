/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Toplevel: var1, TestFunction, TestClass, TestNs, TestType
 * Export: b1, a1, c1, d1, e1
 * Expect toplevel elements to avoid being obfuscated as a1, b1, c1, d1, e1, since they're export elements
 */
import {b1, a1, c1, d1, e1} from './toplevel_import_test2';
let var1 = 1;
function TestFunction(): void {}
class TestClass {}
namespace TestNs {}
type TestType = {}
