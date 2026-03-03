/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

/**
 * Test namespace with both export declare class and variable with implementation.
 * Verifies that the variable is not incorrectly marked as external when it has
 * an initializer (regression test for IN_AMBIENT_CONTEXT scope bug).
 */
namespace ns {
    export declare class A {}
    export let a = 10
}

function main(): void {
    arktest.assertEQ(ns.a, 10, "ns.a should be 10");
}
