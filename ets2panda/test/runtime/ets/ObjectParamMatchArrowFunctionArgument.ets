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

class A {
    public on(event: String, callback: Object): string {
        return event;
    }
}

function main() {
    let temp: A = new A();
    let s1 = temp.on("hello1", () => { });
    let s2 = temp.on("hello2", (args?: Array<Object>) => { });
    let s3 = temp.on("hello3", (args?: Array<Object>): Int => { return 1; });
    assertEQ(s1, "hello1");
    assertEQ(s2, "hello2");
    assertEQ(s3, "hello3");
}
