/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

abstract class Stage1<T> {
    abstract process(p: T): T
}
abstract class Stage2 extends Stage1<byte> {
}
class Stage3 extends Stage2 {
    override process(p: byte): byte { return ++p }
}

function main() {
    let d: Stage1<Object> = (new Stage3()) as Stage1<Object>;
    arktest.assertEQ(d.process(2 as byte), 3);
    arktest.assertEQ(d.process(0 as byte), 1);
}
