/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

module.exports = {
  preset: 'ts-jest',
  testEnvironment: 'node',
  verbose: true,
  collectCoverage: true,
  coverageDirectory: '<rootDir>/dist/coverage',
  coveragePathIgnorePatterns: [
    '<rootDir>/test/'
  ],
  setupFilesAfterEnv: [
    '<rootDir>/test/testHook/jest.memory-usage.js',
    '<rootDir>/test/testHook/jest.time-usage.js',
    '<rootDir>/test/testHook/jest.abc-size.js'
  ],
  testMatch: [
    "<rootDir>/test/ut/base_modeTest/**/*.test.ts",
    '<rootDir>/test/ut/entryTest/**/*.test.ts',
    '<rootDir>/test/ut/generate_arktsconfigTest/**/*.test.ts',
    '<rootDir>/test/ut/compile_process_workerTest/**/*.test.ts',
    '<rootDir>/test/ut/declgen_process_workerTest/**/*.test.ts'
  ],
  testPathIgnorePatterns: [],
};