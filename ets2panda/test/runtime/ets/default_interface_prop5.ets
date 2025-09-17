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

interface BaseInterface {
    baseRequired: number;
    baseOptional?: string;
}

interface ExtendedInterface extends BaseInterface {
    extendedRequired: boolean;
    extendedOptional?: number[];
}

class MultiLevelClass implements ExtendedInterface {
    baseRequired: number;
    baseOptional?: string;
    extendedRequired: boolean;
    extendedOptional?: number[];
    
    constructor() {
        this.baseRequired = 1;
        this.extendedRequired = true;
    }
}

function testMultiLevelInheritance() {
    let obj = new MultiLevelClass();
    arktest.assertEQ(obj.baseRequired, 1);
    arktest.assertEQ(obj.baseOptional, undefined);
    arktest.assertEQ(obj.extendedRequired, true);
    arktest.assertEQ(obj.extendedOptional, undefined);
    
    obj.baseOptional = "base";
    obj.extendedOptional = [1, 2, 3];
    
    arktest.assertEQ(obj.baseOptional, "base");
    arktest.assertEQ(obj.extendedOptional?.length, 3);
    arktest.assertEQ(obj.extendedOptional?.[0], 1);
}
