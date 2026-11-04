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

interface A {
    foo(): string;
}

interface B extends A {
    bar(): string;
}

class Root implements A {
    foo(): string { return "Root"; }
}

class Level1 extends Root implements B {
    foo(): string { return "Level1"; }
    
    bar(): string { return "Level1-Bar"; }
}

class Level2 extends Level1 {
    foo(): string { return "Level2-" + super.foo(); }
    
    bar(): string { return "Level2-" + super.bar(); }
}

function main() {
    let obj: Level2 = new Level2();
    let asInterface: B = obj;
    let asBase: Root = obj;

    arktest.assertEQ(asInterface.foo(), "Level2-Level1");
    arktest.assertEQ(asInterface.bar(), "Level2-Level1-Bar");
    
    arktest.assertEQ(asBase.foo(), "Level2-Level1");
}
