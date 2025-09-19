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

interface MixedProperties {
    required: number;
    optional?: string;
    readonly readOnlyOptional?: boolean;
}

class MixedClass implements MixedProperties {
    required: number;
    optional?: string;
    private _readOnlyOptional?: boolean;
    
    get readOnlyOptional(): boolean | undefined {
        return this._readOnlyOptional;
    }
    
    constructor(required: number, optional?: string, readOnlyOptional?: boolean) {
        this.required = required;
        this.optional = optional;
        this._readOnlyOptional = readOnlyOptional;
    }
}

function testMixedProperties() {
    let obj = new MixedClass(10, "hello", true);
    arktest.assertEQ(obj.required, 10);
    arktest.assertEQ(obj.optional, "hello");
    arktest.assertEQ(obj.readOnlyOptional, true);
    
    obj.optional = "world";
    arktest.assertEQ(obj.optional, "world");
}
