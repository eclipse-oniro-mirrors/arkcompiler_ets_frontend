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

 interface Interface {
    instance_field: number
}

abstract class Abstract {
    private _instance_field: number = -1

    get instance_field(): number{
        return this._instance_field;
    }

    set instance_field(v: number) {
        this._instance_field = v;
    }
}

class Base extends Abstract {
}

class Derived extends Base implements Interface {
    instance_field: number = -1

    public method(): number {
        this.instance_field -= 1
        super.instance_field += 1
        return super.instance_field
    }
}

function main() {
    let d = new Derived
    arktest.assertEQ(d.method(), 0 )
    arktest.assertEQ(d.instance_field, -2)
}
