/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

function main() {
    let apple: Object = new Object();
    let banana: boolean = true;
    let cherry: boolean = false;

    arktest.assertTrue(apple && banana)
    arktest.assertTrue(banana && !!(apple))
    arktest.assertTrue(!!(apple) || cherry)
    arktest.assertTrue(cherry || !!(apple))

    arktest.assertTrue(apple && true)
    arktest.assertTrue(true && !!(apple))
    arktest.assertTrue(!(!(apple) || false))
    arktest.assertTrue(false || !!(apple))
}
