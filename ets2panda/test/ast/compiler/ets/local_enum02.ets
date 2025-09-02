/*
<<<<<<<< HEAD:ets2panda/linter/src/lib/utils/functions/ProcessWrite.ts
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
========
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
>>>>>>>> origin/OpenHarmony_feature_Release_20250728:ets2panda/test/ast/compiler/ets/local_enum02.ets
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

<<<<<<<< HEAD:ets2panda/linter/src/lib/utils/functions/ProcessWrite.ts
export async function processSyncOut(message: string): Promise<void> {
  await new Promise((resolve) => {
    process.stdout.write(message, () => {
      resolve('success');
    });
  });
}

export async function processSyncErr(message: string): Promise<void> {
  await new Promise((resolve) => {
    process.stderr.write(message, () => {
      resolve('success');
    });
  });
}
========
enum Foo {Bar, Baz}

function main() {

    {
      enum Foo {Baz, Bar}
      arktest.assertEQ(Foo.Bar as int, 1)
    }

    arktest.assertEQ(Foo.Bar as int, 0)
}

/* @@? 21:7 Error SyntaxError: Illegal start of ENUM expression.  */
/* @@? 22:24 Warning Warning: Enum cast is deprecated. Cast support from 'Foo' to 'Int' will be removed. */
>>>>>>>> origin/OpenHarmony_feature_Release_20250728:ets2panda/test/ast/compiler/ets/local_enum02.ets
