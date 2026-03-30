/**
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

function measure(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    const originalMethod = descriptor.value;
    
    descriptor.value = function(...args: any[]) {
        print(`start: ${propertyKey}`);
        const start = Date.now();
        
        const result = originalMethod.apply(this, args);
        
        const end = Date.now();
        print(`${propertyKey} Finish, time-consuming: ${end - start}ms`);
        
        return result;
    };
    
    return descriptor;
}

class Calculator {
    @measure
    add(a: number, b: number) {
        for(let i = 0; i < 1000000; i++) {}
        return a + b;
    }
}

const calc = new Calculator();
calc.add(5, 3);
