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

function foo()
{
    /* @@ label1 */interface LocalInterface
    {
        /* @@ label2 */private property : int;
    }
}

/* @@@ label1 Error SyntaxError: Illegal start of INTERFACE expression. */
/* @@@ label2 Error SyntaxError: Local class or interface declaration members can not have access modifies. */
/* @@@ label2 Error SyntaxError: Identifier expected, got 'private'. */
