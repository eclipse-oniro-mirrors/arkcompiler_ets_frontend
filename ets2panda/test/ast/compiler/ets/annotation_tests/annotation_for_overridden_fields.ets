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

@interface SinceSpec {
  specVersion: string
}

@interface BeforeSpec {
  specVersion: string
}

class TestFieldOverride {
  x: int = 1
}

class TestFieldOverrideOk extends TestFieldOverride {
  // @BeforeSpec({specVersion: "1.2.1-alpha"})
  x: int = 2
}

class TestFieldOverrideErr extends TestFieldOverride {
  @SinceSpec({specVersion: "1.2.1-alpha"})
  x: int = 2
}

/* !@@? 30:3 Warning Warning W122948: Cannot annotate this overriden field x in TestFieldOverride */
/* @@? 35:3 Error Semantic error ESE489430: Cannot annotate this overridden field x in TestFieldOverride */