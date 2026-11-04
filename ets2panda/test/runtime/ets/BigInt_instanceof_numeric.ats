/*
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

function test_bigint_instanceof_byte(): void {
    let t: BigInt = new BigInt(1);
    arktest.assertFalse(t instanceof byte, "BigInt instanceof byte should be false");
    arktest.assertFalse(t instanceof short, "BigInt instanceof short should be false");
    arktest.assertFalse(t instanceof int, "BigInt instanceof int should be false");
    arktest.assertFalse(t instanceof long, "BigInt instanceof long should be false");
    arktest.assertFalse(t instanceof float, "BigInt instanceof float should be false");
    arktest.assertFalse(t instanceof double, "BigInt instanceof double should be false");
    arktest.assertTrue(t instanceof BigInt, "BigInt instanceof BigInt should be true");
    arktest.assertTrue(t instanceof Object, "BigInt instanceof Object should be true");
}

function main(): void {
    test_bigint_instanceof_byte();
}

