/**
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

import { readFileSync, writeFileSync } from 'fs';
import { readJsonSync } from 'fs-extra';
import type { IOptions } from '../configs/IOptions';
import { fileExtensions } from '../common/type';
import type { PathAndExtension } from '../common/type';
import fs from 'fs';
import path from 'path';

export const BUNDLE = '@bundle:';
export const NORMALIZE = '@normalized:';

export interface FileInfo {
  exists: boolean;
  isFile?: boolean;
  isDirectory?: boolean;
}

export type GetCachedFileInfo = (filePath: string) => FileInfo | undefined;

export class FileUtils {
  private static getCachedFileInfo: GetCachedFileInfo | undefined;

  public static setCachedFileInfo(fn: GetCachedFileInfo | undefined): void {
    FileUtils.getCachedFileInfo = fn;
  }

  /**
   * Query file meta via hvigor cache first, falling back to fs.statSync on cache miss.
   */
  public static getFileInfo(filePath: string): FileInfo {
    if (FileUtils.getCachedFileInfo) {
      try {
        const info = FileUtils.getCachedFileInfo(filePath);
        if (info && typeof info.exists === 'boolean') {
          return info;
        }
      } catch {
        // cache miss – fall through to fs
      }
    }
    try {
      const stats = fs.statSync(filePath);
      return {
        exists: true,
        isFile: stats.isFile(),
        isDirectory: stats.isDirectory(),
      };
    } catch {
      return { exists: false };
    }
  }

  public static fileExists(filePath: string): boolean {
    return FileUtils.getFileInfo(filePath).exists;
  }

  public static isFile(filePath: string): boolean {
    const info = FileUtils.getFileInfo(filePath);
    return info.exists && !!info.isFile;
  }

  public static isDirectory(filePath: string): boolean {
    const info = FileUtils.getFileInfo(filePath);
    return info.exists && !!info.isDirectory;
  }

  /**
   * Read file and return content
   *
   * @param filePath file path
   */
  public static readFile(filePath: string): string | undefined {
    if (!FileUtils.fileExists(filePath)) {
      console.error(`File <${this.getFileName(filePath)}> is not found.`);
      return undefined;
    }
    return readFileSync(filePath, 'utf-8');
  }

  /**
   * Read file and convert to json object.
   *
   * @param filePath file path
   */
  public static readFileAsJson(filePath: string): IOptions | undefined {
    if (!FileUtils.fileExists(filePath)) {
      console.error(`File <${this.getFileName(filePath)}> is not found.`);
      return undefined;
    }

    try {
      return readJsonSync(filePath);
    } catch (e) {
      console.error('json file read error: ' + filePath);
      return undefined;
    }
  }

  /**
   * Get File Name
   *
   * @param filePath file path
   */
  public static getFileName(filePath: string): string | undefined {
    if (!filePath) {
      return undefined;
    }

    const lastSepIndex: number = filePath.lastIndexOf('/');
    if (lastSepIndex >= 0) {
      return filePath.slice(lastSepIndex + 1);
    }

    return filePath.slice(filePath.lastIndexOf('\\') + 1);
  }

  /**
   * Get suffix of a file.
   *
   * @param filePath file path
   */
  public static getFileExtension(filePath: string): string | undefined {
    if (!filePath || !filePath.includes('.')) {
      return undefined;
    }

    // get file name
    let fileName: string = this.getFileName(filePath);
    if (!fileName.includes('.')) {
      return undefined;
    }

    return fileName.slice(fileName.lastIndexOf('.') + 1);
  }

  public static writeFile(filePath: string, content: string): void {
    writeFileSync(filePath, content);
  }

  /**
   * get prefix of directory
   * @param dirPath
   */
  public static getPrefix(dirPath: string): string | undefined {
    if (!dirPath || (!dirPath.includes('/') && !dirPath.includes('\\'))) {
      return undefined;
    }

    const sepIndex: number = dirPath.lastIndexOf('/');
    if (sepIndex >= 0) {
      return dirPath.slice(0, sepIndex + 1);
    }

    return dirPath.slice(0, dirPath.lastIndexOf('\\') + 1);
  }

  public static getPathWithoutPrefix(filePath: string, prefix: string): string | undefined {
    if (!filePath.startsWith(prefix)) {
      return filePath;
    }

    return filePath.slice(prefix.length);
  }

  public static splitFilePath(filePath: string): string[] {
    if (!filePath.includes('\\') && !filePath.includes('\/')) {
      return [filePath];
    }
    const directories = filePath.split(/[\/\\]/);
    return directories;
  }

  /**
   * split the file path and collect the results into the reserved array
   */
  public static collectPathReservedString(filePath: string, reservedArray: string[]): void {
    const directories = this.splitFilePath(filePath);
    directories.forEach(reservedStr => {
      reservedArray.push(reservedStr);
    });
  }

  static relativePathBegins: string[] = ['./', '../', '.\\', '..\\'];
  public static isRelativePath(filePath: string): boolean {
    for (const bebin of this.relativePathBegins) {
      if (filePath.startsWith(bebin)) {
        return true;
      }
    }
    return false;
  }

  public static getFileSuffix(filePath: string): PathAndExtension {
    for (let ext of fileExtensions) {
      if (filePath.endsWith(ext)) {
        const filePathWithoutSuffix: string = filePath.replace(new RegExp(`${ext}$`), '');
        return { path: filePathWithoutSuffix, ext: ext };
      }
    }
    return { path: filePath, ext: '' };
  }

  public static isReadableFile(filePath: string): boolean {
    const info = FileUtils.getFileInfo(filePath);
    if (info.exists && info.isFile) {
      return true;
    }
    if (!info.exists) {
      return false;
    }
    try {
      fs.accessSync(filePath, fs.constants.R_OK);
    } catch (err) {
      return false;
    }
    return true;
  }

  public static toUnixPath(data: string): string {
    if (/^win/.test(require('os').platform())) {
      const fileTmps: string[] = data.split(path.sep);
      const newData: string = path.posix.join(...fileTmps);
      return newData;
    }
    return data;
  }

  public static getAbsPathBaseConfigPath(configPath: string, relativePath: string): string {
    const absPath: string = path.join(path.dirname(configPath), relativePath);
    return this.toUnixPath(absPath);
  }

  public static deleteFile(filePath: string): void {
    if (FileUtils.fileExists(filePath)) {
      fs.unlinkSync(filePath);
    }
  }

  public static createDirectory(dirPath: string): void {
    if (!FileUtils.fileExists(dirPath)) {
      fs.mkdirSync(dirPath, { recursive: true });
    }
  }

  public static deleteFolderRecursive(folderPath: string): void {
    if (FileUtils.fileExists(folderPath)) {
      const files = fs.readdirSync(folderPath);
      for (const file of files) {
        const curPath = path.join(folderPath, file);
        if (fs.lstatSync(curPath).isDirectory()) {
          FileUtils.deleteFolderRecursive(curPath);
        } else {
          fs.unlinkSync(curPath);
        }
      }
      fs.rmdirSync(folderPath);
    }
  }
}
