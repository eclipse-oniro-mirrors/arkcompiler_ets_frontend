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

import * as crypto from 'crypto';
import * as fs from 'fs';
import * as os from 'os';
import * as path from 'path';

import {
  ARKTS_MODULE_NAME,
  DECL_ETS_SUFFIX,
  LANGUAGE_VERSION,
  NATIVE_MODULE,
  sdkConfigPrefix
} from '../pre_define';
import {
  Logger,
  LogData,
  LogDataFactory
} from '../logger';
import { ErrorCode } from '../error_code';
import {
  ModuleInfo,
  OHOS_MODULE_TYPE,
  BuildConfig
} from '../types';

const WINDOWS: string = 'Windows_NT';
const LINUX: string = 'Linux';
const MAC: string = 'Darwin';

export function isWindows(): boolean {
  return os.type() === WINDOWS;
}

export function isLinux(): boolean {
  return os.type() === LINUX;
}

export function isMac(): boolean {
  return os.type() === MAC;
}

export function changeFileExtension(file: string, targetExt: string, originExt = ''): string {
  let currentExt = originExt.length === 0 ? getFileExtension(file) : originExt;
  let fileWithoutExt = file.substring(0, file.lastIndexOf(currentExt));
  return fileWithoutExt + targetExt;
}

export function changeDeclgenFileExtension(file: string, targetExt: string): string {
  if (file.endsWith(DECL_ETS_SUFFIX)) {
      return changeFileExtension(file, targetExt, DECL_ETS_SUFFIX);
  }
  return changeFileExtension(file, targetExt);
}

export function ensurePathExists(filePath: string): void {
  try {
    const dirPath: string = path.dirname(filePath);
    if (!fs.existsSync(dirPath)) {
      fs.mkdirSync(dirPath, { recursive: true });
    }
  } catch (error) {
    if (error instanceof Error) {
      console.error(`Error: ${error.message}`);
    }
  }
}

export function getFileHash(filePath: string): string {
  const content = fs.readFileSync(filePath, 'utf8');
  return crypto.createHash('sha256').update(content).digest('hex');
}

export function toUnixPath(path: string): string {
  return path.replace(/\\/g, '/');
}

export function readFirstLineSync(filePath: string): string | null {

  const fd = fs.openSync(filePath, 'r');
  const buffer = Buffer.alloc(256);
  const bytesRead = fs.readSync(fd, buffer, 0, buffer.length, 0);
  fs.closeSync(fd);

  const content = buffer.toString('utf-8', 0, bytesRead);
  const firstLine = content.split(/\r?\n/, 1)[0].trim();

  return firstLine;
}

export function safeRealpath(path: string, logger: Logger): string {
  try {
    return fs.realpathSync(path);
  } catch(error) {
    const msg = error instanceof Error ? error.message : String(error);
    const logData: LogData = LogDataFactory.newInstance(
      ErrorCode.BUILDSYSTEM_PATH_RESOLVE_FAIL,
      `Error resolving path "${path}".`,
      msg
    );
    logger.printError(logData);
    throw logData;
  }
}

export function getInteropFilePathByApi(apiName: string, interopSDKPath: Set<string>): string {
  for (const sdkPath of interopSDKPath) {
    const modulePath = path.resolve(sdkPath, apiName + DECL_ETS_SUFFIX);
    if (fs.existsSync(modulePath)) {
      return modulePath;
    }
  }
  return '';
}

/**
 * Issue:26513
 * todo read config from external instead of prodcue
 */
export function getOhmurlByApi(api: string): string {
  const REG_SYSTEM_MODULE: RegExp = new RegExp(`@(${sdkConfigPrefix})\\.(\\S+)`);

  if (REG_SYSTEM_MODULE.test(api.trim())) {
    return api.replace(REG_SYSTEM_MODULE, (_, moduleType, systemKey) => {
      const systemModule: string = `${moduleType}.${systemKey}`;
      if (NATIVE_MODULE.has(systemModule)) {
        return `@native:${systemModule}`;
      } else if (moduleType === ARKTS_MODULE_NAME) {
        // @arkts.xxx -> @ohos:arkts.xxx
        return `@ohos:${systemModule}`;
      } else {
        return `@ohos:${systemKey}`;
      };
    });
  }
  return '';
}

export function isSubPathOf(targetPath: string, parentDir: string): boolean {
  const resolvedParent = toUnixPath(path.resolve(parentDir));
  const resolvedTarget = toUnixPath(path.resolve(targetPath));
  return resolvedTarget === resolvedParent || resolvedTarget.startsWith(resolvedParent + '/');
}

/**
 * Get the full extension of a file, supporting composite extensions like '.d.ts', '.test.ts', '.d.ets', etc.
 * @param filePath - File path or file name.
 * @param knownCompositeExts - Optional list of known composite extensions to match against.
 * @returns The full extension (e.g., '.d.ts'). Returns an empty string if no extension is found.
 */
export function getFileExtension(
  filePath: string,
  knownCompositeExts: string[] = ['.d.ts', '.test.ts', '.d.ets']
): string {
  const baseName = path.basename(filePath);

  // Match known composite extensions first
  for (const ext of knownCompositeExts) {
    if (baseName.endsWith(ext)) {
      return ext;
    }
  }

  // Fallback to default behavior: return the last segment after the final dot
  return path.extname(baseName);
}

export function hasEntry(moduleInfo: ModuleInfo): boolean {
  switch (moduleInfo.moduleType) {
    case OHOS_MODULE_TYPE.SHARED:
    case OHOS_MODULE_TYPE.HAR:
      return true;
    default:
      return false;
  }
}

export function createFileIfNotExists(filePath: string, content: string): boolean {
  try {
    const normalizedPath = path.normalize(filePath);
    if (fs.existsSync(normalizedPath)) {
      return false;
    }

    const dir = path.dirname(normalizedPath);
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }

    fs.writeFileSync(normalizedPath, content, { encoding: 'utf-8' });
    return true;
  } catch (error) {
    return false;
  }
}

export function isMixCompileProject(buildConfig: BuildConfig): boolean {
  for (const moduleInfo of buildConfig.dependentModuleList) {
    if (
      moduleInfo.language === LANGUAGE_VERSION.ARKTS_1_1 ||
      moduleInfo.language === LANGUAGE_VERSION.ARKTS_HYBRID
    ) {
      return true;
    }
  }
  return false;
}

export function createTaskId(): string {
  const timestamp = Date.now().toString(36);
  const randomPart = Math.random().toString(36).slice(2, 6);
  return `task-${timestamp}-${randomPart}`;
}

export function serializeWithIgnore(obj: any, ignoreKeys: string[] = []): any {
  const jsonStr = JSON.stringify(obj, (key, value) => {
    if (typeof value === 'bigint') {
      return undefined;
    }
    if (ignoreKeys.includes(key)) {
      return undefined;
    }
    return value;
  });
  return JSON.parse(jsonStr);
}
