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

@ns.Anno<()=>void>
class A{}

/* @@? 16:2 Error Semantic error ESE0371: Cannot find type 'ns'. */
/* @@? 16:5 Error Semantic error ESE0070: 'Anno' type does not exist. */
/* @@? 16:5 Error Semantic error ESE0067: 'Anno' is not an annotation. */