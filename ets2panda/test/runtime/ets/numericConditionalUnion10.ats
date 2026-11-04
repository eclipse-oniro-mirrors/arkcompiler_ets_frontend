/*
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

let tests = 0; let left = 0; let right = 0;
function condition(c: boolean): boolean { tests++; return c; }
function a(): byte { left++; return 1 as byte; }
function b(): double { right++; return 2.0; }
function f(c: boolean): Object { return condition(c) ? a() : b(); }
function main() { tests = 0; left = 0; right = 0; arktest.assertTrue(f(true) instanceof Byte); arktest.assertTrue(f(false) instanceof Double); arktest.assertEQ(tests,2); arktest.assertEQ(left,1); arktest.assertEQ(right,1); }
