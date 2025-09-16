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

let os = Math.abs(450);
let h: string = `${Math.floor(os / 60)}`;
let m = `${os % 60}`;
h.length === 1 ? (h = `0${h}`) : 1;
m.length === 1 ? (m = `0${m}`) : 1;
arktest.assertEQ(h, '07');
arktest.assertEQ(m, '30');
