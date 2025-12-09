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

function foo1(): int {
    try {
        return 0;
    } catch (e) {
        return 1;
    }
}

function foo2(): int {
    try {
        if (true) {
            return 0;
        } else{
            return 1;
        }
    } catch (e) {
        return 2;
    }
}

function foo3(): int {
    try {
        return 0;  
    } catch (e) {
        if (e instanceof ArithmeticError) {
            return 1;
        } else if (e instanceof ClassCastError) {
            return 2;
        } else {
            return 3;
        }
    }
}

function foo4(): int {
    try {
        return 0;
    } catch (e) {
        return 1;
    } finally {

    }
}

function foo5(): int {
    try {

    } catch (e) {

    } finally /* @@ label */{
        return 2;
    }
}

/* @@@ label Warning Warning W0021: Finally clause cannot complete normally */