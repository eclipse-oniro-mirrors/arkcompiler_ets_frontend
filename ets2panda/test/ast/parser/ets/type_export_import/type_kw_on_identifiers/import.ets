/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

/*---
expected_error: |-
    Warning: default element has already imported
    Warning: default element has already imported
    Warning: default element has already imported
---*/

import { A, B } from "./source"
import type { C, D } from "./source"
import { type E } from "./source"
import { F, type G } from "./source"
import { type H, type I } from "./source"
import { J, K, type L } from "./source"

import d from "./source"
import type dd from "./source"
import {type default as ddd} from "./source"
import {type default as type} from "./source";

import type { type /* @@ first */M, N } from "./source"   // expect: CTE
import type { O, type /* @@ second */P } from "./source"   // expect: CTE
import type { type /* @@ third */Q, type /* @@ fourth */R } from "./source"   // expect: CTE

import type {type /* @@ fifth */default as DD} from "./source"


/* @@@ first Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ second Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ third Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ fourth Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
/* @@@ fifth Error Syntax error ESY135857: Type keyword set on both the specifier list and on the specifier itself. */
