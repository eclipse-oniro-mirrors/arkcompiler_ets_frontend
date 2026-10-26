/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class A {
  n: number = 100;
}

@Entry
@Component
struct Index {
  @State obj: A = new A();

  build() {
    RelativeContainer() {
      Text('Hello world')
        .width(this.getTextWidth())
    }
  }

  getTextWidth(): number {
    this.goo();
    return this.obj.n;
  }

  goo() {
    this.foo();
    this.obj.n--;   // can be detected
  }

  foo() {
    this.obj.n--;   // cannot be detected
  }
}