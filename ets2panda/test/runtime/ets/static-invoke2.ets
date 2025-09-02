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

export class StructBase<T, OptionsT> {
    base_value: number = 10;

    static $_instantiate<S extends StructBase<S, OptionsS>, OptionsS> (
        factory: () => S,
        options: OptionsS
    ): S {
        const instance = factory()
        return instance
    }
}

interface Options {}

class StsTestComponent extends StructBase<StsTestComponent, Options>{
    child_value: number = 20;

    build() {
        let s1 = StsTestComponent.$_instantiate(() => new StsTestComponent(), {} as Options);
        arktest.assertEQ(s1.base_value, 10);
        arktest.assertEQ(s1.child_value, 20);

        let s2 = StsTestComponent({} as Options);
        arktest.assertEQ(s2.base_value, 10);
        arktest.assertEQ(s2.child_value, 20);
    }
}

function main(): void {
    let s = new StsTestComponent()
    s.build();
}
