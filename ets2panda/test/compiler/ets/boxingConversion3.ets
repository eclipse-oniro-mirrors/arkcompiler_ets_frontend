/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function primInt(a: int): void {

}

function retRefShort(a: short): Short {
    return new Short(a);
}

function main(): void {
    let a: Byte = 2;
    let b: Short = 2;
    let c: Char = c'\x02';
    primInt(a);
    primInt(b);
    primInt(c.toInt());
    let d: int = retRefShort(b);
    let e: double = retRefShort(b);
}
