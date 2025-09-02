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

enum Color { Red }
enum Color2 { Red = Color.Red }
enum Color3 { Red = Color2.Red }
enum Color4 { Red = Color3.Red }

arktest.assertEQ(Color2.Red.valueOf(), 0)
arktest.assertEQ(Color3.Red.valueOf(), 0)
arktest.assertEQ(Color4.Red.valueOf(), 0)
