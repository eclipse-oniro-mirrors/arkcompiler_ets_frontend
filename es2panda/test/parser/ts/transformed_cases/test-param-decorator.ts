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

const validations = new Map();

function required(target: any, propertyKey: string, parameterIndex: number) {
    if (!validations.has(target)) {
        validations.set(target, new Map());
    }
    const methodValidations = validations.get(target);
    if (!methodValidations.has(propertyKey)) {
        methodValidations.set(propertyKey, []);
    }
    methodValidations.get(propertyKey).push(parameterIndex);
}

function validate(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    const originalMethod = descriptor.value;
    
    descriptor.value = function(...args: any[]) {
        const requiredParams = validations.get(target)?.get(propertyKey) || [];
        for (const index of requiredParams) {
            if (args[index] === undefined || args[index] === null || args[index] === '') {
                throw new Error(`param ${index + 1} is required`);
            }
        }
        
        return originalMethod.apply(this, args);
    };
}

class UserService {
    @validate
    register(
        @required username: string,
        @required password: string,
        email?: string
    ) {
        print(`username: ${username}`);
        return { success: true };
    }
}

const service = new UserService();
service.register('Jone', '123456');

try {
    service.register('', '123456');
} catch (error) {
    print(error.message);
}