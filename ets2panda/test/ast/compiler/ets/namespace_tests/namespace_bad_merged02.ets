/*
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
namespace X {
  namespace Y {}
}
namespace X {
  export namespace Y {}  // CTE
}

namespace P {
  export namespace Q {}
  namespace R {}
}
namespace P {
  export namespace Q {}  // OK
  namespace R {}  // OK
}



/* @@? 20:10 Error Syntax error ESY0006: Unable to merge namespaces 'Y', because their modifiers are different. */