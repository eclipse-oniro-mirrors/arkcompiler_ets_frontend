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

interface IOperate {
    run(): string;
    run(n: number): string;
}

class Runner implements IOperate {
    id: number = 1;

    run(): string {
        return "Runner:Zero:" + this.id;
    }

    run(n: number): string {
        return "Runner:One:" + (n + this.id);
    }
}

class SpecialRunner extends Runner {
    id: number = 2;

    run(): string {
        return "Special:" + super.run();
    }
}

class SuperRunner extends SpecialRunner {
    id: number = 3;

    run(n: number): string {
        return "Super:" + super.run(n);
    }
}

function main() {
    let s = new SuperRunner();
    arktest.assertEQ(s.run(), "Special:Runner:Zero:3");
    arktest.assertEQ(s.run(10), "Super:Runner:One:13");
}
