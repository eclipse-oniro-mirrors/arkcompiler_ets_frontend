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

class Base {
    overload process { processNumber, processString }

    processNumber(n: number) { return 1 }
    processString(s: string) { return 2 }
}

class D1 extends Base {
    // method is overridden
    override processNumber(n: number) { return 3 }
    // overload declaration is inherited
}

class D2 extends D1 {
    overload process { processNumber, processString }
}

class D3 extends Base {
    // method is added:
    processInt(n: int) { return 4 }
    // new order for overloaded methods is specified:
    overload process { processInt, processNumber, processString }
}

class D4 extends Base {
    processNumber(n: float) {return 5}
    overload process { processNumber, processString } 
}

arktest.assertEQ(new D2().process(1), 3)
arktest.assertEQ(new D3().process(1), 4)
arktest.assertEQ(new D4().process(1), 5)
