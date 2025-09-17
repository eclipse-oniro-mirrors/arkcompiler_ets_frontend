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
    readonly readOnlyOptional?: boolean;
}

class MixedClass implements MixedProperties {
    private _readOnlyOptional?: boolean;
    
    get readOnlyOptional(): boolean | undefined {
        return this._readOnlyOptional;
    }
    
    constructor(readOnlyOptional?: boolean) {
        this._readOnlyOptional = readOnlyOptional;
    }
}

function testMixedProperties() {
    let obj = new MixedClass(true);
    obj./* @@ label */readOnlyOptional = false;
}

/* @@@ label Error TypeError: Cannot assign to this property because it is readonly.  */
