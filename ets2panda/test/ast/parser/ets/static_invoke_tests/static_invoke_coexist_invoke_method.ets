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

class A /* @@ label */{
    static $_invoke(): int {
        return 1;
    }

    static $_instantiate<T extends A>(factory: ()=>T): T {
        return factory();
    }
}


/* @@@ label Error Semantic error ESE0221: Static $_invoke method and static $_instantiate method both exist in class/interface A is not allowed. */