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

declare namespace appManager {
    class A {
        test1(type: 'shareCompleted', callback: number): void
        on(type: 'setArticle', callback: number): void
        once(type: 'complete', callback: number): void
        off(type: number, callback: number): void
        off(type: 'add' | 'remove' | 'change', callback: number): void;
        code(pyte: boolean, callback: number): void
    }

    function off(type: 'waterMarkFlagChange', callbac: string): void;
    function on(event: 'knockShare'): void;
    function on(type: 'applicationState', observer: ApplicationStateObserver): number;
    function once(callback: 'knockShare'): void;

}

export default appManager;
