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

type BaseObject = Record<string, xny>;

class X<T extends BaseObject> {
  x: T;

  constructor(init?: Partial<T>) {

    this.x = {} as T;
   
  }
}

/* @@? 16:34 Error TypeError: Cannot find type 'xny'. */
/* @@? 21:29 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 23:14 Error TypeError: Target type for class composite needs to be an object type, found 'T' */
