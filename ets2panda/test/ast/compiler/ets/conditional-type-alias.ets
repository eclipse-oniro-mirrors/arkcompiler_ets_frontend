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

// (arkts-no-conditional-types)
type X<T> = T extends number ? T : string

/* @@? 17:15 Error SyntaxError: Unexpected token 'extends'. */
/* @@? 17:15 Error TypeError: Conditional type aliases are not supported. Introduce a new type with constraints explicitly, or rewrite logic using Object! */
/* @@? 17:23 Error SyntaxError: Unexpected token 'number'. */
/* @@? 17:23 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 17:32 Error TypeError: Unresolved reference T */
/* @@? 17:36 Error TypeError: Type name 'string' used in the wrong context */
