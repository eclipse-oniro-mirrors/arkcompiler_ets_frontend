import { global } from '@arkts-bindings';
import { getNativeLibraryPath } from '@arkts-bindings/NativeLibrary';
import { getEnvironmentVar } from '@arkts-bindings/Utils';
import { LogDataFactory } from '../logger';
import { DriverError, ErrorCode } from './error';
import { substituteEnvVarsInString } from './utils';

function driverError(code: string, description: string, cause: string = '', position: string = ''): DriverError {
  return new DriverError(LogDataFactory.newDetailedInstance(code, description, cause, position, [], ''));
}

export function readTextFile(path: string): string {
  return global.interop._ReadFile(path);
}

export function fileExists(path: string): boolean {
  return global.interop._FileExists(path);
}

export function ensureFileExists(path: string, reason: string): string {
  if (!fileExists(path)) {
    throw driverError(errorCodeForReason(reason), descriptionForReason(reason), reason, path);
  }
  return path;
}

export function ensurePathExists(path: string, reason: string): string {
  return ensureFileExists(path, reason);
}

export function fileSize(path: string): long {
  return global.interop._FileSize(path);
}

export function ensureNonEmptyBinaryFile(path: string, reason: string): string {
  if (!fileExists(path)) {
    throw driverError(ErrorCode.BUILDSYSTEM_MISSING_OUTPUT_ABC, 'Output ABC file does not exist.', 'missing-output-abc', path);
  }
  if (fileSize(path) === 0) {
    throw driverError(ErrorCode.BUILDSYSTEM_EMPTY_OUTPUT_ABC, 'Output ABC file is empty.', 'empty-output-abc', path);
  }
  return path;
}

export function substituteEnvVarsInJSON(text: string): string {
  return substituteEnvVarsInString(text, getEnvironmentVar);
}

export function resolveNativeCompilerLibraryPath(): string {
  let nativeLibraryPath: string = getNativeLibraryPath();
  if (nativeLibraryPath.length === 0 || !fileExists(nativeLibraryPath)) {
    throw driverError(ErrorCode.BUILDSYSTEM_MISSING_NATIVE_LIBRARY, 'Native compiler library does not exist.', 'missing-native-library', nativeLibraryPath);
  }
  return nativeLibraryPath;
}

function errorCodeForReason(reason: string): string {
  if (reason === 'missing-build-config') {
    return ErrorCode.BUILDSYSTEM_MISSING_BUILD_CONFIG;
  }
  if (reason === 'missing-module-path') {
    return ErrorCode.BUILDSYSTEM_MISSING_MODULE_PATH;
  }
  if (reason === 'missing-sdk-stub-path') {
    return ErrorCode.BUILDSYSTEM_SDK_NOT_EXIST_FAIL;
  }
  if (reason === 'missing-native-library') {
    return ErrorCode.BUILDSYSTEM_MISSING_NATIVE_LIBRARY;
  }
  if (reason === 'missing-output-abc') {
    return ErrorCode.BUILDSYSTEM_MISSING_OUTPUT_ABC;
  }
  if (reason === 'empty-output-abc') {
    return ErrorCode.BUILDSYSTEM_EMPTY_OUTPUT_ABC;
  }
  return ErrorCode.BUILDSYSTEM_PATH_RESOLVE_FAIL;
}

function descriptionForReason(reason: string): string {
  if (reason === 'missing-build-config') {
    return 'Build config file does not exist.';
  }
  if (reason === 'missing-module-path') {
    return 'Dependent module path does not exist.';
  }
  if (reason === 'missing-sdk-stub-path') {
    return 'SDK stub path does not exist.';
  }
  if (reason === 'missing-native-library') {
    return 'Native compiler library does not exist.';
  }
  if (reason === 'missing-output-abc') {
    return 'Output ABC file does not exist.';
  }
  if (reason === 'empty-output-abc') {
    return 'Output ABC file is empty.';
  }
  return 'Required path does not exist.';
}
