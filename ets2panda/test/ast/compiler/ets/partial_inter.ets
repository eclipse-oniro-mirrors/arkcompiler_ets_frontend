/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface User<T> {
    id: number;
    name?: string;
    generic?: T
}

function main() {
    let gu: Readonly<NonNullable<Partial<Awaited<Promise<User<string>>>>>> = {id : 0, name: "test"}
    /* @@ label */gu.id = 10;
}

/* @@@ label Error TypeError: The 'Readonly' property cannot be reassigned.  */
