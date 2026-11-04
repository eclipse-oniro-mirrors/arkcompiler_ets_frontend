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


function fooo() {
}

export { fooo as A }

class AA {
    a = 1
}

export { AA as BB }

interface CC {
    a() {}
}

export { CC as DD}

let bar = 1
export { bar as BAR}

/* @@? 17:1 Error Semantic error ESE71336: Exported function 'fooo' requires type annotation. */
/* @@? 23:5 Error Semantic error ESE71336: Exported class property 'a' requires type annotation. */
/* @@? 29:5 Error Semantic error ESE71336: Exported function 'a' requires type annotation. */
/* @@? 34:5 Error Semantic error ESE71336: Exported variable 'bar' requires type annotation. */
