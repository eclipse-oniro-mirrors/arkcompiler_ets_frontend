/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
export let foo = {name: "123"}
export let person = {age: 12, male: [1, 2, 3]}
export class TestHelper {
    constructor(testName) {
        this.testName = testName;
        this.passed = 0;
        this.failed = 0;
    }

    test(testFunction, description) {
        const result = testFunction();
        if (result) {
            this.passed++;
            console.log(`[PASS] ${this.testName}: ${description}`);
        } else {
            this.failed++;
            console.error(`[FAIL] ${this.testName}: ${description}`);
        }      
    }

    getStats() {
        return {
            passed: this.passed,
            failed: this.failed,
            total: this.passed + this.failed
        };
    }

    printSummary() {
        const stats = this.getStats();
    }
}

export class Machine {
    name = "machine";
}

export class User {
    id;
    constructor(a){
        this.id = a;
    }
}

export class Person {
    name;
    age;
    constructor(a, b){
        this.name = a;
        this.age = b;
    }
}

export class Employee {
    name;
    constructor(a = "employee"){
        this.name = a;
    }
}