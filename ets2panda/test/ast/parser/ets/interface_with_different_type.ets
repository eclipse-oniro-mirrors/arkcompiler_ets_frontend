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
    instance_field(): void {
        return
    }
}

abstract class Abstract {
    instance_field: number = -1
}

class Base extends Abstract {
}

class Derived extends Base implements Interface {
    instance_field: number = -1
}

function main() {
    let d = new Derived
}

/* @@? 16:1 Error Semantic error ESE0215: Cannot inherit from interface Interface because method instance_field is inherited with a different declaration type */
/* @@? 30:5 Error Semantic error ESE0136: Method instance_field(<property>instance_field: Double): void in Derived not overriding any method */
