/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import {
  global,
  Es2pandaContextState,
  EtsNativePointer,
  nullptr
} from '@arkts-bindings/index';
import { BuildConfig, CompileJobInfo, CompileJobType, FileInfo, JobContentType, BUILD_MODE } from '../types';
import { MERGED_INTERMEDIATE_FILE } from '../pre_define';
import { Logger, LogDataFactory } from '../logger';
import { DriverError, ErrorCode } from './error';

export class Ets2pandaContextHandle {
  config: EtsNativePointer = nullptr;
  context: EtsNativePointer = nullptr;
}

export class Ets2panda {
  private static instance: Ets2panda | undefined = undefined;
  private readonly buildConfig: BuildConfig;
  private readonly logger: Logger = Logger.getInstance();
  private readonly debugBuild: boolean;
  private readonly dumpPerf: boolean;

  private constructor(buildConfig: BuildConfig) {
    this.buildConfig = buildConfig;
    this.debugBuild = buildConfig.buildMode === BUILD_MODE.DEBUG;
    this.dumpPerf = buildConfig.dumpPerf ?? false;
  }

  public static getInstance(buildConfig: BuildConfig | undefined = undefined): Ets2panda {
    if (Ets2panda.instance === undefined) {
      if (buildConfig === undefined) {
        throw new Error('buildConfig is required for the first Ets2panda instantiation.');
      }
      Ets2panda.instance = new Ets2panda(buildConfig);
    }
    return Ets2panda.instance as Ets2panda;
  }

  public static destroyInstance(): void {
    Ets2panda.instance = undefined;
  }

  public initalize(): void {
    this.initialize();
  }

  public initialize(): void {
    global.es2panda._MemInitialize();
  }

  public finalize(): void {
    global.es2panda._MemFinalize();
  }

  public createConfig(args: string[]): EtsNativePointer {
    const config = global.es2panda._CreateConfig(args.length, args);
    if (config === nullptr) {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL, 'Create es2panda config failed.'));
    }
    return config;
  }

  public destroyConfig(config: EtsNativePointer): void {
    if (config !== nullptr) {
      global.es2panda._DestroyConfig(config);
    }
  }

  public createContextFromFile(config: EtsNativePointer, filename: string): EtsNativePointer {
    const context = global.es2panda._CreateContextFromFile(config, filename);
    this.checkContext(context, filename);
    return context;
  }

  public createContextFromString(config: EtsNativePointer, source: string, filename: string): EtsNativePointer {
    const context = global.es2panda._CreateContextFromString(config, source, filename);
    this.checkContext(context, filename);
    return context;
  }

  public createContextForFiles(config: EtsNativePointer, inputFiles: string[]): EtsNativePointer {
    if (inputFiles.length === 1) {
      return this.createContextFromFile(config, inputFiles[0]);
    }
    const context = global.es2panda._CreateContextGenerateAbcForExternalSourceFiles(config, inputFiles.length, inputFiles);
    this.checkContext(context, inputFiles.join(','));
    return context;
  }

  public proceedToState(context: EtsNativePointer, state: Es2pandaContextState): EtsNativePointer {
    const nextContext = global.es2panda._ProceedToState(context, state);
    const errorMessage = global.es2panda._GetAllErrorMessages(nextContext);
    if (errorMessage !== '') {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL, 'Proceed es2panda state failed.', errorMessage));
    }
    return nextContext;
  }

  public compile(jobId: string, job: CompileJobInfo, incremental: boolean = false, declGenCb: (() => void) | undefined = undefined, compAbcCb: (() => void) | undefined = undefined): void {
    const args = this.formCompileCliCmd(job, incremental);
    const inputs = this.collectInputs(job);
    let config: EtsNativePointer = nullptr;
    let context: EtsNativePointer = nullptr;
    try {
      config = this.createConfig(args);
      context = this.createContextForFiles(config, inputs);
      context = this.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_PARSED);
      if ((job.jobType & CompileJobType.DECL) !== 0) {
        declGenCb?.();
      }
      context = this.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_CHECKED);
      if ((job.jobType & CompileJobType.ABC) !== 0) {
        context = this.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_ASM_GENERATED);
        global.es2panda._FreeCompilerPartMemory(context);
        context = this.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED);
        compAbcCb?.();
      }
      this.logger.printInfo('[Ets2panda] Compiled abc file for cycle ' + jobId);
    } finally {
      if (context !== nullptr) {
        global.es2panda._DestroyContext(context);
      }
      if (config !== nullptr) {
        this.destroyConfig(config);
      }
    }
  }

  private checkContext(context: EtsNativePointer, target: string): void {
    if (context === nullptr) {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL, 'Create es2panda context failed.', target));
    }
    const errorMessage = global.es2panda._ContextErrorMessage(context);
    if (errorMessage !== '') {
      throw new DriverError(LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL, 'Create es2panda context failed.', errorMessage, target));
    }
  }

  private collectInputs(job: CompileJobInfo): string[] {
    if (job.contentType === JobContentType.FILE) {
      return [(job.content as FileInfo).input];
    }
    const files = job.content as FileInfo[];
    const inputs: string[] = [];
    for (const file of files) {
      inputs.push(file.input);
    }
    return inputs;
  }

  private formCompileCliCmd(job: CompileJobInfo, incremental: boolean = false): string[] {
    const cmd: string[] = ['_', '--extension', 'ets', '--arktsconfig', job.arktsConfig, '--simultaneous'];
    if (job.contentType === JobContentType.CLUSTER && incremental) {
      cmd.push('--incremental');
    }
    if ((job.jobType & CompileJobType.DECL) !== 0) {
      cmd.push('--emit-declaration');
      const cacheConfig = this.buildConfig.declFileNameCacheConfig;
      if (cacheConfig !== undefined) {
        cmd.push('--generate-decl:nameCachePath=' + cacheConfig.nameCachePath);
      }
    }
    if (job.contentType === JobContentType.FILE) {
      cmd.push('--output');
      cmd.push((job.content as FileInfo).output);
    } else if (job.contentType === JobContentType.CLUSTER && !incremental) {
      cmd.push('--output');
      cmd.push(this.joinPath(this.buildConfig.cachePath, MERGED_INTERMEDIATE_FILE));
    }
    if (this.debugBuild) {
      cmd.push('--debug-info');
      cmd.push('--opt-level=0');
    }
    cmd.push('--ets-warnings:diagnostic-format=build-system');
    if (job.contentType === JobContentType.FILE) {
      cmd.push((job.content as FileInfo).input);
    }
    if (this.dumpPerf) {
      cmd.push('--dump-perf-metrics');
    }
    return cmd;
  }

  private joinPath(lhs: string, rhs: string): string {
    if (lhs.endsWith('/')) {
      return lhs + rhs;
    }
    return lhs + '/' + rhs;
  }
}
