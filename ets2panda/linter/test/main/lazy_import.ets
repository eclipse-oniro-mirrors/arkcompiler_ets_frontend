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

import lazy { m } from 'module'

import lazy { a, b } from 'module1'
import { c } from 'module2'

import lazy {worker} from '@kit.ArkTS';   // (arkts-no-lazy-import)

import lazy {harTest} from "har1" // (arkts-no-lazy-import)

import lazy {add} from "hsp1" // (arkts-no-lazy-import)

/* test */ import lazy {WorkerEventListener} from '@kit.ArkTS'; // (arkts-no-lazy-import)

import /* test */ lazy {WorkerEventTarget} from '@kit.ArkTS'; // (arkts-no-lazy-import)

/* test */ import /* test */  lazy  /* test */ {WorkerOptions} /* test */  from '@kit.ArkTS'; // (arkts-no-lazy-import)

import lazy {worker as wk} from '@kit.ArkTS'; // (arkts-no-lazy-import)

/* test */ import /* test */
lazy      // (arkts-no-lazy-import)
/* test */ {worker as ttt} /* test */  from '@kit.ArkTS';