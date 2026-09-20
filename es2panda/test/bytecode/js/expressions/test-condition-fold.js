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

function comparisonAnd(a, b, c, d) {
    if (a < b && c < d) { return 1; }
    return 0;
}
function negatedComparison(a, b) {
    if (!(a < b)) { return 1; }
    return 0;
}
function doubleNot(a) {
    if (!!a) { return 1; }
    return 0;
}
function nested(a, b, c) {
    if (!((a < b && !c) || !(a === b))) { return 1; }
    return 0;
}
function values(a, b) {
    return [a && b, a || b, !a];
}
comparisonAnd(1, 2, 3, 4);
negatedComparison(1, 2);
doubleNot(1);
nested(1, 2, 0);
values(0, "x");
