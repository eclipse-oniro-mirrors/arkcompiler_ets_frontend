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

import { DeclaredIface } from './interface_annotation_in_decl_file_source';

function consumer(value: DeclaredIface): number {
  return value.field;
}

let instance: DeclaredIface = { field: 1 };
console.log(consumer(instance));

/* @@? interface_annotation_in_decl_file_source.d.ets:23:2 Error Semantic error ESE0067: 'ClassOnlyAnno' is not an annotation. */
