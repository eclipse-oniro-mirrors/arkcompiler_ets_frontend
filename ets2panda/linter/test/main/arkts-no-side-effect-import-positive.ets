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

import { TempClass } from './utils/import-test'; // 1. Using Named Imports

import * as Temp from './utils/import-test'; // 2. Using Namespace Imports

import DefaultTempClass from './utils/import-test'; // 3. Using Default Imports

import './utils/import-test'; // 4. Intentional side-effect import When there is no side-effect
import { getTheme } from './utils/import-test'; // Linter will catch this

export {default as TestDef} from './utils/import-test'; // 5. Re-exporting Imports
export * from './utils/import-test';

const test = Temp.TempClass.name;

console.log(TempClass.name);

const test2 = DefaultTempClass.name;

getTheme();

