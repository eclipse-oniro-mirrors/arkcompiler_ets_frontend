/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class ClassE {
    count: number = 0;
}

class ClassF {
    count: number = 1;
}

class ClassG {
    count: number = 0;
}

class ClassH {
    count: number = 1;
}

function getClass3(): ClassG | ClassH {
    if (1 > 0) {
        return new ClassG();
    } else {
        return getClass4();
    }
}

function getClass4(): ClassH {
    return new ClassH()
}

@Entry
@Component
struct DataObservationTest_4 {
    @State a: ClassE | ClassF = 1 > 0 ? new ClassE() : new ClassF();
    @State b: ClassG | ClassH = getClass3();

    build() {
        Column() {
            Text(`${this.a.count}`)
            Button("count++")
                .onClick((e: ClickEvent) => {
                    this.a.count++;
                })
        }
    }
}