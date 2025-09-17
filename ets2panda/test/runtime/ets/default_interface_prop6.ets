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

interface UnionInterface {
    value?: number | string;
    flag?: boolean | number;
}

class UnionClass implements UnionInterface {
    value?: number | string;
    flag?: boolean | number;
    
    constructor() {
        this.value = 10;
        this.flag = true;
    }
}

function testUnionProperties() {
    let obj = new UnionClass();
    arktest.assertEQ(obj.value, 10);
    arktest.assertEQ(obj.flag, true);
    
    obj.value = "ten";
    obj.flag = 1;
    
    arktest.assertEQ(obj.value, "ten");
    arktest.assertEQ(obj.flag, 1);
}
