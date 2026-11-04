/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

@Retention("SOURCE")
@interface Message{
   msg: string = ""
   id: int = 0
}

@Retention("SOURCE")
@interface Anno1{
}

@Retention("SOURCE")
@interface Anno2{
}

// annotations for boxing type
let boxingInt: @Message @Anno1 Int
let boxingFloat: @Anno1 @Anno2 Float
let boxingShort: @Message @Anno1 Short
let boxingByte: @Anno2 @Anno1 Byte
let boxingDouble: @Anno1 @Anno2 Double
let boxingNumber: @Message @Anno2 Number
let boxingChar: @Anno2 @Anno1 Char
let boxingLong: @Message @Anno2 Long

function main(){}