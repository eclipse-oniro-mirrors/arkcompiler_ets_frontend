/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 */

import { ErrorCode } from './util/error';
import { BuildConfig } from './types';

export enum SubsystemCode {
  ARK_GUARD = '113',
  BUILDSYSTEM = '114',
  ES2PANDA = '115',
  SDK = '117'
}

export interface ILogger {
  printInfo(message: string): void;
  printWarn(message: string): void;
  printDebug(message: string): void;
  printError(error: LogData): void;
  printErrorAndExit(error: LogData): void;
}

export type LoggerGetter = (code: SubsystemCode) => ILogger;

export class Logger {
  private static instance: Logger | undefined = undefined;
  private enableDebugOutput: boolean = false;
  private loggerMap: Map<SubsystemCode, ILogger> = new Map<SubsystemCode, ILogger>();
  private hasErrorOccurred: boolean = false;

  private constructor(loggerGetter: LoggerGetter, enableDebugOutput: boolean | undefined) {
    this.enableDebugOutput = enableDebugOutput ?? false;
    this.loggerMap.set(SubsystemCode.ARK_GUARD, loggerGetter(SubsystemCode.ARK_GUARD));
    this.loggerMap.set(SubsystemCode.BUILDSYSTEM, loggerGetter(SubsystemCode.BUILDSYSTEM));
    this.loggerMap.set(SubsystemCode.ES2PANDA, loggerGetter(SubsystemCode.ES2PANDA));
    this.loggerMap.set(SubsystemCode.SDK, loggerGetter(SubsystemCode.SDK));
  }

  public static getInstance(loggerGetter: LoggerGetter | undefined = undefined, enableDebugOutput: boolean | undefined = undefined): Logger {
    if (Logger.instance === undefined) {
      if (loggerGetter === undefined) {
        throw new Error('loggerGetter is required for the first Logger instantiation.');
      }
      Logger.instance = new Logger(loggerGetter, enableDebugOutput);
    }
    return Logger.instance as Logger;
  }

  public static destroyInstance(): void {
    Logger.instance = undefined;
  }

  public printInfo(message: string, subsystemCode: SubsystemCode = SubsystemCode.BUILDSYSTEM): void {
    this.getLoggerFromSubsystemCode(subsystemCode).printInfo('[time:' + Date.now().toString() + '] ' + message);
  }

  public printWarn(message: string, subsystemCode: SubsystemCode = SubsystemCode.BUILDSYSTEM): void {
    this.getLoggerFromSubsystemCode(subsystemCode).printWarn('[time:' + Date.now().toString() + '] ' + message);
  }

  public printDebug(message: string, subsystemCode: SubsystemCode = SubsystemCode.BUILDSYSTEM): void {
    if (!this.enableDebugOutput) {
      return;
    }
    this.getLoggerFromSubsystemCode(subsystemCode).printDebug('[time:' + Date.now().toString() + '] ' + message);
  }

  public printError(error: LogData): void {
    this.hasErrorOccurred = true;
    this.getLoggerFromErrorCode(error.code).printError(error);
  }

  public printErrorAndExit(error: LogData): void {
    this.hasErrorOccurred = true;
    this.getLoggerFromErrorCode(error.code).printErrorAndExit(error);
  }

  protected isValidErrorCode(errorCode: ErrorCode): boolean {
    return errorCode.valueOf().length === 8;
  }

  protected getLoggerFromErrorCode(errorCode: ErrorCode): ILogger {
    if (!this.isValidErrorCode(errorCode)) {
      throw new Error('Invalid errorCode.');
    }
    const subsystemCodeValue = errorCode.valueOf().substring(0, 3);
    if (subsystemCodeValue === SubsystemCode.ARK_GUARD.valueOf()) {
      return this.getLoggerFromSubsystemCode(SubsystemCode.ARK_GUARD);
    }
    if (subsystemCodeValue === SubsystemCode.ES2PANDA.valueOf()) {
      return this.getLoggerFromSubsystemCode(SubsystemCode.ES2PANDA);
    }
    if (subsystemCodeValue === SubsystemCode.SDK.valueOf()) {
      return this.getLoggerFromSubsystemCode(SubsystemCode.SDK);
    }
    return this.getLoggerFromSubsystemCode(SubsystemCode.BUILDSYSTEM);
  }

  protected getLoggerFromSubsystemCode(subsystemCode: SubsystemCode): ILogger {
    const logger = this.loggerMap.get(subsystemCode);
    if (logger === undefined) {
      throw new Error('Invalid subsystemCode.');
    }
    return logger;
  }

  public hasErrors(): boolean {
    return this.hasErrorOccurred;
  }

  public resetErrorFlag(): void {
    this.hasErrorOccurred = false;
  }
}

export class LogDataFactory {
  static newInstance(code: ErrorCode, description: string, cause: string = '', position: string = '', solutions: string[] = [], moreInfo: Object | undefined = undefined): LogData {
    return new LogData(code, description, cause, position, solutions, moreInfo);
  }
}

export class LogData {
  code: ErrorCode;
  description: string;
  cause: string;
  position: string;
  solutions: string[];
  moreInfo: Object | undefined;

  constructor(code: ErrorCode, description: string, cause: string, position: string, solutions: string[], moreInfo: Object | undefined) {
    this.code = code;
    this.description = description;
    this.cause = cause;
    this.position = position;
    this.solutions = solutions;
    this.moreInfo = moreInfo;
  }

  toString(): string {
    let errorString = 'ERROR Code: ' + this.code + ' ' + this.description + '\n';
    if (this.cause !== '' || this.position !== '') {
      errorString += 'Error Message: ' + this.cause + '\n';
      if (this.position !== '') {
        errorString += 'Position: ' + this.position + '\n';
      }
      errorString += '\n\n';
    }
    if (this.solutions.length > 0 && this.solutions[0] !== '') {
      errorString += '* Try the following: \n';
      for (const solution of this.solutions) {
        errorString += '  > ' + solution + '\n';
      }
    }
    return errorString;
  }
}

export class ConsoleLogger implements ILogger {
  private static instances: Map<string, ConsoleLogger> = new Map<string, ConsoleLogger>();

  public printInfo(message: string): void {
    console.info('[INFO]', message);
  }

  public printWarn(message: string): void {
    console.warn('[WARN]', message);
  }

  public printDebug(message: string): void {
    console.debug('[DEBUG]', message);
  }

  public printError(error: LogData): void {
    console.error('[ERROR]', error.toString());
  }

  public printErrorAndExit(error: LogData): void {
    this.printError(error);
    throw new Error(error.toString());
  }

  public static createLogger(subsystemCode: string): ConsoleLogger {
    let logger = ConsoleLogger.instances.get(subsystemCode);
    if (logger === undefined) {
      logger = new ConsoleLogger();
      ConsoleLogger.instances.set(subsystemCode, logger);
    }
    return logger;
  }
}

export function getConsoleLogger(subsystemCode: string): ConsoleLogger {
  return ConsoleLogger.createLogger(subsystemCode);
}

export function patchBuildConfigLogger(buildConfig: BuildConfig, loggerGetter: LoggerGetter): void {
  buildConfig.getHvigorConsoleLogger = loggerGetter;
}
