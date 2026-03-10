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

abstract class AbstractLogger {
    prefix: string = "LOG";
    
    abstract log(msg: string): string;
    
    getPrefix(): string {
        return this.prefix;
    }
}

class ConsoleLogger extends AbstractLogger {
    prefix: string = "CONSOLE";

    log(msg: string): string {
        return "[" + this.getPrefix() + "] " + msg;
    }
}

class FileLogger extends AbstractLogger {
    prefix: string = "FILE";

    log(msg: string): string {
        return "Write [" + this.getPrefix() + "] " + msg;
    }
}

function main() {
    let l1: AbstractLogger = new ConsoleLogger();
    let l2: AbstractLogger = new FileLogger();

    arktest.assertEQ(l1.log("start"), "[CONSOLE] start");
    arktest.assertEQ(l2.log("data"), "Write [FILE] data");
}
