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

// 1.
const tmpl = ``

// 2.
const flag = true
const a = +(flag)
const b = -(flag)
const c = ~(flag)

/* @@? 21:11 Error TypeError: Wrong operand type for unary expression */
/* @@? 21:12 Error TypeError: Bad operand type, the type of the operand must be numeric type. */
/* @@? 22:11 Error TypeError: Wrong operand type for unary expression */
/* @@? 22:12 Error TypeError: Bad operand type, the type of the operand must be numeric type. */
/* @@? 23:11 Error TypeError: Wrong operand type for unary expression */
/* @@? 23:12 Error TypeError: Bad operand type, the type of the operand must be numeric type. */
