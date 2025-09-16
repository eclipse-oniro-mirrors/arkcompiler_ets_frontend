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

export class binary_operations  {
    private sum1 : int = 3 + 2;
    private sum2 : int = 3 + 2 - 4 + 1;
    private sub1 : int = 8 - 3;
    private sub2 : int = 8 - 3 + 8;
    private rem1 : int = 5 % 3;
    private rem2 : int = 5 % 3 - 2 * 8;
    private mult1 : float = 6.2f * 3.14f;
    private mult2 : double = 6.2 * (3.7 + 8.18) / 2.2;
    private div1 : float = 4.2f / 2.3f;
    private div2 : double = 4.2 / (2.2 - 0.1) + (3.3 / 6.1);
    private lsh : int = 7 << 2;
    private rsh1 : int = 7 >> 2;
    private rsh2 : int = 7 >>> 2;
    private bit1 : int = 7 & 3;
    private bit2 : int = 7 | 3;
    private bit3 : int = 7 ^ 3;
    private b1 : boolean = 3 < 8;
    private b2 : boolean = 3 <= 7;
    private b3 : boolean = 3 >= 7;
    private b4 : boolean = 3 > 8;
    private b5 : boolean = 7 == 8;
    private b6 : boolean = 7 != 4;
    private b7 : boolean = (7 < 3) && (8 > 1);
    private b8 : boolean = (7 < 3) || (8 > 1);
}
