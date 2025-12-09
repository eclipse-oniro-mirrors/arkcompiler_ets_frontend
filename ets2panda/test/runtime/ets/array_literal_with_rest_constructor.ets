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

class ClassWithRestParams {
    value : int = 0;

    constructor(... args : Any[])
    {
        this.value = args.length;
    }
}

class ClassWithNormalConstructor {
    value : int = 0;

    constructor()
    {
        this.value = 42;
    }
}

function
main()
{
    let restArray : ClassWithRestParams[] = /* @@ restArray */ new ClassWithRestParams[3];
    let normalArray : ClassWithNormalConstructor[] = /* @@ normalArray */ new ClassWithNormalConstructor[2];

    // Check lengths
    arktest.assertTrue(restArray.length == 3);
    arktest.assertTrue(normalArray.length == 2);

    // Check each element is properly initialized
    for (let i = 0; i < restArray.length; i++) {
        arktest.assertTrue(restArray[i] != null);
        arktest.assertTrue(restArray[i] instanceof ClassWithRestParams);
    }

    for (let i = 0; i < normalArray.length; i++) {
        arktest.assertTrue(normalArray[i] != null);
        arktest.assertTrue(normalArray[i] instanceof ClassWithNormalConstructor);
    }
}

/* @@@ restArray Success */
/* @@@ normalArray Success */