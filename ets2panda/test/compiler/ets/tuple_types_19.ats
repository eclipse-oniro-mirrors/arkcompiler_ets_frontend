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

class TuplePair<X, Y> {
    one: [X, Y]
    constructor(value: [X, Y]) { this.one = value;}
    public provide(): [X, Y] { return this.one; }
}

class TupleProvider<X> {
    memb: [X, X];
    constructor(value: [X, X]) { this.memb = value;}
    public publish(): [X, X] { return this.memb; }
}

function foo() {
    let number_tup: [Double, Double] = [42.0, 43.0];
    let tn: TupleProvider<number> = new TupleProvider<number>(number_tup);
    let result: [Number, Number] = tn.publish();
}
