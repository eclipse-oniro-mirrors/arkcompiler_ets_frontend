/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

// Step 1: Define an enum with a numeric constant
enum MyEnum {
  EnumField = 42
}

// Step 2: Define an annotation with a field initialized using the enum value
@interface ClassAnnotation {
  field: number = MyEnum.EnumField;
}

// Step 3: Apply the annotation to a class
@ClassAnnotation
class MyClass {
}
