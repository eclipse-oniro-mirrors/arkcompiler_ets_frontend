/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
import type { IncrementalLintInfo } from './IncrementalLintInfo';
import type { IsEtsFileCallback } from './IsEtsFileCallback';
import type { IsFileFromModuleCallback } from './IsFileFromModuleCallback';
import type { ReportAutofixCallback } from './autofixes/ReportAutofixCallback';

// Common options interface, additional fields may be used by plugins
export interface LinterOptions {
  checkTsAsSource?: boolean;
  ideInteractive?: boolean;
  migratorMode?: boolean;
  warningsAsErrors?: boolean;
  enableAutofix?: boolean;
  arkts2?: boolean;
  useRtLogic?: boolean;
  advancedClassChecks?: boolean;
  useRelaxedRules?: boolean;
  cancellationToken?: ts.CancellationToken;
  incrementalLintInfo?: IncrementalLintInfo;
  reportAutofixCb?: ReportAutofixCallback;
  isFileFromModuleCb?: IsFileFromModuleCallback;
  isEtsFileCb?: IsEtsFileCallback;
  interopCheckMode?: boolean;
  compatibleSdkVersion?: number;
  compatibleSdkVersionStage?: string;
  etsLoaderPath?: string;
  migrationMaxPass?: number;
  migrationFilePathMap?: Map<string, string>;
  noMigrationBackupFile?: boolean;
  migrationReport?: boolean;
  wholeProjectPath?: string;
  projectFolderList?: string[];
  checkTsAndJs?: boolean;
  inputFiles?: string[];
  ruleConfigTags?: Set<number>;
  autofixCheck?: boolean;
  autofixRuleConfigTags?: Set<number>;
}
