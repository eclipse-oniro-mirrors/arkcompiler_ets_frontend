/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export declare namespace recordkeyInterfaceTest {
    interface recordkeyTypeTest {
        /**
         * @recordkey objectTest1, objectTest2
         */
        recordkeyObjectTest(data: object): void;
        /**
         * @recordkey indexTest1, indexTest2
         */
        recordkeyIndexTest(data: { [key: string]: number }): void;
        /**
         * @recordkey recordTest1 recordTest2
         */
        recordkeyRecordTest(data: Record<string, object>): void;
        /**
         * @recordkey anyTest1, anyTest2, anyTest11,
         * @recordkey anyTest22
         */
        recordkeyAnyTest(data: any): void;
        /**
         * @recordkey1 testForRecordkey1
         */
        recordkey1Test(data: any): void;
        /**
         * @recordkey propertySignatureTest1, propertySignatureTest2
         */
        recordkeyPropertySignatureTest: (data: Record<string, object>) => void;
    }
}

/**
 * @recordkey variableStatementTest1, variableStatementTest2
 */
export const variableStatementTest: ((data: Record<string, object>) => void);

/**
 * @recordkey functionTest1, functionTest2
 */
export declare function functionTest(data: Record<string, object>): void;

/**
 * @recordkey typeTest1, typeTest2
 */
export type typeTest = (data: Record<string, object>) => void;

export declare class recordkeyClassTest {
  /**
   * @recordkey constructorTest1, constructorTest2
   */
  constructor(data: Record<string, object>);
  /**
   * @recordkey propertyTest1, propertyTest2
   */
  propertyTest: (data: Record<string, object>) => void;
  /**
   * @recordkey methodTest1, methodTest2
   */
  methodTest(data: Record<string, object>): void;
  /**
   * @recordkey setterTest1, setterTest2
   */
  set setterTest(data: Record<string, object>);
  /**
   * @recordkey2 testForRecordkey2
   */
  recordkey2Test(data: Record<string, object>): void;
}
export {};