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

import type * as ts from 'typescript';
import type { LinterOptions } from './LinterOptions';
import type { MigrationInfo } from './progress/MigrationInfo';
import type { CmdProgressInfo } from './progress/CmdProgressInfo';

export interface LinterInputInfo {
  tsProgram: ts.Program;
  srcFiles: ts.SourceFile[];
  options: LinterOptions;
  tscStrictDiagnostics: Map<string, ts.Diagnostic[]>;
  migrationInfo?: MigrationInfo;
  cmdProgressInfo: CmdProgressInfo;
}
