/*
* Copyright (c) 2024-2025 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable low or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

class Base {
    foo(): Base {
        return new Base();
    }
}

class Derived extends Base {
    override foo(): Object {
        return new Object();
    }
}

/* @@? 23:17 Error Semantic error ESE0141: foo(): Object in Derived cannot override foo(): Base in Base because overriding return type is not compatible with the other return type. */
/* @@? 23:17 Error Semantic error ESE0136: Method foo(): Object in Derived not overriding any method */