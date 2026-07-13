/**
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

import * as os from 'os';
import * as fs from 'fs';
import * as path from 'path';
import * as child_process from 'child_process';
import { ArkTSConfigGenerator } from '../build/generate_arktsconfig';
import {
    DECL_ETS_SUFFIX,
    DEP_ANALYZER_OUTPUT_FILE,
    ENABLE_CLUSTERS,
    ENABLE_DECL_FILE_CACHE,
    INCRE_COMPILE_FILE,
    INCRE_DEP_ANALYZER_INPUT_FILE,
    INCRE_DEP_ANALYZER_OUTPUT_FILE
} from '../pre_define';
import { BuildConfig, isHarOrHsp, ModuleInfo } from '../types';
import {
    DepAnalyzer,
    DependencyFileMap,
    DepGraphContext,
    MakeEmptyDepFileMap
} from './dep_analyzer';
import { changeFileExtension, shouldBeUpdated, updateFileHash } from '../util/utils';
import { DriverError, ErrorCode } from '../util/error';
import { LogDataFactory } from '../logger';

enum IncreDepAnalyzerEvent {
    INCRE_UPD_DELETED_FILES = 'Incre upd deleted files',
    INCRE_FIND_MODIFIED_FILES = 'Incre find modified files',
    INCRE_CALL_BIN = 'Incre call depAna bin',
    INCRE_UPD_MAP = 'Incre upd depMap',
    INCRE_GEN_COMPILE_MAP = 'Incre gen compile map'
}

function formEvent(event: IncreDepAnalyzerEvent): string {
    return event;
}

/**
 * incre dep result from incre graph traversed
 */
class IncreDepMap {
    private dependencies: Record<string, Set<string>>;
    private dependants: Record<string, Set<string>>;
    private outputMatching: Record<string, string>;

    constructor() {
        this.dependencies = {};
        this.dependants = {};
        this.outputMatching = {};
    }

    public addDep(importer: string, importee: string): void {
        if (!this.dependencies[importer]) {
            this.dependencies[importer] = new Set<string>();
        }
        this.dependencies[importer].add(importee);

        if (!this.dependants[importee]) {
            this.dependants[importee] = new Set<string>();
        }
        this.dependants[importee].add(importer);
    }

    public addOutputMatching(input: string, output: string): void {
        this.outputMatching[input] = output;
    }

    public toDepFileMap(): DependencyFileMap {
        const dependencies: Record<string, string[]> = {};
        const dependants: Record<string, string[]> = {};

        for (const filePath in this.dependencies) {
            dependencies[filePath] = Array.from(this.dependencies[filePath]);
        }

        for (const filePath in this.dependants) {
            dependants[filePath] = Array.from(this.dependants[filePath]);
        }

        return {
            dependencies,
            dependants,
            outputMatching: this.outputMatching
        };
    }
}

/**
 * Graph consist of incre affected source files
 */
class IncreGraph {
    private depMap: DependencyFileMap;
    private increDepMap: IncreDepMap;
    private visited: Set<string>;

    constructor(depMap: DependencyFileMap) {
        this.depMap = depMap;
        this.increDepMap = new IncreDepMap();
        this.visited = new Set<string>();
    }

    /**
     * traverse the incre graph from source file located at file
     * @param file 
     * @returns 
     */
    private traverse(file: string): void {
        if (this.visited.has(file)) {
            return;
        }

        const queue: string[] = [file];
        this.visited.add(file);

        while (queue.length > 0) {
            // 1. visit the node
            const importee = queue.shift()!;
            this.increDepMap.addOutputMatching(importee, this.depMap.outputMatching[importee]);
            const importers: string[] = this.depMap.dependants[importee] ?? [];

            for (const importer of importers) {
                this.increDepMap.addDep(importer, importee);
                this.increDepMap.addOutputMatching(importer, this.depMap.outputMatching[importer]);
            }

            // 2. add its neighbors to queue
            for (const importer of importers) {
                if (!this.visited.has(importer)) {
                    this.visited.add(importer);
                    queue.push(importer);
                }
            }
        }
    }

    /**
     * traverse the incre graph from source files
     */
    public traverseAll(fileList: string[]): Set<string> {
        for (const file of fileList) {
            this.traverse(file);
        }
        return this.visited;
    }

    public increToDepFileMap(): DependencyFileMap {
        return this.increDepMap.toDepFileMap();
    }
}

/**
 * Dependency Analyzer when incremental build
 */
export class IncreDepAnalyzer extends DepAnalyzer {
    // has source files modified or added
    private hasModified: boolean = false;

    constructor(
        buildConfig: BuildConfig,
        generator: ArkTSConfigGenerator,
        clusteredBuild: boolean = ENABLE_CLUSTERS
    ) {
        super(buildConfig, generator, clusteredBuild);
    }

    private deleteRelatedDepsInDependants(deletedFile: string, relatedDeps: string[], prevDepMap: DependencyFileMap): void {
        for (const depFile of relatedDeps) {
            if (prevDepMap.dependants[depFile]) {
                prevDepMap.dependants[depFile] = prevDepMap.dependants[depFile].filter(item => item !== deletedFile);
                if (prevDepMap.dependants[depFile].length === 0) {
                    delete prevDepMap.dependants[depFile];
                }
            }
        }
    }

    private updateFullDependencyWithDeletedFiles(prevDepMap: DependencyFileMap, deletedFiles: string[]): void {
        for (const deletedFile of deletedFiles) {
            delete prevDepMap.outputMatching[deletedFile];
            const relatedDeps = prevDepMap.dependencies[deletedFile] || [];
            delete prevDepMap.dependencies[deletedFile];

            this.deleteRelatedDepsInDependants(deletedFile, relatedDeps, prevDepMap);
            delete prevDepMap.dependants[deletedFile];
        }
    }

    private fileChanged(filePath: string, fileToModule: Map<string, ModuleInfo>, depMap: DependencyFileMap): boolean {
        this.getOrInitFileChangeStatus(filePath);
        const hashChanged: boolean = updateFileHash(filePath, this.filesHashCache);
        this.setFileHashChanged(filePath, hashChanged);
        if (hashChanged) {
            return true;
        }

        const abcOutdated: boolean = shouldBeUpdated(filePath, depMap.outputMatching[filePath]);
        this.setAbcOutdatedChanged(filePath, abcOutdated);
        if (abcOutdated) {
            return true;
        }

        if (ENABLE_DECL_FILE_CACHE && isHarOrHsp(this.mainModuleType)) {
            const module = fileToModule.get(filePath);
            const relative: string = changeFileExtension(
                path.relative(module?.moduleRootPath!, filePath),
                DECL_ETS_SUFFIX
            );
            const declEtsOutputPath: string = path.resolve(this.declgenV2OutDir, relative);
            const declOutdated: boolean = shouldBeUpdated(filePath, declEtsOutputPath);
            this.setDeclOutdatedChanged(filePath, declOutdated);
            if (declOutdated) {
                return true;
            }
        }

        return false;
    }

    private findModifiedSourceFiles(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        depMap: DependencyFileMap
    ): string[] {
        const modifiedFiles: string[] = [];
        for (const filePath of entryFiles) {
            const fileChangedFlag: boolean = this.fileChanged(filePath, fileToModule, depMap);
            if (fileChangedFlag) {
                modifiedFiles.push(filePath);
            }
        }

        if (modifiedFiles.length > 0) {
            const increInputFile: string = path.join(this.outputDir, INCRE_DEP_ANALYZER_INPUT_FILE);
            const fileContent: string = modifiedFiles.join(os.EOL);
            fs.writeFileSync(increInputFile, fileContent);
        }
        return modifiedFiles;
    }

    private updateDependencyItem(
        increDepFileList: string[],
        prevDepMap: DependencyFileMap,
        prevDepList: string[],
        filePath: string
    ): void {
        const prevSet = new Set(prevDepList);
        const increSet = new Set(increDepFileList);

        const addedFiles = increDepFileList.filter(item => !prevSet.has(item));
        const removedFiles = prevDepList.filter(item => !increSet.has(item));

        prevDepMap.dependencies[filePath] = increDepFileList;

        for (const removedFile of removedFiles) {
            if (prevDepMap.dependants[removedFile]) {
                prevDepMap.dependants[removedFile] = prevDepMap.dependants[removedFile].filter(f => f !== filePath);
            }
        }

        for (const addedFile of addedFiles) {
            if (!prevDepMap.dependants[addedFile]) {
                prevDepMap.dependants[addedFile] = [];
            }
            if (!prevDepMap.dependants[addedFile].includes(filePath)) {
                prevDepMap.dependants[addedFile].push(filePath);
            }
        }
    }

    private updateFullDependency(prevDepMap: DependencyFileMap, modules: ModuleInfo[], modifiedSourceFiles: string[]): void {
        if (!this.hasModified) {
            return;
        }

        // 1. generate incremental dependency json file
        const arktsConfigPath: string = this.mergedArktsConfigPath;
        this.generateMergedArktsConfig(modules, arktsConfigPath);

        const increInputFile: string = path.join(this.outputDir, INCRE_DEP_ANALYZER_INPUT_FILE);
        const increOutputFile: string = path.join(this.outputDir, INCRE_DEP_ANALYZER_OUTPUT_FILE);
        const execCmd = this.formExecCmd(increInputFile, increOutputFile, arktsConfigPath);

        this.statsRecorder.record(formEvent(IncreDepAnalyzerEvent.INCRE_CALL_BIN));
        try {
            child_process.execSync(execCmd, {
                stdio: 'pipe',
                encoding: 'utf-8'
            });
        } catch (error) {
            if (error instanceof Error) {
                const execError = error as child_process.ExecException;
                let fullErrorMessage = execError.message;
                if (execError.stderr) {
                    fullErrorMessage += `\nStdErr: ${execError.stderr}`;
                }
                if (execError.stdout) {
                    fullErrorMessage += `\nStdOutput: ${execError.stdout}`;
                }
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                        'Failed to analyze incre files dependency.',
                        fullErrorMessage
                    )
                );
            }
            throw error;
        }

        // 2. update prevDepMap with incremental result
        this.statsRecorder.record(formEvent(IncreDepAnalyzerEvent.INCRE_UPD_MAP));
        const increDepMap: DependencyFileMap = JSON.parse(fs.readFileSync(increOutputFile, 'utf-8'));
        for (const [filePath, increDepFileList] of Object.entries(increDepMap.dependencies)) {
            const prevDepList = prevDepMap.dependencies[filePath] || [];
            this.updateDependencyItem(increDepFileList, prevDepMap, prevDepList, filePath);
        }

        // 3. if file in outputMatching and not in dependencies, it means that there is no import in file
        for (const modifiedFile of modifiedSourceFiles) {
            if (!increDepMap.dependencies[modifiedFile]) {
                const prevDepList = prevDepMap.dependencies[modifiedFile] || [];
                this.updateDependencyItem([], prevDepMap, prevDepList, modifiedFile);
            }
        }

        for (const [filePath, outputPath] of Object.entries(increDepMap.outputMatching)) {
            prevDepMap.outputMatching[filePath] = outputPath;
        }
    }

    protected generateDependencyMap(
        entryFiles: Set<string>,
        modules: ModuleInfo[],
        fileToModule: Map<string, ModuleInfo>
    ): DependencyFileMap {
        // 1. update prevDepMap with deletedFiles
        this.statsRecorder.record(formEvent(IncreDepAnalyzerEvent.INCRE_UPD_DELETED_FILES));
        const prevDepFile: string = path.join(this.outputDir, DEP_ANALYZER_OUTPUT_FILE);
        const prevDepMap: DependencyFileMap = JSON.parse(fs.readFileSync(prevDepFile, 'utf-8'));
        this.updateFullDependencyWithDeletedFiles(prevDepMap, this.deletedFiles);

        // 2. find all the added and modified source files and update prevDepMap
        this.statsRecorder.record(formEvent(IncreDepAnalyzerEvent.INCRE_FIND_MODIFIED_FILES));
        let modifiedSourceFiles: string[] = this.findModifiedSourceFiles(entryFiles, fileToModule, prevDepMap);
        this.hasModified = modifiedSourceFiles.length > 0;
        this.updateFullDependency(prevDepMap, modules, modifiedSourceFiles);

        // 3. persist the prevDepMap to disk
        if (this.deletedFiles.length > 0 || this.hasModified) {
            fs.writeFileSync(prevDepFile, JSON.stringify(prevDepMap, null, 2), 'utf-8');
        }

        return this.filterDependencyMap(prevDepMap, entryFiles);
    }

    protected createDepGraphContext(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        depMap: DependencyFileMap
    ): DepGraphContext {
        this.statsRecorder.record(formEvent(IncreDepAnalyzerEvent.INCRE_GEN_COMPILE_MAP));
        let filesTobeCompiled: Set<string> = new Set<string>();
        if (!this.hasModified) {
            return {
                entryFiles: filesTobeCompiled,
                fileToModule,
                dependencyMap: MakeEmptyDepFileMap()
            };
        }

        const increInputFile: string = path.join(this.outputDir, INCRE_DEP_ANALYZER_INPUT_FILE);
        const content = fs.readFileSync(path.resolve(increInputFile), 'utf-8');
        const increFiles: string[] = content
            .split(os.EOL)
            .map(line => line.trim())
            .filter(line => line.length > 0);

        // Traverse from modified entry files to collect all affected compile files
        const increGraph: IncreGraph = new IncreGraph(depMap);
        filesTobeCompiled = increGraph.traverseAll(increFiles);
        const increCompileMap: DependencyFileMap = increGraph.increToDepFileMap();

        // Write incremental compile dependency map to file
        const increCompileFile: string = path.join(this.outputDir, INCRE_COMPILE_FILE);
        fs.writeFileSync(increCompileFile, JSON.stringify(increCompileMap, null, 2), 'utf-8');

        return {
            entryFiles: filesTobeCompiled,
            fileToModule,
            dependencyMap: increCompileMap
        };
    }
}