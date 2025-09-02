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

declare namespace buffer {
    class Blob {
        slice(start?: number, end?: number, type?: string): Blob;
    }
    class Buffer {
        readInt8(offset?: number): number;
        readInt16BE(offset?: number): number;
        readInt16LE(offset?: number): number;
        readInt32BE(offset?: number): number;
        readInt32LE(offset?: number): number;
        readIntBE(offset: number, byteLength: number): number;
        readIntLE(offset: number, byteLength: number): number;
        indexOf(value: string | number | Buffer | Uint8Array, byteOffset?: number, encoding?: BufferEncoding): number;
    }
    function from(string: String, encoding?: BufferEncoding): Buffer;
}
export default buffer;