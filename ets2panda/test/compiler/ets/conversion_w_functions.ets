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

function getI(i : int): int {
    return i;
}

function getD(d : double): double {
    return d;
}

function apple(): Byte {
    return getI(0).toByte();
}

function banana(): Short {
    return getI(1).toShort();
}

function cherry(): Int {
    return getI(2).toInt();
}

function date(): Long {
    return getI(3).toLong();
}

function elderberry(): Float {
    return getI(4).toFloat();
}

function fig(): Float {
    return getD(4.0).toFloat();
}

function grape(): Double {
    return getI(5).toDouble();
}

function honeydewmelon(): Double {
    return getD(5.0).toDouble();
}

function indian_fig(): Char {
    return getI(65).toChar();
}

function raspberry(): Short {
    return getD(1.0).toShort();
}

function strawberry(): Int {
    return getD(2.0).toInt();
}

function tomato(): Long {
    return getD(3.0).toLong();
}

function main(): void {}
