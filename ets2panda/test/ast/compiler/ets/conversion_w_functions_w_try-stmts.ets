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
    try { return i; } 
    catch (e) {}
    finally /* @@ label */{ return -1; }
}
/*
function getD(d : double): double {
    try { return d; } 
    catch (e) {}
    finally { return -1.0; }
}

function apple(): Byte {
    return getI(0) as byte;
}

function banana(): Short {
    return getI(1) as short;
}

function cherry(): Int {
    return getI(2) as int;
}

function date(): Long {
    return getI(3) as long;
}

function elderberry(): Float {
    return getI(4) as float;
}

function fig(): Float {
    return getD(4.0) as float;
}

function grape(): Double {
    return getI(5) as double;
}

function honeydewmelon(): Double {
    return getD(5.0) as double;
}

function indian_fig(): Char {
    return getI(65) as char;
}

function raspberry(): Short {
    return getD(1.0) as short;
}

function strawberry(): Int {
    return getD(2.0) as int;
}

function tomato(): Long {
    return getD(3.0) as long;
}

function uglifruit(): Char {
    return getD(65.0) as char;
}
*/
function main(): void {
    /*
  let asd : Byte = apple();
  banana();
  cherry();
  date();
  elderberry();
  fig();
  grape();
  honeydewmelon();
  indian_fig();
  
  raspberry();
  strawberry();
  tomato();
  uglifruit();  */
}


/* @@@ label Warning Warning W0021: Finally clause cannot complete normally */