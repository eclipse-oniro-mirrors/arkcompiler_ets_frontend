import { LogData } from '../logger';

export class ErrorCode {
  public static readonly BUILDSYSTEM_ENTRY_DISPATCH_FAIL: string = '11410040';
}

export class DriverError extends Error {
  public logData: LogData;

  public constructor(logData: LogData) {
    super(logData.toString());
    this.logData = logData;
  }

  public toString(): string {
    return this.logData.toString();
  }
}
