/**
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

enum ByteEnum:byte{
     a = 1
}

enum ShortEnum:short{
     a = 1,
}

enum LongEnum{
     a = -0x1_1111_1111,
}

enum C {R,G,B}

enum IntEnum:int{
     a = 1,
}

enum FloatEnum:float{
     a = 3.14f,
}

enum DoubleEnum:double{
     a = -3.141592653589,
}

function main() : void {
     arktest.assertEQ(typeof(ByteEnum.a),"byte")
     arktest.assertEQ(typeof(ShortEnum.a),"short")
     arktest.assertEQ(typeof(LongEnum.a),"long")
     arktest.assertEQ(typeof(C.R),"int")
     arktest.assertEQ(typeof(IntEnum.a),"int")
     arktest.assertEQ(typeof(FloatEnum.a),"float")
     arktest.assertEQ(typeof(DoubleEnum.a),"number")
}