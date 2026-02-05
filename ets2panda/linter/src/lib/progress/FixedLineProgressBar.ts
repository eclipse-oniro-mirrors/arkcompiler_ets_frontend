/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import chalk from 'chalk';
import type { CmdProgressInfo } from './CmdProgressInfo';
import type { DisplayInfo } from './DisplayInfo';

const DEFAULT_TERMINAL_ROWS = 24;
const DEFAULT_TERMINAL_COLUMNS = 80;
const DEFAULT_BAR_WIDTH = 30;
const DEFAULT_RESERVED_LINES = 2;
const MAX_TASK_NAME_LENGTH = 20;
export class FixedLineProgressBar {
  private isActive = false;
  private currentTask: string = '';
  private currentStatus: string = '';
  private total: number = 0;
  private currentProgress: number = 0;
  private lastOutput = '';
  private hasCompleted = false;
  private isFirstUpdate = true;
  private isFirstRender = true;
  private lastWindowRows = 0;
  private lastWindowCols = 0;
  private readonly fixedBarWidth = DEFAULT_BAR_WIDTH;
  private readonly reservedLines = DEFAULT_RESERVED_LINES;
  private readonly useColors: boolean;

  private alternateScreenActive = false;

  constructor() {
    this.useColors = process.stderr.isTTY && !process.env.NO_COLOR && process.env.TERM !== 'dumb';

    if (process.stdout.isTTY) {
      this.lastWindowRows = process.stdout.rows || DEFAULT_TERMINAL_ROWS;
      this.lastWindowCols = process.stdout.columns || DEFAULT_TERMINAL_COLUMNS;

      process.stdout.on('resize', () => {
        this.handleResize();
      });
    }
  }

  private switchToAlternateScreen(): void {
    if (process.stderr.isTTY && !this.alternateScreenActive) {
      try {
        process.stderr.write('\x1B[?1049h');
        process.stderr.write('\x1B[2J');
        process.stderr.write('\x1B[1;1H');
        this.alternateScreenActive = true;
      } catch (error) {
        console.error('Error switching to alternate screen:', error);
      }
    }
  }

  private switchFromAlternateScreen(): void {
    if (process.stderr.isTTY && this.alternateScreenActive) {
      try {
        process.stderr.write('\x1B[?1049l');
        this.alternateScreenActive = false;
      } catch (error) {
        console.error('Error switching from alternate screen:', error);
      }
    }
  }

  private clearAlternateScreen(): void {
    if (this.alternateScreenActive) {
      try {
        process.stderr.write('\x1B[2J');
        process.stderr.write('\x1B[1;1H');
      } catch (error) {
        console.error('Error clearing alternate screen:', error);
      }
    }
  }

  private handleResize(): void {
    if (!this.isActive || this.hasCompleted) {
      return;
    }

    const currentRows = process.stdout.rows || DEFAULT_TERMINAL_ROWS;
    const currentCols = process.stdout.columns || DEFAULT_TERMINAL_COLUMNS;

    if (currentRows !== this.lastWindowRows || currentCols !== this.lastWindowCols) {
      this.lastWindowRows = currentRows;
      this.lastWindowCols = currentCols;

      this.switchToAlternateScreen();

      this.clearAlternateScreen();
      this.forceRedraw();
    }
  }

  private forceRedraw(): void {
    const content = this.generateContent(this.currentProgress);
    this.writeToTerminal(content, true);
  }

  private getStatusText(useColor: boolean = false): string {
    const statusMap: Record<string, { text: string; color: chalk.Chalk }> = {
      scanning: { text: 'Scanning...', color: chalk.blue },
      fixing: { text: 'Fixing...', color: chalk.yellow },
      completed: { text: 'Completed', color: chalk.green },
      skipped: { text: 'Skipped', color: chalk.magenta }
    };

    const statusInfo = statusMap[this.currentStatus] || statusMap.scanning;

    if (useColor && this.useColors) {
      return statusInfo.color(statusInfo.text);
    }

    return statusInfo.text;
  }

  private generateBarString(progressRatio: number): string {
    const completed = Math.floor(progressRatio * this.fixedBarWidth);
    const remaining = this.fixedBarWidth - completed;

    if (this.useColors) {
      const completedPart = chalk.white('█'.repeat(completed));
      const remainingPart = chalk.bgGray(' '.repeat(remaining));
      return completedPart + remainingPart;
    }
    const completedPart = '#'.repeat(completed);
    const remainingPart = '.'.repeat(remaining);
    return completedPart + remainingPart;
  }

  private generateContent(progress: number): string {
    const progressRatio = this.total > 0 ? Math.min(1, progress / this.total) : 0;
    const percent = Math.round(progressRatio * 100);

    const barString = this.generateBarString(progressRatio);
    const statusText = this.getStatusText(true);

    let taskName = this.currentTask;
    if (taskName.length > MAX_TASK_NAME_LENGTH) {
      taskName = taskName.substring(0, MAX_TASK_NAME_LENGTH - 3) + '...';
    }

    if (this.useColors) {
      taskName = chalk.bold(taskName);
    }

    return `${taskName} ${statusText} [${barString}] ${percent}%`;
  }

  private writeToTerminal(content: string, clearFirst: boolean = true): void {
    try {
      this.switchToAlternateScreen();

      if (this.isFirstRender) {
        this.clearAlternateScreen();
        this.isFirstRender = false;
      }

      process.stderr.write('\x1B[1;1H');

      if (clearFirst) {
        process.stderr.write('\x1B[2K');
      }

      process.stderr.write('\r' + content);

      process.stderr.write('\n\x1B[2K');

      process.stderr.write('\n');
    } catch (error) {
      console.error('Progress bar write error:', error);
    }
  }

  getDisplayInfo(): DisplayInfo {
    return {
      cliLine: 0,
      ideLine: 1,
      totalReserved: this.reservedLines,
      inAlternateScreen: this.alternateScreenActive
    };
  }

  startBar(taskName: string, total: number, status: 'scanning' | 'fixing'): void {
    this.hasCompleted = false;
    this.isActive = true;
    this.isFirstUpdate = true;
    this.isFirstRender = true;
    this.currentTask = taskName;
    this.currentStatus = status;
    this.total = total > 0 ? total : 1;
    this.currentProgress = 0;

    this.switchToAlternateScreen();

    this.updateBar(0, status, true);
  }

  updateBar(progress: number, status: 'scanning' | 'fixing', forceUpdate: boolean = false): void {
    if (!this.isActive || this.hasCompleted) {
      return;
    }

    this.currentProgress = progress;
    this.currentStatus = status;

    const content = this.generateContent(progress);

    if (forceUpdate || this.isFirstUpdate || content !== this.lastOutput) {
      const shouldClear = !this.isFirstUpdate;
      this.writeToTerminal(content, shouldClear);
      this.lastOutput = content;
      this.isFirstUpdate = false;
    }
  }

  completeBar(): void {
    if (!this.isActive || this.hasCompleted) {
      return;
    }

    this.hasCompleted = true;
    this.currentStatus = 'completed';

    const finalProgress = this.total;
    const content = this.generateContent(finalProgress);

    this.writeToTerminal(content, true);

    process.stderr.write('\n');

    setTimeout(() => {
      this.switchFromAlternateScreen();
    }, 100);

    this.isActive = false;
    this.lastOutput = '';
  }

  skipBar(): void {
    if (!this.isActive || this.hasCompleted) {
      return;
    }

    this.hasCompleted = true;
    this.currentStatus = 'skipped';

    let taskName = this.currentTask;
    if (taskName.length > MAX_TASK_NAME_LENGTH) {
      taskName = taskName.substring(0, MAX_TASK_NAME_LENGTH - 3) + '...';
    }

    if (this.useColors) {
      taskName = chalk.bold(taskName);
    }

    const statusText = this.getStatusText(true);
    const content = `${taskName} ${statusText}`;

    this.writeToTerminal(content, true);
    process.stderr.write('\n');

    this.switchFromAlternateScreen();

    this.isActive = false;
    this.lastOutput = '';
  }

  stop(): void {
    if (this.isActive) {
      this.hasCompleted = true;
      this.isActive = false;

      this.switchFromAlternateScreen();
    }
  }

  getConfig(): { barWidth: number; useColors: boolean } {
    return {
      barWidth: this.fixedBarWidth,
      useColors: this.useColors
    };
  }
}

export function preProcessCmdProgressBar(cmdProgressInfo: CmdProgressInfo): void {
  const { options, cmdProgressBar, srcFiles } = cmdProgressInfo;

  const isMigrationStep = options.migratorMode && cmdProgressInfo.migrationInfo;
  const migrationPhase = isMigrationStep ?
    ` ${(cmdProgressInfo.migrationInfo!.currentPass ?? 0) + 1} / ${cmdProgressInfo.migrationInfo!.maxPasses ?? 1}` :
    '';
  const phasePrefix = isMigrationStep ? 'Migration Phase' : 'Scan Phase';
  const displayContent = `${phasePrefix}${migrationPhase}`;
  const totalFiles = srcFiles.length;

  if (isMigrationStep) {
    cmdProgressBar.startBar(displayContent, totalFiles, 'fixing');
    if (!srcFiles || srcFiles.length === 0) {
      cmdProgressBar.skipBar();
    }
  } else {
    cmdProgressBar.startBar(displayContent, totalFiles, 'scanning');
  }
}

export function processCmdProgressBar(cmdProgressInfo: CmdProgressInfo, fileCount: number): void {
  const progress = fileCount;
  const status = cmdProgressInfo.options.migratorMode && cmdProgressInfo.migrationInfo ? 'fixing' : 'scanning';

  cmdProgressInfo.cmdProgressBar.updateBar(progress, status);
}

export function postProcessCmdProgressBar(cmdProgressInfo: CmdProgressInfo): void {
  const { cmdProgressBar, srcFiles } = cmdProgressInfo;

  if (srcFiles.length > 0) {
    cmdProgressBar.completeBar();
  } else {
    cmdProgressBar.skipBar();
  }
}
