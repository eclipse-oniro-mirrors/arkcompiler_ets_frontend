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

class A {
  prop: number = 666;
}

function foo(arr: Array<A>): void {
  let an: A | null = null;
  for(let i = 0; i < arr.length; i = i + 1) {
     an = arr[i];
  }

  if (an == null) {
    return
  }

  arktest.assertTrue(an.prop == 666)
}
foo([new A()])

function goo(arr: Array<()=>void>): void {
  let an: (() => void)| null = null;
  for(let i = 0; i < arr.length; i = i + 1) {
     an = arr[i];
  }

  if (an == null) {
    return
  }

  arktest.assertTrue(an.name == 'testLambda')
}
let testLambda = () => {}
goo([testLambda])
