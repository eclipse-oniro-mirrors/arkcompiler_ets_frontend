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

class A extends Numeric {
    private value: int;

    public constructor() {
        this.value = 0;
    }

    public constructor(value: int) {
        this.value = value;
    }

    public override toByte(): byte {
        return Int.toByte(this.value);
    }

    public override toShort(): short {
        return Int.toShort(this.value);
    }

    public override toInt(): int {
        return this.value;
    }

    public override toLong(): long {
        return Int.toLong(this.value);
    }

    public override toFloat(): float {
        return Int.toFloat(this.value);
    }

    public override toDouble(): double {
        return Int.toDouble(this.value);
    }

    public override toString(): String {
        return new String(this.value);
    }
}

function main(){
    let r: Record<A | number, number> = {
        1: 1,
    }
}
