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

interface IProcessor {
    process(val: number): string;
    process(val: string): string;
}

class BaseProcessor implements IProcessor {
    prefix: string = "Base";

    process(val: number): string {
        return this.prefix + "-Num:" + val;
    }

    process(val: string): string {
        return this.prefix + "-Str:" + val;
    }
}

class ExtendedProcessor extends BaseProcessor {
    prefix: string = "Ext";

    process(val: number): string {
        return "Override-" + super.process(val);
    }
}

function main() {
    let p: IProcessor = new ExtendedProcessor();
    arktest.assertEQ(p.process(42), "Override-Ext-Num:42");
    arktest.assertEQ(p.process("hello"), "Ext-Str:hello");
}
