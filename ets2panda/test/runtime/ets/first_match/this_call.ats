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

class MyArray<T> {
    public pushArray(...val: T[]): string {
        return "invoke1";
    }

    public pushOne(val: T): string {
        return "invoke2";
    }
    overload push{ pushOne, pushArray }
    
    public testThisCall1(val: T){
        return this.push(val);
    }

    public testThisCall2(...val: T[]){
        return this.push(...val);
    }
}

function main() {
    let test = new MyArray<number>();
    arktest.assertEQ(test.testThisCall1(1), "invoke2");
    arktest.assertEQ(test.testThisCall2(1, 2, 3), "invoke1");
}
