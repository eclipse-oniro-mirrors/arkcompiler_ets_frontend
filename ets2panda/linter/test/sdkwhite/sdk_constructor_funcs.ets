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

import cloudDatabase from '@hms.core.deviceCloudGateway.cloudDatabase';
class Test extends cloudDatabase.DatabaseObject{
  query = new cloudDatabase.DatabaseQuery(Test); //Error
  equalTo(){
    return this.query.equalTo('test','');
  }
}
const  localQuery = new cloudDatabase.DatabaseQuery(Test); //Error
class LocalDatabaseQuery extends cloudDatabase.DatabaseQuery<Test>{ //Error
  query = new cloudDatabase.DatabaseQuery(Test); //Error
  set(query:cloudDatabase.DatabaseQuery<Test>){ //Error
    query.equalTo('test','');
  }
  getTypeQuery(): cloudDatabase.DatabaseQuery<Test> { //Error
    typeof new cloudDatabase.DatabaseQuery(Test); //Error
    return new cloudDatabase.DatabaseQuery(Test); //Error
  }
  getEqual(){
    this.query = new Test().equalTo(); //Error?
  }

}
function useDatabaseQuery1(query:cloudDatabase.DatabaseQuery<Test>){ //Error
  console.log(' '+new cloudDatabase.DatabaseQuery(Test)); //Error
}
class DatabaseQuery{}
new DatabaseQuery();