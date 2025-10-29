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



declare class A {
    [index : number]: stringdd
}

// Extra empty line special for equal position like in file "ambient_indexer_2.ets"
// #23399 Error almost has not ideal position, problem in generated code

/* @@? 19:5 Error Semantic error ESE0371: Cannot find type 'stringdd'. */