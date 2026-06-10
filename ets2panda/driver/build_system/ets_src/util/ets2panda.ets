import { global, Es2pandaContextState, EtsNativePointer, nullptr } from '@arkts-bindings';
import { LogDataFactory } from '../logger';
import { DriverError, ErrorCode } from './error';
import { ensureNonEmptyBinaryFile } from './interop_helper';

export class NativeCompilerConfigHandle {
  public value: EtsNativePointer;

  public constructor(value: EtsNativePointer) {
    this.value = value;
  }
}

export class NativeCompilerContextHandle {
  public value: EtsNativePointer;

  public constructor(value: EtsNativePointer) {
    this.value = value;
  }
}

export interface NativeCompilerConfigRequest {
  argv: string[];
  configText: string;
}

export interface CompileUnitRequest {
  configRequest: NativeCompilerConfigRequest;
  sourceFile: string;
  sourceFiles?: string[];
  outputPath: string;
  requestedStates?: Es2pandaContextState[];
}

export class Ets2panda {
  public compileFile(request: CompileUnitRequest): void {
    this.compile(request, false);
  }

  public compileExternalSourceSet(request: CompileUnitRequest): void {
    this.compile(request, true);
  }

  private compile(request: CompileUnitRequest, externalSourceSet: boolean): void {
    let memInitialized: boolean = false;
    let config: NativeCompilerConfigHandle = new NativeCompilerConfigHandle(nullptr);
    let context: NativeCompilerContextHandle = new NativeCompilerContextHandle(nullptr);
    let primaryError: DriverError | undefined = undefined;
    let cleanupError: string = '';
    try {
      this.initializeMemory(request.sourceFile);
      memInitialized = true;
      config = this.createConfig(request.configRequest, request.sourceFile);
      context = externalSourceSet ? this.createExternalSourceSetContext(config, request) : this.createFileContext(config, request.sourceFile);
      this.progressContext(context, this.statesForRequest(request), request.sourceFile);
      ensureNonEmptyBinaryFile(request.outputPath, 'empty-output-abc');
    } catch (error) {
      primaryError = this.asDriverError(error, request.sourceFile);
    } finally {
      cleanupError = this.cleanup(context, config, memInitialized, request.sourceFile);
    }
    if (primaryError !== undefined) {
      if (cleanupError.length > 0) {
        (primaryError as DriverError).logData.moreInfo = this.joinDetails((primaryError as DriverError).logData.moreInfo, 'cleanup-error: ' + cleanupError);
      }
      throw primaryError as DriverError;
    }
    if (cleanupError.length > 0) {
      throw this.nativeBindingError('native-cleanup-failed', cleanupError, request.sourceFile);
    }
  }

  private initializeMemory(sourceFile: string): void {
    try {
      global.es2panda._MemInitialize();
    } catch (error) {
      throw this.nativeBindingError('native-memory-initialize-failed', this.stringifyError(error), sourceFile);
    }
  }

  private createConfig(request: NativeCompilerConfigRequest, sourceFile: string): NativeCompilerConfigHandle {
    if (request.configText.length === 0) {
      throw this.nativeBindingError('missing-native-config-text', 'NativeCompilerConfigRequest.configText is empty.', sourceFile);
    }
    let argv: string[] = this.encodeConfigTextArgv(request);
    let config: EtsNativePointer = nullptr;
    try {
      config = global.es2panda._CreateConfig(argv.length, argv);
    } catch (error) {
      throw this.nativeBindingError('create-config-failed', this.stringifyError(error), sourceFile);
    }
    if (config === nullptr) {
      throw this.nativeBindingError(
        'arkts-bindings-config-text-api-gap',
        '_CreateConfig(argc, argv) could not consume NativeCompilerConfigRequest.configText through the in-process argv contract.',
        sourceFile
      );
    }
    return new NativeCompilerConfigHandle(config);
  }

  private encodeConfigTextArgv(request: NativeCompilerConfigRequest): string[] {
    let argv: string[] = request.argv.slice();
    if (argv.length === 0) {
      argv.push('_');
    }
    if (!this.hasInProcessConfigPayload(argv)) {
      argv.push('--arktsconfig=arktsconfig-json:' + request.configText);
    }
    return argv;
  }

  private hasInProcessConfigPayload(argv: string[]): boolean {
    for (let i: int = 0; i < argv.length; i++) {
      if (argv[i].startsWith('--arktsconfig=arktsconfig-json:')) {
        return true;
      }
    }
    return false;
  }

  private createFileContext(config: NativeCompilerConfigHandle, sourceFile: string): NativeCompilerContextHandle {
    let context: EtsNativePointer = nullptr;
    try {
      context = global.es2panda._CreateContextFromFile(config.value, sourceFile);
    } catch (error) {
      throw this.nativeBindingError('create-context-failed', this.stringifyError(error), sourceFile);
    }
    this.checkCreatedContext(context, sourceFile);
    return new NativeCompilerContextHandle(context);
  }

  private createExternalSourceSetContext(config: NativeCompilerConfigHandle, request: CompileUnitRequest): NativeCompilerContextHandle {
    let files: string[] = request.sourceFiles === undefined ? [request.sourceFile] : request.sourceFiles as string[];
    if (files.length === 0) {
      throw this.nativeBindingError('missing-external-source-files', 'compileExternalSourceSet requires at least one source file.', request.sourceFile);
    }
    let context: EtsNativePointer = nullptr;
    try {
      context = global.es2panda._CreateContextGenerateAbcForExternalSourceFiles(config.value, files.length, files);
    } catch (error) {
      throw this.nativeBindingError('create-external-source-set-context-failed', this.stringifyError(error), files.join(','));
    }
    this.checkCreatedContext(context, files.join(','));
    return new NativeCompilerContextHandle(context);
  }

  private checkCreatedContext(context: EtsNativePointer, sourceFile: string): void {
    if (context === nullptr) {
      throw this.compilerStateError(sourceFile, Es2pandaContextState.ES2PANDA_STATE_NEW, Es2pandaContextState.ES2PANDA_STATE_ERROR, 'Create native es2panda context failed.', '');
    }
    let errorMessage: string = global.es2panda._ContextErrorMessage(context);
    let allErrors: string = global.es2panda._GetAllErrorMessages(context);
    if (errorMessage.length > 0 || allErrors.length > 0) {
      throw this.compilerStateError(sourceFile, Es2pandaContextState.ES2PANDA_STATE_NEW, global.es2panda._ContextState(context), errorMessage, allErrors);
    }
  }

  private progressContext(context: NativeCompilerContextHandle, states: Es2pandaContextState[], sourceFile: string): void {
    for (let i: int = 0; i < states.length; i++) {
      let requestedState: Es2pandaContextState = states[i];
      let nextContext: EtsNativePointer = nullptr;
      try {
        nextContext = global.es2panda._ProceedToState(context.value, requestedState);
      } catch (error) {
        throw this.compilerStateError(sourceFile, requestedState, this.safeContextState(context.value), this.stringifyError(error), this.safeAllErrorMessages(context.value));
      }
      if (nextContext !== nullptr) {
        context.value = nextContext;
      }
      let observedState: int = this.safeContextState(context.value);
      let errorMessage: string = this.safeContextErrorMessage(context.value);
      let allErrors: string = this.safeAllErrorMessages(context.value);
      if (observedState !== requestedState || errorMessage.length > 0 || allErrors.length > 0) {
        throw this.compilerStateError(sourceFile, requestedState, observedState, errorMessage, allErrors);
      }
    }
  }

  private statesForRequest(request: CompileUnitRequest): Es2pandaContextState[] {
    if (request.requestedStates !== undefined) {
      return request.requestedStates as Es2pandaContextState[];
    }
    return [
      Es2pandaContextState.ES2PANDA_STATE_PARSED,
      Es2pandaContextState.ES2PANDA_STATE_BOUND,
      Es2pandaContextState.ES2PANDA_STATE_CHECKED,
      Es2pandaContextState.ES2PANDA_STATE_ASM_GENERATED,
      Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED,
    ];
  }

  private cleanup(context: NativeCompilerContextHandle, config: NativeCompilerConfigHandle, memInitialized: boolean, sourceFile: string): string {
    let cleanupError: string = '';
    if (context.value !== nullptr) {
      try {
        global.es2panda._DestroyContext(context.value);
        context.value = nullptr;
      } catch (error) {
        cleanupError = this.joinDetails(cleanupError, 'destroy-context: ' + this.stringifyError(error));
      }
    }
    if (config.value !== nullptr) {
      try {
        global.es2panda._DestroyConfig(config.value);
        config.value = nullptr;
      } catch (error) {
        cleanupError = this.joinDetails(cleanupError, 'destroy-config: ' + this.stringifyError(error));
      }
    }
    if (memInitialized) {
      try {
        global.es2panda._MemFinalize();
      } catch (error) {
        cleanupError = this.joinDetails(cleanupError, 'mem-finalize: ' + this.stringifyError(error));
      }
    }
    if (cleanupError.length > 0) {
      return sourceFile + ': ' + cleanupError;
    }
    return cleanupError;
  }

  private safeContextState(context: EtsNativePointer): int {
    if (context === nullptr) {
      return Es2pandaContextState.ES2PANDA_STATE_ERROR;
    }
    try {
      return global.es2panda._ContextState(context);
    } catch (error) {
      return Es2pandaContextState.ES2PANDA_STATE_ERROR;
    }
  }

  private safeContextErrorMessage(context: EtsNativePointer): string {
    if (context === nullptr) {
      return '';
    }
    try {
      return global.es2panda._ContextErrorMessage(context);
    } catch (error) {
      return this.stringifyError(error);
    }
  }

  private safeAllErrorMessages(context: EtsNativePointer): string {
    if (context === nullptr) {
      return '';
    }
    try {
      return global.es2panda._GetAllErrorMessages(context);
    } catch (error) {
      return this.stringifyError(error);
    }
  }

  private compilerStateError(sourceFile: string, requestedState: int, observedState: int, errorMessage: string, allErrors: string): DriverError {
    let details: string = 'sourceFile=' + sourceFile + '\nrequestedState=' + requestedState.toString() + '\nobservedState=' + observedState.toString();
    if (errorMessage.length > 0) {
      details += '\ncontextErrorMessage=' + errorMessage;
    }
    if (allErrors.length > 0) {
      details += '\nallErrorMessages=' + allErrors;
    }
    return new DriverError(LogDataFactory.newDetailedInstance(
      ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
      'Native es2panda compilation failed.',
      details,
      sourceFile,
      [],
      ''
    ));
  }

  private nativeBindingError(reason: string, detail: string, position: string): DriverError {
    return new DriverError(LogDataFactory.newDetailedInstance(
      ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
      'Native es2panda binding failed.',
      reason + ': ' + detail,
      position,
      [],
      ''
    ));
  }

  private asDriverError(error: Object, position: string): DriverError {
    if (error instanceof DriverError) {
      return error as DriverError;
    }
    return this.nativeBindingError('unexpected-native-adapter-error', this.stringifyError(error), position);
  }

  private joinDetails(lhs: string, rhs: string): string {
    if (lhs.length === 0) {
      return rhs;
    }
    if (rhs.length === 0) {
      return lhs;
    }
    return lhs + '\n' + rhs;
  }

  private stringifyError(error: Object): string {
    return error.toString();
  }
}
