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

export function fun1(x: int): string {
  console.println("fun1 = " + x);
  return x.toString();
}

export function fun2(x: string): string {
  console.println("fun2 = " + x);
  return x;
}

export overload fun { fun1, fun2 }


export class C {

  foo1(x: int): string {
    console.println("foo1 = " + x);
    return x.toString();
  }

  foo2(x: string): string {
    console.println("foo2 = " + x);
    return x;
  }

  overload foo { foo1, foo2 }

}
