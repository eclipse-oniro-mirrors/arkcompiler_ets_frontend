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

export class AbstractDaoSession{
    public async queryRaw<T,K>(entityClass: T,where: string, ...selectionArgs: dataRdb.ValueType[]):Promise<Any>{
        let paramUse = Type.of(entityClass)
        let dao:AbstractDao<Any,Any> = this.getDao(DbUtils.getEntityClassName(paramUse));
        return await dao.queryRaw(where, ...selectionArgs);
    }
}


/* @@? 1:3 Error Semantic error ESE0371: Cannot find type 'dataRdb'. */
/* @@? 1:3 Error Semantic error ESE0070: 'ValueType' type does not exist. */
/* @@? 17:62 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 19:17 Error Semantic error ESE0371: Cannot find type 'AbstractDao'. */
/* @@? 20:16 Error Semantic error ESE0303: Type '*ERROR_TYPE*' can not be awaited, it is not a Promise. */