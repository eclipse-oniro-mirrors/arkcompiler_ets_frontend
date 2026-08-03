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

export enum NativeExitCode {
  Success = 0,
  DiagnosticError = 1,
  InternalError = 2,
}

export interface NativeProcessInvocation {
  readonly executable: string;
  readonly arguments: readonly string[];
  readonly cwd: string;
  readonly environment: Readonly<NodeJS.ProcessEnv>;
  readonly signal: AbortSignal | undefined;
  readonly timeoutMs: number;
  readonly maxOutputBytes: number;
}

export interface NativeProcessResult {
  readonly exitCode: number | null;
  readonly signal: NodeJS.Signals | null;
  readonly stdout: string;
  readonly stderr: string;
}

export type NativeProcessFailureKind = 'launch' | 'timeout' | 'cancelled' | 'output-limit';

export class NativeProcessError extends Error {
  public constructor(
    public readonly kind: NativeProcessFailureKind,
    message: string,
  ) {
    super(message);
    this.name = 'NativeProcessError';
  }
}
