/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import { cloudDatabase } from '@kit.CloudFoundationKit';
import cloudDatabaseHms from '@hms.core.deviceCloudGateway.cloudDatabase';

class Test extends cloudDatabase.DatabaseObject {
  query = new cloudDatabase.DatabaseQuery(Test); // Error

  equalTo() {
    return this.query.equalTo('test', '');
  }
}

const localQuery1 = new cloudDatabase.DatabaseQuery(Test); // Error

class LocalDatabaseQuery extends cloudDatabase.DatabaseQuery<Test> {
  query = new cloudDatabase.DatabaseQuery(Test); // Error

  set(query: cloudDatabase.DatabaseQuery<Test>) {
    query.equalTo('test', '');
  }

  getTypeQuery(): cloudDatabase.DatabaseQuery<Test> {
    typeof new cloudDatabase.DatabaseQuery(Test); // Error
    return new cloudDatabase.DatabaseQuery(Test); // Error
  }

  getEqual() {
    this.query = new Test().equalTo();
  }
}

function useDatabaseQuery1(query: cloudDatabase.DatabaseQuery<Test>) {
  console.log(' ' + new cloudDatabase.DatabaseQuery(Test)); // Error
}

const localQuery2 = new cloudDatabaseHms.DatabaseQuery(Test); // Error

function useDatabaseQuery2(query: cloudDatabaseHms.DatabaseQuery<Test>) {
  console.log(' ' + new cloudDatabaseHms.DatabaseQuery(Test)); // Error
}

class DatabaseQuery<T> {
  constructor(entityClass: () => T) {
  }
}

let a = (): number => {
  return 1;
};

new DatabaseQuery<number>(a);