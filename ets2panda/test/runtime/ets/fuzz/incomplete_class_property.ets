/*
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
/*---
    tags: [compile-only, negative]
---*/

interface GeneratedTypeLiteralInterface2 {
  baz: 0 | 1 | 42 | undefined | '';
}

interface GeneratedTypeLiteralInterface1 {
  bar(): GeneratedTypeLiteralInterface2;
  baz: 0 | 1 | 42 | undefined | '';
}

interface Container {
  foo: GeneratedTypeLiteralInterface1;
  baz: 0 | 1 | 42 | undefined | '';
}

class D implements GeneratedTypeLiteralInterface1 {
  baz: "" | 0 | 1 | 42 | undefined;

  constructor(baz: "" | 0 | 1 | 42 | undefined) {
    this.baz = baz;
  }

  bar(): GeneratedTypeLiteralInterface2 {
    return new E(this.baz);
  }
}

class E implements GeneratedTypeLiteralInterface2 {
  baz: "" | 0 | 1 | 42 | undefined;

  constructor(baz: "" | 0 | 1 | 42 | undefined) {
    this.baz = baz;
  }
}

class B implements Container {
  baz: "" | 0 | 1 | 42 | undefined = 0;
  foo: GeneratedTypeLiteralInterface1 = new D(this.baz);
}

const container: Container = new B();
