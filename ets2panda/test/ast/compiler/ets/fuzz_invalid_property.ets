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

class D{
    strB = (new StringBuilder).append('{ ').append(strB.toString()).append(' }')       
}

/* @@? 17:12 Error TypeError: Circular call function */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Boolean): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Byte): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Short): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Char): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Int): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Long): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Float): StringBuilder` */
/* @@? 17:12 Error TypeError: Call to `append` is ambiguous as `2` versions of `append` are available: `append(s: String): StringBuilder` and `append(i: Double): StringBuilder` */
/* @@? 17:52 Error TypeError: Property 'strB' must be accessed through 'this' */
