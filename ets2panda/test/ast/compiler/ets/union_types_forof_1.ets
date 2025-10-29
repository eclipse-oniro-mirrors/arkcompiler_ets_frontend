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
 
function processValue(value: string | number): void {
    if (typeof value === 'string') {
    // Handle string case
        for (const it of value) {
            console.log(it);
        }
    } else {
        // Handle number case
        console.log(value.toString());
    }
}

const testValues: (string | number)[] = ['hello', 42, 'world', 7];

for (const item of testValues) {
    processValue(item);
}


/* @@? 19:26 Error Semantic error ESE0258: Object type doesn't have proper iterator method. */