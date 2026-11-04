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

function checkEqualityConsistency(eqRes:boolean, streqRes:boolean, p1:Any, p2:Any) {
    arktest.assertEQ(eqRes, p1 == p2)
    arktest.assertEQ(streqRes, p1 === p2)
    arktest.assertEQ(eqRes, streqRes)
}

function g() { return 1 }

const vi: int = g()
const vl: long = g()
const vf: float = g()
const vd: number = g()
const vb: bigint = (()=>1n)()

checkEqualityConsistency(vi == vl, vi === vl, vi, vl)
checkEqualityConsistency(vi == vf, vi === vf, vi, vf)
checkEqualityConsistency(vi == vd, vi === vd, vi, vd)
checkEqualityConsistency(vi == vb, vi === vb, vi, vb)

checkEqualityConsistency(vl == vi, vl === vi, vl, vi)
checkEqualityConsistency(vl == vf, vl === vf, vl, vf)
checkEqualityConsistency(vl == vd, vl === vd, vl, vd)
checkEqualityConsistency(vl == vb, vl === vb, vl, vb)

checkEqualityConsistency(vf == vi, vf === vi, vf, vi)
checkEqualityConsistency(vf == vl, vf === vl, vf, vl)
checkEqualityConsistency(vf == vd, vf === vd, vf, vd)
checkEqualityConsistency(vf == vb, vf === vb, vf, vb)

checkEqualityConsistency(vd == vi, vd === vi, vd, vi)
checkEqualityConsistency(vd == vl, vd === vl, vd, vl)
checkEqualityConsistency(vd == vf, vd === vf, vd, vf)
checkEqualityConsistency(vd == vb, vd === vb, vd, vb)

checkEqualityConsistency(vb == vi, vb === vi, vb, vi)
checkEqualityConsistency(vb == vl, vb === vl, vb, vl)
checkEqualityConsistency(vb == vf, vb === vf, vb, vf)
checkEqualityConsistency(vb == vd, vb === vd, vb, vd)
