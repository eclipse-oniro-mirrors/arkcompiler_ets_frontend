/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

interface AInterface<T, U> {}
interface BInterface<T, U> extends AInterface<T, U> {}
final class B<T, U> implements BInterface<T, U>{}

final class StaticClass<T, U> implements AInterface<T, U>{
    public static staticFunction<T, U = string>(arg: AInterface<T, U>) : StaticClass<T, U> {
        return new StaticClass<T, U>();
    }
}

function func (arg: B<String, Number>) {
    return StaticClass.staticFunction(arg);
}

function main() {
    let a = func(new B<String, Number>());
}
