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

import { logger } from "./logger";

export const TextPositionUtils = {
    // char offset -> byte offset
    charOffsetToByteOffset(content: string, charOffset: number): number {
        if (charOffset < 0 || charOffset > content.length) {
            logger.error('Character offset out of bounds')
        }

        const encoder = new TextEncoder();
        return encoder.encode(content.substring(0, charOffset)).length;
    },

    // byte offset -> char offset
    byteOffsetToCharOffset(content: string, byteOffset: number): number {
        const encoder = new TextEncoder();
        const totalBytes = encoder.encode(content).length;

        if (byteOffset < 0 || byteOffset > totalBytes) {
            logger.error('Byte offset out of bounds')
        }

        const buffer = encoder.encode(content).subarray(0, byteOffset);
        return new TextDecoder('utf-8', { fatal: false }).decode(buffer).length;
    }
};