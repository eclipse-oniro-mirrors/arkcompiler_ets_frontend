/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

interface Vehicle {
    getHorsePower3(rpm: int, trq: int): int{
        return (rpm * trq) / 5252;
    }

    getPwrIndex(trq: int): int{
        return this.getHorsePower3(5252, trq);
    }

    getHorsePower3(rpm: int): int{
        return rpm;
    }

    getPwrIndex(trq: int, second: int): int{
        return this.getHorsePower3(trq + second);
    }
}

class Car implements Vehicle{}

function main(): void {
    arktest.assertEQ(new Car().getPwrIndex(1), 1)
    arktest.assertEQ(new Car().getPwrIndex(1,1), 2)
}
