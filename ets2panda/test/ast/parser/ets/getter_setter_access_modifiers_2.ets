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

class Core {
    private _size: int;
    private _width: int;

    final get size(): int {
        return this._size;
    }

    final set size(s: int) {
        this._size = s;
    }
}

class Hex extends Core {
    private _size: int;
    private _width: int;

    override set size(s: int) {
        this._size = s;
    }

    override get size(): int {
        return this._size;
    }
}

/* @@? 33:22 Error TypeError: size(s: Int): void in Hex cannot override size(s: Int): void in Core because overridden method is final.  */
/* @@? 33:22 Error TypeError: Method size(s: Int): void in Hex not overriding any method  */
/* @@? 37:22 Error TypeError: size(): Int in Hex cannot override size(): Int in Core because overridden method is final.  */
/* @@? 37:22 Error TypeError: Method size(): Int in Hex not overriding any method  */
