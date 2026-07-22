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

class A {
    bar(): String {
        return "A.bar";
    }
}

function bar<T extends Object>(this: T): String {
    return "bar";
}

interface HasBaseBar {}

class Base {
    inheritedBar(): String {
        return "Base.inheritedBar";
    }
}

class Derived extends Base implements HasBaseBar {}

function inheritedBar(this: HasBaseBar): String {
    return "extension.inheritedBar";
}

interface HasHidden {}

class HiddenBase {
    private hidden(): String {
        return "HiddenBase.hidden";
    }
}

class HiddenDerived extends HiddenBase implements HasHidden {}

function hidden(this: HasHidden): String {
    return "extension.hidden";
}

arktest.assertEQ((new A).bar(), "A.bar");
arktest.assertEQ((new Derived).inheritedBar(), "Base.inheritedBar");
arktest.assertEQ((new HiddenDerived).hidden(), "extension.hidden");
