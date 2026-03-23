/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with License.
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
    private _data: number = 5

    private get data(): number {
        return this._data
    }

    private set data(value: number) {
        this._data = value
    }

    static getData(obj: D): number {
        return obj.data
    }

    static setData(obj: D, value: number): void {
        obj.data = value
    }
}

class D extends A {}

function main(): int {
    const d = new D()
    const result = A.getData(d)
    A.setData(d, 15)
    return Double.toInt(result + A.getData(d))
}