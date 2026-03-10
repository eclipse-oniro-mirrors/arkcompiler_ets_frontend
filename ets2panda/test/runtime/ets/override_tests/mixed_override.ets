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

interface IComponent {
    getName(): string;
    process(input: string): string;
    process(input: number): string;
}

abstract class BaseComponent implements IComponent {
    category: string = "Base";
    version: number = 1;

    constructor() {
        this.initialize(); 
    }

    initialize(): void {
    }

    getName(): string {
        return "[" + this.category + "] v" + this.version;
    }

    process(input: string): string {
        return "BaseStr: " + input;
    }

    process(input: number): string {
        return "BaseNum: " + input;
    }

    abstract executeTask(): string;
}

class UIComponent extends BaseComponent {
    category: string = "UI";
    
    executeTask(): string {
        return "RenderUI";
    }

    process(input: string): string {
        return "UIWrapper(" + super.process(input) + ")";
    }
}

class Button extends UIComponent {
    category: string = "Button";
    version: number = 2;

    initialize(): void {
    }

    executeTask(): string {
        return "ClickButton";
    }

    process(input: number): string {
        return "BtnClick(" + input + ")";
    }
}

class DataComponent extends BaseComponent {
    category: string = "Data";
    
    executeTask(): string {
        return "FetchData";
    }

    process(input: string): string {
        return "DataQuery: " + input;
    }

    process(input: number): string {
        return "DataId: " + input;
    }
}

class SecureDataComponent extends DataComponent {
    category: string = "SecureData";

    process(input: string): string {
        return "Authorized " + super.process(input);
    }
}

function testPolymorphism() {
    let comps: BaseComponent[] = [new Button(), new SecureDataComponent()];
    
    let btn = comps[0];
    
    arktest.assertEQ(btn.getName(), "[Button] v2");
    
    arktest.assertEQ(btn.executeTask(), "ClickButton");
    
    arktest.assertEQ(btn.process("hello"), "UIWrapper(BaseStr: hello)");
    
    arktest.assertEQ(btn.process(123), "BtnClick(123)");

    let sec = comps[1];
    
    arktest.assertEQ(sec.getName(), "[SecureData] v1");
    
    arktest.assertEQ(sec.executeTask(), "FetchData");
    
    arktest.assertEQ(sec.process("secret"), "Authorized DataQuery: secret");
    
    arktest.assertEQ(sec.process(999), "DataId: 999");
}

function testInterfaceAccess() {
    let b: IComponent = new Button();
    
    arktest.assertEQ(b.process(1), "BtnClick(1)");
    arktest.assertEQ(b.process("s"), "UIWrapper(BaseStr: s)");
    arktest.assertEQ(b.getName(), "[Button] v2");
}

function main() {
    testPolymorphism();
    testInterfaceAccess();
}
