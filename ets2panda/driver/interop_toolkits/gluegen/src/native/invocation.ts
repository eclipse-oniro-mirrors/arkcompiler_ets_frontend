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

import type { NativeProcessInvocation, NativeProcessResult } from './index';
import { executeNativeProcess } from './process';

export interface GlueGenProcessOptions {
  readonly executable: string;
  readonly cwd: string;
  readonly environment: Readonly<NodeJS.ProcessEnv>;
  readonly signal: AbortSignal | undefined;
  readonly timeoutMs: number;
  readonly maxOutputBytes: number;
}

interface GlueGenCommandOptions {
  readonly inputFileList?: string;
  readonly arktsConfig?: string;
  readonly output?: string;
  readonly cachePath?: string;
  readonly reportPath?: string;
}

export class GlueGenInvocation {
  private constructor(private readonly processInvocation: NativeProcessInvocation) {}

  public static builder(options: GlueGenProcessOptions): GlueGenCommandBuilder {
    return new GlueGenCommandBuilder(options, (invocation): GlueGenInvocation => new GlueGenInvocation(invocation));
  }

  public async execute(): Promise<NativeProcessResult> {
    return executeNativeProcess(this.processInvocation);
  }
}

class GlueGenCommandBuilder {
  public constructor(
    private readonly processOptions: GlueGenProcessOptions,
    private readonly createInvocation: (invocation: NativeProcessInvocation) => GlueGenInvocation,
    private readonly commandOptions: GlueGenCommandOptions = {},
  ) {}

  /**
   * Sets the path to the `fileInfo.txt` for gluegen native.
   * @param filePath The path to the `fileInfo.txt`, which records the list of input files for gluegen.
   * @returns The updated GlueGenCommandBuilder instance.
   */
  public inputFileList(filePath: string): GlueGenCommandBuilder {
    return this.withCommandOptions({ inputFileList: filePath });
  }

  /**
   * Sets the path to the `arktsconfig.json` file for gluegen.
   * @param filePath The path to the `arktsconfig.json` file for gluegen.
   * @returns The updated GlueGenCommandBuilder instance.
   */
  public arktsConfig(filePath: string): GlueGenCommandBuilder {
    return this.withCommandOptions({ arktsConfig: filePath });
  }

  /**
   * Sets the path to the output file for gluegen.
   * @param filePath The path to the output file for gluegen.
   * @returns The updated GlueGenCommandBuilder instance.
   */
  public output(filePath: string): GlueGenCommandBuilder {
    return this.withCommandOptions({ output: filePath });
  }

  /**
   * Sets the path to the cache directory for gluegen.
   * @param directoryPath cache directory path for gluegen.
   * @returns The updated GlueGenCommandBuilder instance.
   */
  public cachePath(directoryPath: string): GlueGenCommandBuilder {
    return this.withCommandOptions({ cachePath: directoryPath });
  }

  /**
   * Sets the path to the report file for gluegen.
   * @param filePath The path to the report file for gluegen.
   * @returns The updated GlueGenCommandBuilder instance.
   */
  public reportPath(filePath: string): GlueGenCommandBuilder {
    return this.withCommandOptions({ reportPath: filePath });
  }

  public build(): GlueGenInvocation {
    const inputFileList = requireCommandOption(this.commandOptions.inputFileList, '--input-file-list');
    const arktsConfig = requireCommandOption(this.commandOptions.arktsConfig, '--arktsconfig');
    const output = requireCommandOption(this.commandOptions.output, '--output');
    const cachePath = requireCommandOption(this.commandOptions.cachePath, '--cache-path');
    const reportPath = requireCommandOption(this.commandOptions.reportPath, '--report-path');
    const invocation: NativeProcessInvocation = {
      ...this.processOptions,
      arguments: [
        '--input-file-list',
        inputFileList,
        '--arktsconfig',
        arktsConfig,
        '--output',
        output,
        '--cache-path',
        cachePath,
        '--report-path',
        reportPath,
      ],
    };
    return this.createInvocation(invocation);
  }

  private withCommandOptions(options: GlueGenCommandOptions): GlueGenCommandBuilder {
    return new GlueGenCommandBuilder(this.processOptions, this.createInvocation, {
      ...this.commandOptions,
      ...options,
    });
  }
}

function requireCommandOption(value: string | undefined, option: string): string {
  if (value === undefined || value.trim() === '') {
    throw new Error(`missing required gluegen command option: ${option}`);
  }
  return value;
}
