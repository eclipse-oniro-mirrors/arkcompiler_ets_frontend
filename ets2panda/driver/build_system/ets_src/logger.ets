export class SubsystemCode {
  public static readonly ARK_GUARD: string = '113';
  public static readonly BUILDSYSTEM: string = '114';
  public static readonly ES2PANDA: string = '115';
  public static readonly SDK: string = '117';
}

export interface ILogger {
  printInfo(message: string): void;
  printWarn(message: string): void;
  printDebug(message: string): void;
  printError(error: LogData): void;
  printErrorAndExit(error: LogData): void;
}

export class LogData {
  public code: string;
  public description: string;
  public cause: string;
  public position: string;
  public solutions: string[];
  public moreInfo: string;

  public constructor(code: string, description: string, cause: string, position: string, solutions: string[], moreInfo: string) {
    this.code = code;
    this.description = description;
    this.cause = cause;
    this.position = position;
    this.solutions = solutions;
    this.moreInfo = moreInfo;
  }

  public toString(): string {
    let result: string = 'ERROR Code: ' + this.code + ' ' + this.description + '\n';
    if (this.cause.length > 0 || this.position.length > 0) {
      result += 'Error Message: ' + this.cause + '\n';
      if (this.position.length > 0) {
        result += 'Position: ' + this.position + '\n';
      }
      result += '\n\n';
    }
    if (this.solutions.length > 0 && this.solutions[0].length > 0) {
      result += '* Try the following: \n';
      for (let i: int = 0; i < this.solutions.length; i++) {
        result += '  > ' + this.solutions[i] + '\n';
      }
    }
    if (this.moreInfo.length > 0) {
      result += '\nMore Info:\n';
      result += '  - VALUE: ' + this.moreInfo + '\n';
    }
    return result;
  }
}

export class LogDataFactory {
  public static newInstance(code: string, description: string): LogData {
    return new LogData(code, description, '', '', [], '');
  }

  public static newDetailedInstance(
    code: string,
    description: string,
    cause: string,
    position: string,
    solutions: string[],
    moreInfo: string
  ): LogData {
    return new LogData(code, description, cause, position, solutions, moreInfo);
  }
}

class ConsoleLogger implements ILogger {
  public printInfo(message: string): void {
    console.log(message);
  }

  public printWarn(message: string): void {
    console.warn(message);
  }

  public printDebug(message: string): void {
    console.log(message);
  }

  public printError(error: LogData): void {
    console.error(error.toString());
  }

  public printErrorAndExit(error: LogData): void {
    let message: string = error.toString();
    console.error(message);
    throw new Error(message);
  }
}

export class Logger implements ILogger {
  private static instance: Logger = new Logger();
  private enableDebugOutput: boolean = false;
  private hasErrorOccurred: boolean = false;
  private arkGuardLogger: ILogger = new ConsoleLogger();
  private buildSystemLogger: ILogger = new ConsoleLogger();
  private es2pandaLogger: ILogger = new ConsoleLogger();
  private sdkLogger: ILogger = new ConsoleLogger();

  public static getInstance(): Logger {
    return Logger.instance;
  }

  public static destroyInstance(): void {
    Logger.instance = new Logger();
  }

  public setDebugOutput(enableDebugOutput: boolean): void {
    this.enableDebugOutput = enableDebugOutput;
  }

  public printInfo(message: string): void {
    this.buildSystemLogger.printInfo(message);
  }

  public printWarn(message: string): void {
    this.buildSystemLogger.printWarn(message);
  }

  public printWarning(message: string): void {
    this.printWarn(message);
  }

  public printDebug(message: string): void {
    if (this.enableDebugOutput) {
      this.buildSystemLogger.printDebug(message);
    }
  }

  public printInfoForSubsystem(message: string, subsystemCode: string): void {
    this.getLoggerFromSubsystemCode(subsystemCode).printInfo(message);
  }

  public printWarnForSubsystem(message: string, subsystemCode: string): void {
    this.getLoggerFromSubsystemCode(subsystemCode).printWarn(message);
  }

  public printDebugForSubsystem(message: string, subsystemCode: string): void {
    if (this.enableDebugOutput) {
      this.getLoggerFromSubsystemCode(subsystemCode).printDebug(message);
    }
  }

  public printError(error: LogData): void {
    this.hasErrorOccurred = true;
    this.getLoggerFromErrorCode(error.code).printError(error);
  }

  public printErrorAndExit(error: LogData): void {
    this.hasErrorOccurred = true;
    this.getLoggerFromErrorCode(error.code).printErrorAndExit(error);
  }

  public isValidErrorCode(errorCode: string): boolean {
    if (errorCode.length !== 8) {
      return false;
    }
    for (let i: int = 0; i < errorCode.length; i++) {
      let value: int = errorCode.charCodeAt(i);
      if (value < 48 || value > 57) {
        return false;
      }
    }
    return true;
  }

  public getLoggerFromErrorCode(errorCode: string): ILogger {
    if (!this.isValidErrorCode(errorCode)) {
      throw new Error('Invalid errorCode.');
    }
    return this.getLoggerFromSubsystemCode(errorCode.substring(0, 3));
  }

  public getLoggerFromSubsystemCode(subsystemCode: string): ILogger {
    if (subsystemCode === SubsystemCode.ARK_GUARD) {
      return this.arkGuardLogger;
    }
    if (subsystemCode === SubsystemCode.BUILDSYSTEM) {
      return this.buildSystemLogger;
    }
    if (subsystemCode === SubsystemCode.ES2PANDA) {
      return this.es2pandaLogger;
    }
    if (subsystemCode === SubsystemCode.SDK) {
      return this.sdkLogger;
    }
    throw new Error('Invalid subsystemCode.');
  }

  public hasErrors(): boolean {
    return this.hasErrorOccurred;
  }

  public resetErrorFlag(): void {
    this.hasErrorOccurred = false;
  }
}

export function getConsoleLogger(subsystemCode: string): ILogger {
  return new ConsoleLogger();
}
