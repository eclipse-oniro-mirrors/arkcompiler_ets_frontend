/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");W
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export a /* @@ label */from /* @@ label1 */"./type"


/* @@? 13:8 Error Syntax error ESY0279: Cannot find name 'a' to export. */
/* @@@ label Error Syntax error ESY0227: Unexpected token 'from'. */
/* @@@ label Error Semantic error ESE0143: Unresolved reference from */
/* @@@ label1 Error Syntax error ESY0227: Unexpected token './type'. */