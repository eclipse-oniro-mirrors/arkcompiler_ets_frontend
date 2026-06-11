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

class Base {
  foo(p: string): string { return "base" }
}
class Derived extends Base {
  foo(p: string): string { return "d1" }
  foo(p: Object): string { return "d2" }
}

/* @@? 21:6 Error Semantic error ESE602430: Method foo(p: Object): String from class or interface Derived, inherited into class or interface Derived, conflicts with the previous declaration of method foo(p: String): String from class or interface Derived because both declarations override method foo(p: String): String from class or interface Base */
