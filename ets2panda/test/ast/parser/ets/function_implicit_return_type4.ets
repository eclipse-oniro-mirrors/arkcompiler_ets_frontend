
/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
  public  fn(): void { return; }
}

final class B extends A {
  public override fn(): int { return 42; }
}


/* @@? 22:21 Error Semantic error ESE0141: fn(): Int in B cannot override fn(): void in A because overriding return type is not compatible with the other return type. */
/* @@? 22:21 Error Semantic error ESE0136: Method fn(): Int in B not overriding any method */