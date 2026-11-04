/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function blob(): void {}

let arrowBlob = (): void => {}

let doSmthArrow = () => { 
    try {
        blob()
    } catch (err) {
        blob()
        return;
    } finally {}
}

function doSmth1() {
    try {
        blob()
    } catch (err) {
        blob()
        return;
    } finally {}
}

function doSmth2() {
    try {
        arrowBlob()
    } catch (err) {
        arrowBlob()
        return;
    } finally {}
}

function main() {
    doSmth1()
    doSmth2()
    doSmthArrow()
}