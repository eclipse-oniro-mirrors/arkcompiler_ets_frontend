/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import fs from 'fs';
import path from 'path';

export enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
}

export interface LogEntry {
    timestamp: Date;
    level: LogLevel;
    message: string;
    context?: any;
}

export interface LoggerConfig {
    consoleOutput?: boolean;
    fileOutput?: boolean;
    logDir?: string;
    filePrefix?: string;
    maxFileSize?: number;
    minLevel?: LogLevel;
}

const DEFAULT_CONFIG: Required<LoggerConfig> = {
    consoleOutput: true,
    fileOutput: false,
    logDir: path.resolve(__dirname),
    filePrefix: 'lsp',
    maxFileSize: 10,
    minLevel: LogLevel.INFO,
};

export class Logger {
    private static instance: Logger;

    private constructor(config: LoggerConfig = {}) {
        this.config = { ...DEFAULT_CONFIG, ...config };
        if (this.config.fileOutput) {
            this.createNewLogFile();
        }
    }

    public static getInstance(config?: LoggerConfig): Logger {
        if (!Logger.instance) {
            Logger.instance = new Logger(config);
        }
        return Logger.instance;
    }
    private config: Required<LoggerConfig>;
    private currentFilePath: string = '';
    private currentFileSize: number = 0;

    private formatDateTime(date: Date): string {
        const year = date.getFullYear();
        const month = String(date.getMonth() + 1).padStart(2, '0');
        const day = String(date.getDate()).padStart(2, '0');
        const hours = String(date.getHours()).padStart(2, '0');
        const minutes = String(date.getMinutes()).padStart(2, '0');
        const seconds = String(date.getSeconds()).padStart(2, '0');
        const milliseconds = String(date.getMilliseconds()).padStart(3, '0');

        return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${milliseconds}`;
    }

    private getDatePart(date: Date): string {
        return [
            date.getFullYear(),
            (date.getMonth() + 1).toString().padStart(2, '0'),
            date.getDate().toString().padStart(2, '0')
        ].join('-');
    }

    private getTimePart(date: Date): string {
        return [
            date.getHours().toString().padStart(2, '0'),
            date.getMinutes().toString().padStart(2, '0'),
            date.getSeconds().toString().padStart(2, '0')
        ].join('-');
    }

    private createNewLogFile(): void {
        const maxRetries = 3;
        let retryCount = 0;
        while (retryCount < maxRetries) {
            try {
                const now = new Date();
                const dateString = this.getDatePart(now);
                const timeString = this.getTimePart(now);
                const newPath = path.join(
                    this.config.logDir,
                    `${this.config.filePrefix}_${dateString}_${timeString}.log`
                );
                fs.writeFileSync(newPath, '');
                this.currentFilePath = newPath;
                this.currentFileSize = 0;
                return;
            } catch (err) {
                retryCount++;
                console.error(`Attempt ${retryCount} failed: ${err}`);
                if (retryCount === maxRetries) {
                    console.error('All attempts failed. Exiting process.');
                    process.exit(1);
                }
            }
        }
    }

    private formatEntry(entry: LogEntry): string {
        const timestamp = this.formatDateTime(entry.timestamp);
        const level = LogLevel[entry.level].padEnd(5, ' ');
        const context = entry.context
            ? `\nContext: ${JSON.stringify(entry.context, null, 2)}`
            : '';

        return `[${timestamp}] [${level}] ${entry.message}${context}\n`;
    }

    private writeToFile(entry: string): void {
        if (!this.config.fileOutput) return;

        const entrySize = Buffer.byteLength(entry, 'utf8');
        const newFileSize = this.currentFileSize + entrySize;

        if (newFileSize > this.config.maxFileSize * 1024 * 1024) {
            this.createNewLogFile();
        }

        try {
            fs.appendFileSync(this.currentFilePath, entry, 'utf8');
            this.currentFileSize += entrySize;
        } catch (err) {
            console.error(`write log file failed: ${err}`);
            process.exit(1);
        }
    }

    private log(level: LogLevel, message: string, context?: any): void {
        if (level < this.config.minLevel) return;

        const entry: LogEntry = {
            timestamp: new Date(),
            level,
            message,
            context,
        };

        const formatted = this.formatEntry(entry);

        if (this.config.consoleOutput) {
            const color = this.getConsoleColor(level);
            console.log(color, formatted, '\x1b[0m');
        }

        this.writeToFile(formatted);
    }

    private getConsoleColor(level: LogLevel): string {
        switch (level) {
            case LogLevel.DEBUG: return '\x1b[36m';
            case LogLevel.INFO: return '\x1b[32m';
            case LogLevel.WARN: return '\x1b[33m';
            case LogLevel.ERROR: return '\x1b[31m';
            default: return '\x1b[0m';
        }
    }

    debug(message: string, context?: any): void {
        this.log(LogLevel.DEBUG, message, context);
    }

    info(message: string, context?: any): void {
        this.log(LogLevel.INFO, message, context);
    }

    warn(message: string, context?: any): void {
        this.log(LogLevel.WARN, message, context);
    }

    error(message: string, context?: any): void {
        this.log(LogLevel.ERROR, message, context);
    }
}

export const logger = Logger.getInstance();