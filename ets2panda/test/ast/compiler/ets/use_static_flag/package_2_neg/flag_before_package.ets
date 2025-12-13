"use static"
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

package usestaticpackage3;

let str2:String = "use static"
import {A as A} from "../export1.ets"
import {B as B} from "../export2.ets"


/* @@? 20:1 Error Syntax error ESY0226: Import declarations can only be used on the top level and before any other declaration, top level statement or directive. */
/* @@? 21:1 Error Syntax error ESY0226: Import declarations can only be used on the top level and before any other declaration, top level statement or directive. */