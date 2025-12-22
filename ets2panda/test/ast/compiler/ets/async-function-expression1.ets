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

function relationalStoreCustomDirTest() {
    it(async function () {})

    it(async function () {
        let storestore = null;
        let u8 = new Uint8Array([1, 2, 3]);
        const valueBucket = {
            "name": "",
            "age": 18,
            "salary": 100.5,
            "blobType": u8,
        }
        let ret = await storestore.insert("test", valueBucket);
    })

    it(async function () {
        const STORE_CONFIG = {
            name: "",
            securityLevel: data_Rdb.SecurityLevel.S1,
            customDir: ""
        }
        let storestore = await data_Rdb.getRdbStore(context, STORE_CONFIG);
        await storestore.executeSql(CREATE_TABLE_TEST, null);
        let u8 = new Uint8Array([1, 2, 3]);
        const valueBucket = {
            "name": "",
            "age": 18,
            "salary": 100.5,
            "blobType": u8,
        }
        let ret = await storestore.insert("test", valueBucket);
    })
}

/* @@? 17:5 Error Semantic error ESE0143: Unresolved reference it */
/* @@? 17:14 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */
/* @@? 19:5 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 19:14 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */
/* @@? 31:5 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 31:14 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */