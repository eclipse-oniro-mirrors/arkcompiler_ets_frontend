/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';

import { Es2pandaContextState, Lsp, LspDiagnosticNode, ModuleDescriptor, PathConfig } from './index';

type DiagnosticKind = 'syntactic' | 'semantic';
const RESULT_MARKER = '__ETS_DIAGNOSTICS_RESULT__';

function installNode14Compatibility(): void {
  const arrayPrototype = Array.prototype as unknown as { at?: (index: number) => unknown };
  if (!arrayPrototype.at) {
    Object.defineProperty(arrayPrototype, 'at', {
      configurable: true,
      writable: true,
      value(index: number): unknown {
        const normalized = Math.trunc(index) || 0;
        return this[normalized < 0 ? this.length + normalized : normalized];
      }
    });
  }

  const objectConstructor = Object as unknown as { hasOwn?: (object: object, key: PropertyKey) => boolean };
  if (!objectConstructor.hasOwn) {
    objectConstructor.hasOwn = (object: object, key: PropertyKey): boolean =>
      Object.prototype.hasOwnProperty.call(object, key);
  }

  const stringPrototype = String.prototype as unknown as { replaceAll?: (search: string, replacement: string) => string };
  if (!stringPrototype.replaceAll) {
    Object.defineProperty(stringPrototype, 'replaceAll', {
      configurable: true,
      writable: true,
      value(search: string, replacement: string): string {
        return this.split(search).join(replacement);
      }
    });
  }
}

interface DiagnosticRequest {
  buildTools: string;
  projectRoot: string;
  files: string[];
  checks: DiagnosticKind[];
  cacheDir?: string;
  initAstCache?: boolean;
}

interface SerializedDiagnostic {
  file: string;
  kind: DiagnosticKind;
  message: string;
  source: string;
  code: number | string;
  severity: number;
  range: {
    start: { line: number; character: number };
    end: { line: number; character: number };
  };
}

function readRequest(): DiagnosticRequest {
  const raw = fs.readFileSync(0, 'utf8');
  const request = JSON.parse(raw) as DiagnosticRequest;
  if (!request.buildTools || !request.projectRoot || !Array.isArray(request.files) || !Array.isArray(request.checks)) {
    throw new Error('Invalid diagnostics request');
  }
  return request;
}

function serialize(file: string, kind: DiagnosticKind, diagnostic: LspDiagnosticNode): SerializedDiagnostic {
  return {
    file,
    kind,
    message: diagnostic.message.toString(),
    source: diagnostic.source.toString(),
    code: typeof diagnostic.code === 'number' ? diagnostic.code : diagnostic.code.toString(),
    severity: diagnostic.severity,
    range: {
      start: { line: diagnostic.range.start.line, character: diagnostic.range.start.character },
      end: { line: diagnostic.range.end.line, character: diagnostic.range.end.character }
    }
  };
}

function setupEnvironment(buildTools: string): void {
  process.env.BINDINGS_PATH = process.env.BINDINGS_PATH || path.resolve(__dirname, '..');
  process.env.PANDA_LIB_PATH = path.join(buildTools, 'ets2panda', 'lib');
  process.env.PANDA_BIN_PATH = path.join(buildTools, 'ets2panda', 'bin');
}

function createPathConfig(buildTools: string, projectRoot: string, cacheDir: string): PathConfig {
  return {
    buildSdkPath: path.dirname(buildTools),
    projectPath: projectRoot,
    declgenOutDir: path.join(cacheDir, 'declgen'),
    cacheDir
  };
}

function getDiagnosticsForCheck(lsp: Lsp, file: string, check: DiagnosticKind): LspDiagnosticNode[] {
  const result = check === 'syntactic'
    ? lsp.getSyntacticDiagnostics(file)
    : lsp.getSemanticDiagnostics(file);
  if (!result) {
    throw new Error(`Failed to obtain ${check} diagnostics for ${file}`);
  }
  return result.diagnostics ?? [];
}

function serializeAll(file: string, check: DiagnosticKind, diagnostics: LspDiagnosticNode[]): SerializedDiagnostic[] {
  return diagnostics.map((diagnostic) => serialize(file, check, diagnostic));
}

function collectCachedDiagnostics(
  lsp: Lsp,
  files: string[],
  check: DiagnosticKind,
  cached: Map<string, SerializedDiagnostic[]>
): void {
  for (const file of files) {
    lsp.prepareSdkValidatorFile(file);
    cached.set(`${file}\0${check}`, serializeAll(file, check, getDiagnosticsForCheck(lsp, file, check)));
  }
}

function runCachedDiagnostics(
  request: DiagnosticRequest,
  pathConfig: PathConfig,
  modules: ModuleDescriptor[],
  files: string[]
): SerializedDiagnostic[] {
  // Syntactic ranges must be materialized before checking/lowering rewrites
  // the AST. Semantic diagnostics require a separate checked context.
  const cached = new Map<string, SerializedDiagnostic[]>();
  for (const check of request.checks) {
    const lsp = new Lsp(pathConfig, undefined, modules);
    try {
      const targetState = check === 'semantic'
        ? Es2pandaContextState.ES2PANDA_STATE_CHECKED
        : Es2pandaContextState.ES2PANDA_STATE_PARSED;
      lsp.initSdkValidatorCache(files, targetState);
      collectCachedDiagnostics(lsp, files, check, cached);
    } finally {
      lsp.dispose();
    }
  }
  const diagnostics: SerializedDiagnostic[] = [];
  for (const file of files) {
    for (const check of request.checks) {
      diagnostics.push(...(cached.get(`${file}\0${check}`) ?? []));
    }
  }
  return diagnostics;
}

function runDirectDiagnostics(lsp: Lsp, files: string[], checks: DiagnosticKind[]): SerializedDiagnostic[] {
  const diagnostics: SerializedDiagnostic[] = [];
  for (const file of files) {
    for (const check of checks) {
      diagnostics.push(...serializeAll(file, check, getDiagnosticsForCheck(lsp, file, check)));
    }
  }
  return diagnostics;
}

function emitResult(diagnostics: SerializedDiagnostic[]): void {
  process.stdout.write(`${RESULT_MARKER}${JSON.stringify({ diagnostics })}`);
}

function cleanup(cacheDir: string, removeCache: boolean): void {
  if (removeCache) {
    fs.rmSync(cacheDir, { recursive: true, force: true });
  }
}

function main(): void {
  installNode14Compatibility();
  const request = readRequest();
  const buildTools = path.resolve(request.buildTools);
  const projectRoot = path.resolve(request.projectRoot);
  const files = request.files.map((file) => path.resolve(file));
  const cacheDir = request.cacheDir
    ? path.resolve(request.cacheDir)
    : fs.mkdtempSync(path.join(os.tmpdir(), 'ets-lsp-diagnostics-'));
  const removeCache = request.cacheDir === undefined;

  setupEnvironment(buildTools);
  const pathConfig = createPathConfig(buildTools, projectRoot, cacheDir);
  const modules: ModuleDescriptor[] = [{
    name: 'diagnostics',
    moduleType: 'har',
    srcPath: projectRoot
  }];

  if (request.initAstCache) {
    try {
      emitResult(runCachedDiagnostics(request, pathConfig, modules, files));
    } finally {
      cleanup(cacheDir, removeCache);
    }
    return;
  }

  const lsp = new Lsp(pathConfig, undefined, modules);
  try {
    emitResult(runDirectDiagnostics(lsp, files, request.checks));
  } finally {
    lsp.dispose();
    cleanup(cacheDir, removeCache);
  }
}

main();
