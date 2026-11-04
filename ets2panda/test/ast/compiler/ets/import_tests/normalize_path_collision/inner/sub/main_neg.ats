/**
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

// NOTE: This test only triggers the original crash on Windows.
// On Windows, NormalizePath produced different keys for '../..' and '../../'
// (e.g. 'C:\path\..' vs 'C:\path\'), causing a modules_ map collision
// that crashed the compiler. The NormalizePath fix (filesystem.cpp)
// now properly handles the final '..' and empty segments, so both paths
// produce the same key on all platforms.

import { value } from '../..'
import { value } from '../../'

let x: int = /* @@ v1 */value;

/* @@? 24:10 Warning Warning W125428: 'value' has already imported */
/* @@@ v1 Error Semantic error ESE0143: Unresolved reference value */
