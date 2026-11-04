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

export class constructor_test  {
    public constructor() {
        this(0);
    }

    protected constructor(b : double, c : char, i : int) {
    }

    private constructor(i : int) {
    }

}

class derived extends constructor_test  {
    public constructor() {
        this(5);
    }

    protected constructor(i : int) {
        super(1.2, c'c', i);
    }

}
