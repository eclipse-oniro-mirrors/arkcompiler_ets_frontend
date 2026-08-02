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

import { spawn, type ChildProcess, type ChildProcessByStdio } from 'node:child_process';
import type { Readable } from 'node:stream';

import { NativeProcessError, type NativeProcessInvocation, type NativeProcessResult } from './index';

const PROCESS_KILL_GRACE_MS = 10000;
type MonitoredProcess = ChildProcessByStdio<null, Readable, Readable>;

export async function executeNativeProcess(invocation: NativeProcessInvocation): Promise<NativeProcessResult> {
  validateInvocation(invocation);
  return new Promise<NativeProcessResult>((resolve, reject) => {
    const child = spawnNativeProcess(invocation);
    new NativeProcessMonitor(child, invocation, resolve, reject).start();
  });
}

function validateInvocation(invocation: NativeProcessInvocation): void {
  if (invocation.signal?.aborted === true) {
    throw new NativeProcessError('cancelled', 'native execution was cancelled before launch');
  }
  if (invocation.timeoutMs <= 0 || invocation.maxOutputBytes <= 0) {
    throw new NativeProcessError('launch', 'native process limits must be positive');
  }
}

function spawnNativeProcess(invocation: NativeProcessInvocation): MonitoredProcess {
  return spawn(invocation.executable, [...invocation.arguments], {
    cwd: invocation.cwd,
    env: { ...invocation.environment },
    detached: process.platform !== 'win32',
    shell: false,
    stdio: ['ignore', 'pipe', 'pipe'],
  });
}

class NativeProcessMonitor {
  private outputBytes = 0;
  private readonly stdoutChunks: Buffer[] = [];
  private readonly stderrChunks: Buffer[] = [];
  private terminationError: Error | undefined;
  private killGraceTimeout: NodeJS.Timeout | undefined;
  private timeout: NodeJS.Timeout | undefined;
  private completed = false;

  constructor(
    private readonly child: MonitoredProcess,
    private readonly invocation: NativeProcessInvocation,
    private readonly resolve: (result: NativeProcessResult) => void,
    private readonly reject: (error: Error) => void,
  ) {}

  start(): void {
    this.timeout = setTimeout(
      () => this.terminate(new NativeProcessError('timeout', 'native process timed out')),
      this.invocation.timeoutMs,
    );
    this.child.stdout.on('data', (chunk: Buffer) => this.collectOutput(this.stdoutChunks, chunk));
    this.child.stderr.on('data', (chunk: Buffer) => this.collectOutput(this.stderrChunks, chunk));
    this.child.once('error', (error) => this.fail(new NativeProcessError('launch', error.message)));
    this.child.once('close', (code, signal) => this.handleClose(code, signal));
    this.invocation.signal?.addEventListener('abort', this.abort, { once: true });
    if (this.invocation.signal?.aborted === true) {
      this.abort();
    }
  }

  private readonly abort = (): void => {
    this.terminate(new NativeProcessError('cancelled', 'native execution was cancelled'));
  };

  private collectOutput(target: Buffer[], chunk: Buffer): void {
    this.outputBytes += chunk.byteLength;
    if (this.outputBytes > this.invocation.maxOutputBytes) {
      this.terminate(new NativeProcessError('output-limit', 'native process output exceeded the configured limit'));
      return;
    }
    target.push(chunk);
  }

  private terminate(error: Error): void {
    if (this.terminationError !== undefined) {
      return;
    }
    this.terminationError = error;
    if (!killProcessTree(this.child)) {
      this.fail(error);
      return;
    }
    this.killGraceTimeout = setTimeout(() => this.forceFinish(error), PROCESS_KILL_GRACE_MS);
  }

  private forceFinish(error: Error): void {
    this.child.stdout.destroy();
    this.child.stderr.destroy();
    this.child.unref();
    this.fail(error);
  }

  private handleClose(exitCode: number | null, signal: NodeJS.Signals | null): void {
    if (this.terminationError !== undefined) {
      this.fail(this.terminationError);
      return;
    }
    const result: NativeProcessResult = {
      exitCode,
      signal,
      stdout: Buffer.concat(this.stdoutChunks).toString('utf8'),
      stderr: Buffer.concat(this.stderrChunks).toString('utf8'),
    };
    this.finish(() => this.resolve(result));
  }

  private fail(error: Error): void {
    this.finish(() => this.reject(error));
  }

  private finish(action: () => void): void {
    if (this.completed) {
      return;
    }
    this.completed = true;
    this.cleanup();
    action();
  }

  private cleanup(): void {
    if (this.timeout !== undefined) {
      clearTimeout(this.timeout);
    }
    if (this.killGraceTimeout !== undefined) {
      clearTimeout(this.killGraceTimeout);
    }
    this.invocation.signal?.removeEventListener('abort', this.abort);
  }
}

function killProcessTree(child: ChildProcess): boolean {
  if (process.platform !== 'win32' && child.pid !== undefined) {
    try {
      process.kill(-child.pid, 'SIGKILL');
      return true;
    } catch {
      // Fall through to the direct child kill if the process group is gone.
    }
  }
  return child.kill('SIGKILL');
}
