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
import * as os from 'os';
import * as fs from 'fs';
import * as path from 'path';
import * as child_process from 'child_process';
import * as common from '@interop-toolkits/common';
import { NodeType } from '../graph';
import type { DependencyNode } from '../graph';
import { PartialResolver } from '../partialResolver';
import type { PartialResolvedDependencyMap } from '../partialResolver';

/** Name of the working sub-directory created under the module cache. */
const WORK_DIR_NAME = 'dependency-resolver';
/** File that lists the static entry files fed to dep_analyzer (one per line). */
const DEP_ANALYZER_INPUT_FILE = 'dep_analyzer_input.txt';
/** File dep_analyzer writes its JSON dependency map to. */
const DEP_ANALYZER_OUTPUT_FILE = 'dep_analyzer_output.json';
/**
 * The JSON shape produced by ets2panda's `dep_analyzer`.
 *
 * `dependencies[file]` lists the files that `file` imports; `dependants[file]`
 * lists the files that import `file`; `outputMatching` maps a source file to
 * its compiled output.
 */
interface DependencyFileMap {
  dependants: Record<string, string[]>;
  dependencies: Record<string, string[]>;
  outputMatching: Record<string, string>;
}

type ArkTSConfigDependencies = common.arktsconfig.ArkTSConfig['compilerOptions']['dependencies'];

interface DepAnalyzerResult {
  readonly dependencyMap: DependencyFileMap;
  readonly arktsConfigDependencies: ArkTSConfigDependencies;
}

interface ClassifiedDependency {
  readonly filePath: string;
  readonly type: NodeType;
  readonly isSentinel: boolean;
}

/**
 * Resolves the dependency graph of the project's static ArkTS files using the
 * `dep_analyzer` tool shipped with the Panda SDK.
 *
 * An arktsconfig supplied by the caller is passed to `dep_analyzer`, whose
 * output is turned into {@link DependencyNode}s.
 * References that point at dynamic (1.1) files are recorded as
 * dynamic nodes with `isSentinel` set so the dynamic resolver can stitch them
 * in later. References that resolve into the dynamic interop SDK are
 * relocated to the same-named dynamic SDK declaration first.
 */
export class StaticResolver extends PartialResolver {
  constructor(
    private readonly depAnalyzerPath: string,
    private readonly arktsConfigPath: string,
  ) {
    super();
  }

  /**
   * Resolve every static file in the project into a partial dependency map.
   *
   * @returns The static nodes plus the dynamic files they reference (sentinels).
   * @throws If `dep_analyzer` cannot be run or its output cannot be parsed.
   */
  resolve(entryFiles: string[]): PartialResolvedDependencyMap {
    const nodes = new Map<string, DependencyNode>();
    const sentinels = new Set<string>();

    if (!this.validateEntryFiles(entryFiles)) {
      throw new Error('One or more entry files are not part of the static file set.');
    }

    const staticFiles = this.context.fileManager.staticFiles;
    if (entryFiles.length === 0) {
      return { nodes, sentinels: [] };
    }

    const { dependencyMap, arktsConfigDependencies } = this.runDepAnalyzer(entryFiles);

    // Seed a node for every static file so isolated files still appear.
    for (const file of entryFiles) {
      this.ensureNode(nodes, file, NodeType.STATIC);
    }

    for (const [rawFile, rawDeps] of Object.entries(dependencyMap.dependencies)) {
      const fileKey = common.fileUtils.normalizePath(rawFile);
      if (!staticFiles.has(fileKey)) {
        continue; // dep_analyzer may report files outside our static scope.
      }
      const node = this.ensureNode(nodes, fileKey, NodeType.STATIC);

      for (const rawDep of rawDeps) {
        const dependency = this.classify(rawDep, arktsConfigDependencies);
        if (dependency === undefined || dependency.filePath === fileKey) {
          continue;
        }
        this.ensureNode(nodes, dependency.filePath, dependency.type, dependency.isSentinel);
        if (dependency.isSentinel) {
          sentinels.add(dependency.filePath);
        }
        if (!node.dependencies.includes(dependency.filePath)) {
          node.dependencies.push(dependency.filePath);
        }
      }
    }

    return { nodes, sentinels: [...sentinels] };
  }

  private validateEntryFiles(entryFiles: string[]): boolean {
    const staticFiles = this.context.fileManager.staticFiles;
    for (const file of entryFiles) {
      if (!staticFiles.has(file)) {
        return false;
      }
    }
    return true;
  }

  /** Classify a native dependency key as an in-language dependency or cross-language sentinel. */
  private classify(key: string, dependencies: ArkTSConfigDependencies): ClassifiedDependency | undefined {
    const normalizedPath = common.fileUtils.normalizePath(key);
    if (this.context.fileManager.staticFiles.has(normalizedPath)) {
      return { filePath: normalizedPath, type: NodeType.STATIC, isSentinel: false };
    }

    const dependency = dependencies[key];
    if (dependency === undefined) {
      return undefined;
    }
    if (dependency.sourceFilePath !== undefined) {
      const sourceFilePath = common.fileUtils.normalizePath(dependency.sourceFilePath);
      return this.context.fileManager.dynamicFiles.has(sourceFilePath)
        ? { filePath: sourceFilePath, type: NodeType.DYNAMIC, isSentinel: true }
        : undefined;
    }

    const interopSdkPath = common.fileUtils.normalizePath(dependency.path);
    if (!this.context.fileManager.isDynamicInteropSdkFile(interopSdkPath)) {
      return undefined;
    }
    // The dynamic interop SDK only mirrors dynamic SDK content for static
    // consumption; relocate to the same-named dynamic SDK declaration so the
    // dependency edge points at a file the dynamic resolver can produce a real
    // node for. The interop SDK file remains the fallback when the dynamic SDK
    // has no counterpart.
    const dynamicSdkPath = this.context.fileManager.queryDynamicSdkPathForFile(interopSdkPath);
    return { filePath: dynamicSdkPath ?? interopSdkPath, type: NodeType.DYNAMIC, isSentinel: true };
  }

  /**
   * Write the dep_analyzer input list, run the tool, and parse its JSON output.
   *
   * @throws If the tool fails to run or its output cannot be read/parsed.
   */
  private runDepAnalyzer(entryFiles: string[]): DepAnalyzerResult {
    const workDir = this.getWorkingDir();
    const inputFile = path.resolve(workDir, DEP_ANALYZER_INPUT_FILE);
    const outputFile = path.resolve(workDir, DEP_ANALYZER_OUTPUT_FILE);

    fs.writeFileSync(inputFile, entryFiles.join(os.EOL));
    const arktsConfigDependencies = this.readArkTSConfigDependencies(this.arktsConfigPath);

    const command = this.formExecCommand(inputFile, outputFile, this.arktsConfigPath);
    try {
      child_process.execSync(command, { stdio: 'pipe', encoding: 'utf-8' });
    } catch (error) {
      const execError = error as Error & { stdout?: string | Buffer; stderr?: string | Buffer };
      let detail = execError.message;
      if (execError.stderr) {
        detail += `\nStdErr: ${execError.stderr.toString()}`;
      }
      if (execError.stdout) {
        detail += `\nStdOut: ${execError.stdout.toString()}`;
      }
      throw new Error(`Failed to analyze static dependencies.\n${detail}`);
    }

    const content = fs.readFileSync(outputFile, 'utf-8');
    const dependencyMap = JSON.parse(content) as DependencyFileMap;
    // dep_analyzer may list a file only under dependants; normalize it in.
    for (const file of Object.keys(dependencyMap.dependants)) {
      if (!(file in dependencyMap.dependencies)) {
        dependencyMap.dependencies[file] = [];
      }
    }
    return { dependencyMap, arktsConfigDependencies };
  }

  private readArkTSConfigDependencies(arktsConfigFile: string): ArkTSConfigDependencies {
    const config = JSON.parse(fs.readFileSync(arktsConfigFile, 'utf-8')) as common.arktsconfig.ArkTSConfig;
    return config.compilerOptions.dependencies;
  }

  /** Build the dep_analyzer command line, adding the dylib path on macOS. */
  private formExecCommand(inputFile: string, outputFile: string, arktsConfigFile: string): string {
    const parts = [
      `"${path.resolve(this.depAnalyzerPath)}"`,
      `@"${inputFile}"`,
      `--arktsconfig="${arktsConfigFile}"`,
      `--output="${outputFile}"`,
    ];
    let command = parts.join(' ');
    if (os.type() === 'Darwin') {
      command = `DYLD_LIBRARY_PATH="${process.env.DYLD_LIBRARY_PATH ?? ''}" ${command}`;
    }
    return command;
  }

  /**
   * Working directory for dep_analyzer artifacts, created if needed. Prefers the
   * entry module's cache, falling back to the system temp directory.
   *
   * @throws If the directory cannot be created.
   */
  private getWorkingDir(): string {
    const cachePath = this.context.cachePath;
    const workDir = cachePath
      ? path.resolve(cachePath, WORK_DIR_NAME)
      : path.resolve(os.tmpdir(), 'interop-toolkits', WORK_DIR_NAME);
    fs.mkdirSync(workDir, { recursive: true });
    return workDir;
  }

  private ensureNode(
    nodes: Map<string, DependencyNode>,
    key: string,
    type: NodeType,
    isSentinel = false,
  ): DependencyNode {
    let node = nodes.get(key);
    if (node === undefined) {
      node = { fileName: key, type, isSentinel, isResolved: !isSentinel, dependencies: [], dependants: [] };
      nodes.set(key, node);
    } else {
      node.isSentinel ||= isSentinel;
      node.isResolved ||= !isSentinel;
    }
    return node;
  }
}
