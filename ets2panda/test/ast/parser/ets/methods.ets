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

class C {
  a(): void {};
  static b(): void {};
  public final c(): void {};
  private final d(): void {};
  protected final e(): void {};
  protected f(): void {};
  protected static g(): void {};
  //native z(): void
}

abstract class C1 {
  abstract a(): void;
}

final class C2 extends C1 {
  override a(): void {};
}

class C3 {
  a/* @@ label */(): void;
}

final class C4 extends C3 {
  override a(): void {};
}


/* @@@ label Error Semantic error ESE0017: Only abstract or native methods can't have body. */