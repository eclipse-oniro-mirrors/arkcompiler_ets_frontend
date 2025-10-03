/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

let sector: float
{
  const angle = Math.PI / 3;
  const radius = 10;
  sector = {
    radius,
    angle,
    area: (angle  2),
    perimeter: 2
  };
}


/* @@? 21:5 Error Semantic error ESE0065: type Float has no property named radius */
/* @@? 23:19 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? 23:20 Error Syntax error ESY0016: Unexpected token. */
/* @@? 23:21 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 24:16 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 26:1 Error Syntax error ESY0227: Unexpected token '}'. */