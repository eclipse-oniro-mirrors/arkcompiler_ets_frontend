/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { ErrorCode } from './util/error'

export enum SubsystemCode {
    BUILDSYSTEM = '114',
    ES2PANDA = '115',
}

export interface ILogger {
    printInfo(message: string): void;
    printWarn(message: string): void;
    printDebug(message: string): void;
    printError(error: LogData): void;
    printErrorAndExit(error: LogData): void;
}

export type LoggerGetter = (code: SubsystemCode) => ILogger;

export class Logger implements ILogger {
    private static instance?: Logger;
    private loggerMap: { [key in SubsystemCode]?: ILogger };
    private hasErrorOccurred: boolean = false;

    private constructor(loggerGetter: LoggerGetter) {
        this.loggerMap = {};
        this.loggerMap[SubsystemCode.BUILDSYSTEM] = loggerGetter(SubsystemCode.BUILDSYSTEM);
        this.loggerMap[SubsystemCode.ES2PANDA] = loggerGetter(SubsystemCode.ES2PANDA);
    }

    public static getInstance(loggerGetter?: LoggerGetter): Logger {
        if (!Logger.instance) {
            if (!loggerGetter) {
                throw new Error('loggerGetter is required for the first Logger instantiation.');
            }
            Logger.instance = new Logger(loggerGetter);
        }
        return Logger.instance;
    }

    public static destroyInstance(): void {
        Logger.instance = undefined;
    }

    public printInfo(message: string, subsystemCode: SubsystemCode = SubsystemCode.BUILDSYSTEM): void {
        const logger: ILogger = this.getLoggerFromSubsystemCode(subsystemCode);
        logger.printInfo(`[ID:${process.pid}] [time:${Date.now()}] ${message}`);
    }

    public printWarn(message: string, subsystemCode: SubsystemCode = SubsystemCode.BUILDSYSTEM): void {
        const logger: ILogger = this.getLoggerFromSubsystemCode(subsystemCode);
        logger.printWarn(`[ID:${process.pid}] [time:${Date.now()}] ${message}`);
    }

    public printDebug(message: string, subsystemCode: SubsystemCode = SubsystemCode.BUILDSYSTEM): void {
        const logger: ILogger = this.getLoggerFromSubsystemCode(subsystemCode);
        logger.printDebug(`[ID:${process.pid}] [time:${Date.now()}] ${message}`);
    }

    public printError(error: LogData): void {
        this.hasErrorOccurred = true;
        const logger: ILogger = this.getLoggerFromErrorCode(error.code);
        logger.printError(error);
    }

    public printErrorAndExit(error: LogData): void {
        this.hasErrorOccurred = true;
        const logger: ILogger = this.getLoggerFromErrorCode(error.code);
        logger.printErrorAndExit(error);
    }

    protected isValidErrorCode(errorCode: ErrorCode): boolean {
        return /^\d{8}$/.test(errorCode);
    }

    protected getLoggerFromErrorCode(errorCode: ErrorCode): ILogger {
        if (!this.isValidErrorCode(errorCode)) {
            throw new Error('Invalid errorCode.');
        }
        const subsystemCode = errorCode.slice(0, 3) as SubsystemCode;
        const logger = this.getLoggerFromSubsystemCode(subsystemCode);
        return logger;
    }

    protected getLoggerFromSubsystemCode(subsystemCode: SubsystemCode): ILogger {
        if (!this.loggerMap[subsystemCode]) {
            throw new Error('Invalid subsystemCode.');
        }
        return this.loggerMap[subsystemCode];
    }

    public hasErrors(): boolean {
        return this.hasErrorOccurred;
    }

    public resetErrorFlag(): void {
        this.hasErrorOccurred = false;
    }
}

export class LogDataFactory {
    static newInstance(
        code: ErrorCode,
        description: string,
        cause: string = '',
        position: string = '',
        solutions: string[] = [],
        moreInfo?: Object
    ): LogData {
        const data: LogData = new LogData(code, description, cause, position, solutions, moreInfo);
        return data;
    }
}

export class LogData {
    code: ErrorCode;
    description: string;
    cause: string;
    position: string;
    solutions: string[];
    moreInfo?: Object;

    constructor(
        code: ErrorCode,
        description: string,
        cause: string,
        position: string,
        solutions: string[],
        moreInfo?: Object
    ) {
        this.code = code;
        this.description = description;
        this.cause = cause;
        this.position = position;
        this.solutions = solutions;
        this.moreInfo = moreInfo;
    }

    toString(): string {
        let errorString = `ERROR Code: ${this.code} ${this.description}\n`;

        if (this.cause || this.position) {
            errorString += `Error Message: ${this.cause}\n`;
            if (this.position) {
                errorString += `Position: ${this.position}\n`;
            }
            errorString += '\n\n';
        }

        if (this.solutions.length > 0 && this.solutions[0] !== '') {
            errorString += `* Try the following: \n${this.solutions.map(str => `  > ${str}`).join('\n')}\n`;
        }

        if (this.moreInfo) {
            errorString += `\nMore Info:\n`;
            for (const [key, value] of Object.entries(this.moreInfo)) {
                errorString += `  - ${key.toUpperCase()}: ${value}\n`;
            }
        }

        return errorString;
    }
}

class ConsoleLogger implements ILogger {
    private static instances: { [key: string]: ConsoleLogger } = {};

    private constructor() { }

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
        process.exit(1);
    }

    public static createLogger(subsystemCode: string): ConsoleLogger {
        if (!ConsoleLogger.instances[subsystemCode]) {
            ConsoleLogger.instances[subsystemCode] = new ConsoleLogger();
        }
        return ConsoleLogger.instances[subsystemCode];
    }
}

export function getConsoleLogger(subsystemCode: string): ConsoleLogger {
    return ConsoleLogger.createLogger(subsystemCode);
}
