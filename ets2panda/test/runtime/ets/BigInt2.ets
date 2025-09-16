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

function test_left_shift(): void {
    arktest.assertEQ(new BigInt(0).operatorLeftShift(new BigInt(0)), (0n))
    arktest.assertEQ(new BigInt(0).operatorLeftShift(new BigInt(1)), (0n))
    arktest.assertEQ(new BigInt(1).operatorLeftShift(new BigInt(0)), (1n))
    arktest.assertEQ(new BigInt(10).operatorLeftShift(new BigInt(2)), (40n))
    arktest.assertEQ(new BigInt(255).operatorLeftShift(new BigInt(41)), (560750930165760n))
    arktest.assertEQ(new BigInt(65535).operatorLeftShift(new BigInt(60)), (75556710804409716572160n))
    arktest.assertEQ(new BigInt(4294967295).operatorLeftShift(new BigInt(5)), (137438953440n))
    arktest.assertEQ(new BigInt(18446744073709551615n).operatorLeftShift(new BigInt(6)), (1180591620717411303360n))
    arktest.assertEQ(new BigInt(1275418875248948586535904902545412130n).operatorLeftShift(new BigInt(123)), (13562579802667292602585801202159372574330573695725523059072421474640855040n))
    arktest.assertEQ(new BigInt(-1).operatorLeftShift(new BigInt(0)), (-1n))
    arktest.assertEQ(new BigInt(-12).operatorLeftShift(new BigInt(4)), (-192n))
    arktest.assertEQ(new BigInt(-255).operatorLeftShift(new BigInt(19)), (-133693440n))
    arktest.assertEQ(new BigInt(-65535).operatorLeftShift(new BigInt(73)), (-618960574909724398159134720n))
    arktest.assertEQ(new BigInt(-4294967295).operatorLeftShift(new BigInt(24)), (-72057594021150720n))
    arktest.assertEQ(new BigInt(-18446744073709551615n).operatorLeftShift(new BigInt(31)), (-39614081257132168794624491520n))
    arktest.assertEQ(new BigInt(-4095059032950818422890113130149234924134n).operatorLeftShift(new BigInt(103)), (-41528832328721100931613913139905162112381494314462183326283215847555072n))
}

function test_right_shift(): void {
    arktest.assertEQ(new BigInt(-200).operatorRightShift(new BigInt(2)), (-50n))
    arktest.assertEQ(new BigInt(-12).operatorRightShift(new BigInt(2)), (-3n))
    arktest.assertEQ(new BigInt(-1).operatorRightShift(new BigInt(0)), (-1n))
    arktest.assertEQ(new BigInt(0).operatorRightShift(new BigInt(0)), (0n))
    arktest.assertEQ(new BigInt(0).operatorRightShift(new BigInt(1)), (0n))
    arktest.assertEQ(new BigInt(1).operatorRightShift(new BigInt(0)), (1n))
    arktest.assertEQ(new BigInt(55).operatorRightShift(new BigInt(2)), (13n))
    arktest.assertEQ(new BigInt(-50).operatorRightShift(new BigInt(2)), (-13n))
    arktest.assertEQ(new BigInt(255).operatorRightShift(new BigInt(4)), (15n))
    arktest.assertEQ(new BigInt(255).operatorRightShift(new BigInt(8)), (0n))
    arktest.assertEQ(new BigInt(65535).operatorRightShift(new BigInt(10)), (63n))
    arktest.assertEQ(new BigInt(4294967295).operatorRightShift(new BigInt(17)), (32767n))
    arktest.assertEQ(new BigInt(4294967295).operatorRightShift(new BigInt(48)), (0n))
    arktest.assertEQ(new BigInt(18446744073709551615n).operatorRightShift(new BigInt(55)), (511n))
    arktest.assertEQ(new BigInt(4930493049034092989191918392837727383823827n).operatorRightShift(new BigInt(129)), (7244n))
    arktest.assertEQ(new BigInt(34930693049034092980065918370009389341341932481328231493102392100010239n).operatorRightShift(new BigInt(41)), (15884640128676479880626138024130973163365493965706369405371n))
    arktest.assertEQ(new BigInt(34095405903900293499034024029409200000000000000000000094049304394284993939382949384984981018480144184891848991934893429489324901148449849382222222222222888384932890000103010381803018300103180381030084545225875678328499213403403984398439489384984287113383827387373382328328194394892948392222219791749717949183748237n).operatorRightShift(new BigInt(511)), (5085903091997271218878067997324523590835536101241386224503069830700984460490307918626942225457432781938207872710051592009243523341752202627425621983245162030428n))
    arktest.assertEQ(new BigInt(-255).operatorRightShift(new BigInt(2)), (-64n))
    arktest.assertEQ(new BigInt(-65535).operatorRightShift(new BigInt(12)), (-16n))
    arktest.assertEQ(new BigInt(-4294967295).operatorRightShift(new BigInt(18)), (-16384n))
    arktest.assertEQ(new BigInt(-18446744073709551615n).operatorRightShift(new BigInt(38)), (-67108864n))
    arktest.assertEQ(new BigInt(-4095059032950818422890113130149234924134n).operatorRightShift(new BigInt(99)), (-6460863952n))
    arktest.assertEQ(new BigInt(-3294302940940294094923040592059302590950294502940294029029409029429042942049028100104029439420990952n).operatorRightShift(new BigInt(100)), (-2598746800062401791237527125433630339355687972972274247737341685088307n))
}

function main() : void {
    test_left_shift()
    test_right_shift()
}

