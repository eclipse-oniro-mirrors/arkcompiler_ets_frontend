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

interface Configs {
    priority?: Priority;
    timeout?: number;
}

enum Priority {
    LOW,
    HIGH,
}

function executePriority(priority: Priority): string {
    return "priority";
}

function executeConfigs(configs: Configs): string {
    return "configs";
}

overload execute { executePriority, executeConfigs }

function executePriorityUnion(priority: Priority | undefined): string {
    return "priority";
}

function executeConfigsUnion(configs: Configs | undefined): string {
    return "configs";
}

overload executeUnion { executePriorityUnion, executeConfigsUnion }

function main(): void {
    arktest.assertEQ(execute({}), "configs");
    arktest.assertEQ(executeUnion({}), "configs");
}
