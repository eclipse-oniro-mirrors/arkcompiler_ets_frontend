/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface J {
  fn(): void;
}

interface I extends J {}

abstract class A implements I {
  public override fn(): int { return 1; }
}

/* @@? 23:21 Error TypeError: fn(): Int in A cannot override fn(): void in J because overriding return type is not compatible with the other return type.  */
/* @@? 23:21 Error TypeError: Method fn(): Int in A not overriding any method  */
