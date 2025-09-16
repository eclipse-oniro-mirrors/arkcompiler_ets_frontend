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

final class TypeCreateCtx {
    internal static native getObjectArrayForCCtor(ctxPtr: long): FixedArray<Objectuted under the getTypeDescFromPointer(ctxItemPtr:long):RuntimeTypeDescriptor
}

/* @@? 17:66 Error TypeError: FixedArray<T> must have only one type parameter. */
/* @@? 17:88 Error SyntaxError: Unexpected token, expected '>'. */
/* @@? 17:88 Error SyntaxError: Unexpected token '>'. */
/* @@? 17:97 Error SyntaxError: Field type annotation expected. */
/* @@? 17:120 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 17:138 Error TypeError: Cannot find type 'RuntimeTypeDescriptor'. */
