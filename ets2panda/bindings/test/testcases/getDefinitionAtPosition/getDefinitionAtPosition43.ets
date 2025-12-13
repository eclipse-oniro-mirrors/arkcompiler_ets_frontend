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

'use static';

import {
  Foo,
  UserParm,
  ID,
  Color,
  Printable,
  Document,
  move,
  Direction,
  Service,
  foo1,
  Base,
  Derived,
  StaticClass,
  PrivateClass,
  EmptyClass,
  obj,
  a,
  Parent,
  Child,
  test,
  user,
  Person,
  Person1,
  UserList,
  numbers,
  moreNumbers,
  Test
} from 'getDefinitionAtPosition/getDefinitionAtPositionTest';

const prop = Foo.staticProperty;

const colorNum = Color.Red;

const documentName = new Document();
documentName.getTitle();

const serviceName = new Service();
serviceName.oldMethod();

const baseName = new Base();
const prop1Name = baseName.prop1;

const derivedName = new Derived();
const prop2Name = derivedName.prop2;
