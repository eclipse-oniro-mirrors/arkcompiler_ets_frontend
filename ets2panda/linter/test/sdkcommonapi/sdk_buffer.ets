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
import buffer from './api/@ohos.buffer'

let blob: buffer.Blob = new buffer.Blob(['a', 'b', 'c', 'd', 'e']);
const newBlob = blob.slice(0, 2, 'MIME'); //error
const res1 = newBlob.size // 5"

//indexOf
let buf = buffer.from([-1, 5]);
let index = buf.indexOf(""); // error
let buf1 = new buffer.Buffer().indexOf(""); // error
let index2 = buffer.from("abc").indexOf(""); // error
let buf2 = buffer.from("123");
let buf3 = buf2;
buf3.indexOf(""); // error
let buf4 = new buffer.Buffer();
let buf5 = buf4;
buf5.indexOf(""); // error
buf5.indexOf(); // error
buf5.indexOf(1);
buf5.indexOf('adasf');
new buffer.Buffer().indexOf("555");

//sum:7