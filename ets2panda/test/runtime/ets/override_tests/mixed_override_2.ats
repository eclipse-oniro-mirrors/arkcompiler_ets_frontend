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

class InitBase {
    value: string = "Base";
    
    constructor() {
        this.value = this.getValue(); 
    }
    
    getValue(): string {
        return "BaseDesc";
    }
}

class InitDerived extends InitBase {
    value: string = "Derived"; 
    
    getValue(): string {
        return "DerivedDesc";
    }
}

class InitDerivedNoField extends InitBase {
    getValue(): string {
        return "DerivedDesc";
    }
}


function main() {
    let d = new InitDerived();
    let d2 = new InitDerivedNoField();
    
    arktest.assertEQ(d.value, "Derived");
    arktest.assertEQ(d2.value, "DerivedDesc");
}