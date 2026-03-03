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

const processWithDefaultArray = async (prefix: string, items: string[] = ['default1', 'default2']): Promise<string> => {
    return prefix + items[0];
};

const ccc = (prefix: string, items: int[] = [1, 1, 2]): string => {
    return prefix + items.length.toString();
};

interface ParamType {
    a: number;
    b: string;
}

const func1 = (param: ParamType = { a: 1, b: 'hello' }) => {
    return param;
};

interface NestedParamType {
    id: number;
    details: ParamType;
}

const func2 = (param: NestedParamType = { id: 1, details: { a: 1, b: 'hello' } }) => {
    return param;
};

function main(): void {
    processWithDefaultArray("prefix_").then((result) => {
        arktest.assertEQ(result, "prefix_default1");
    });

    processWithDefaultArray("custom_", ['item1', 'item2']).then((result) => {
        arktest.assertEQ(result, "custom_item1");
    });

    let result1 = ccc("test_");
    arktest.assertEQ(result1, "test_3");

    let result2 = ccc("custom_", [10, 20, 30, 40]);
    arktest.assertEQ(result2, "custom_4");

    let defaultParam1 = func1();
    arktest.assertEQ(defaultParam1.a, 1);
    arktest.assertEQ(defaultParam1.b, 'hello');

    let customParam1 = func1({ a: 42, b: 'world' });
    arktest.assertEQ(customParam1.a, 42);
    arktest.assertEQ(customParam1.b, 'world');

    let defaultParam2 = func2();
    arktest.assertEQ(defaultParam2.id, 1);
    arktest.assertEQ(defaultParam2.details.a, 1);
    arktest.assertEQ(defaultParam2.details.b, 'hello');

    let customParam2 = func2({ id: 100, details: { a: 200, b: 'test' } });
    arktest.assertEQ(customParam2.id, 100);
    arktest.assertEQ(customParam2.details.a, 200);
    arktest.assertEQ(customParam2.details.b, 'test');
}
