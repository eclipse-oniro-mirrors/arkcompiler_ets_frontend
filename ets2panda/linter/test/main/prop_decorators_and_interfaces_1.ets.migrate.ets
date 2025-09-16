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

import {
  PropRef,
  StoragePropRef,
  LocalStoragePropRef,
} from '@kit.ArkUI';

import {
  Entry,
  Component,
  StorageLink,
  LocalStorageLink,
} from '@kit.ArkUI';

class User {
  name: string = ""
  age: number = 0
}

@Entry
@Component
struct FatherComponent {
  @PropRef user1: User = new User()
  @StorageLink("user2") user2: User = new User()
  @LocalStorageLink("user3") user3: User = new User()

  build() {
  }
}

@Component
struct ChildComponent {
  @StoragePropRef("user2") user2: User = new User()
  @LocalStoragePropRef("user3") user3: User = new User()

  build() {
  }
}