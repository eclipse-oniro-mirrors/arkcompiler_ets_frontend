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

interface MyList<T> {
    foo(): MyList<T>;
    bar(): MyList<T>;
}

class MyListImpl<T> implements MyList<T> {
    override foo(): MyList<T> {
        return this;
    }
    override bar(): MyList<T> {
        return this as MyList<T>;
    }
}
