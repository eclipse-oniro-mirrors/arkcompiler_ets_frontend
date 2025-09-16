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

namespace QualifiedNamespace {
    export namespace LevelOne {
        export namespace LevelTwo {
            export function qualifiedFunction() {
                return "Qualified Function";
            }

            export let qualifiedValue = 100;

            // Utility function to reset the state of LevelTwo
            export function resetState() {
                qualifiedValue = 100;  // Reset qualifiedValue to its initial state
            }
        }
    }

    // Utility function to reset the state of LevelOne and LevelTwo
    export function resetQualifiedState() {
        LevelOne.LevelTwo.resetState();
    }
}

function main() {
    // Reset QualifiedNamespace state before running tests
    QualifiedNamespace.resetQualifiedState();

    // Test access to deeply qualified namespace members
    arktest.assertEQ(QualifiedNamespace.LevelOne.LevelTwo.qualifiedFunction(), "Qualified Function",  "Qualified function access failed")
    arktest.assertEQ(QualifiedNamespace.LevelOne.LevelTwo.qualifiedValue, 100,  "Qualified value access failed")

    // Test modification of qualified value
    QualifiedNamespace.LevelOne.LevelTwo.qualifiedValue = QualifiedNamespace.LevelOne.LevelTwo.qualifiedValue + 50;
    arktest.assertEQ(QualifiedNamespace.LevelOne.LevelTwo.qualifiedValue, 150,  "Qualified value update failed")
}