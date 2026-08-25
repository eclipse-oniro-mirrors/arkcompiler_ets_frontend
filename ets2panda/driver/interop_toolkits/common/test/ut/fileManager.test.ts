/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import * as fs from 'node:fs';
import * as os from 'node:os';
import * as path from 'node:path';

import { FileManagerBuilder, Owner, type FileManagerModuleInput } from '../../src/fileManager';
import { Language } from '../../src/fileUtils';

describe('FileManager metadata', () => {
  let projectRootPath: string;

  beforeEach(() => {
    projectRootPath = fs.mkdtempSync(path.join(os.tmpdir(), 'common-files-'));
  });

  afterEach(() => {
    fs.rmSync(projectRootPath, { recursive: true, force: true });
  });

  it('registers SDK and module files in pathToFileMetaMap', () => {
    const staticSdkFile = createFile('sdk/static/@ohos.static.d.ets');
    const dynamicSdkFile = createFile('sdk/dynamic/@ohos.dynamic.d.ts');
    const staticInteropFile = createFile('sdk/static-interop/@ohos.staticInterop.d.ts');
    const dynamicInteropFile = createFile('sdk/dynamic-interop/@ohos.dynamicInterop.d.ets');
    const unindexedSdkFile = createFile('sdk/static/nonPlatform.d.ets');
    const ignoredSdkFile = createFile('sdk/static/ignored.ets');
    const staticModuleFile = createFile('entry/src/static/Static.ets');
    const dynamicModuleFile = createFile('entry/src/dynamic/Dynamic.ets');
    const moduleConfig: FileManagerModuleInput = {
      packageName: 'entry.package',
      moduleName: 'entry',
      modulePath: path.join(projectRootPath, 'entry'),
      language: 'hybrid',
      packageVersion: '1.0.0',
      declgenV1OutPath: path.join(projectRootPath, 'declgen-v1'),
      declgenV2OutPath: path.join(projectRootPath, 'declgen-v2'),
      staticFiles: [staticModuleFile],
      dynamicFiles: [dynamicModuleFile],
    };
    const manager = new FileManagerBuilder()
      .addStaticSdkPath(path.dirname(staticSdkFile))
      .addDynamicSdkPaths([path.dirname(dynamicSdkFile)])
      .addStaticInteropSdkPath(path.dirname(staticInteropFile))
      .addDynamicInteropSdkPaths([path.dirname(dynamicInteropFile)])
      .addModule(moduleConfig)
      .addModuleList([])
      .build();

    expect(manager.queryFileMeta(staticSdkFile)).toMatchObject({
      fileName: '@ohos.static.d.ets',
      filePath: staticSdkFile,
      language: Language.STATIC,
      owner: Owner.SDK,
    });
    expect(manager.queryFileMeta(dynamicSdkFile)).toMatchObject({
      language: Language.DYNAMIC,
      owner: Owner.SDK,
    });
    expect(manager.queryFileMeta(staticInteropFile)).toMatchObject({
      language: Language.DYNAMIC,
      owner: Owner.INTEROP_SDK,
    });
    expect(manager.queryFileMeta(dynamicInteropFile)).toMatchObject({
      language: Language.STATIC,
      owner: Owner.INTEROP_SDK,
    });
    expect(manager.queryFileMeta(unindexedSdkFile)).toMatchObject({ language: Language.STATIC, owner: Owner.SDK });
    expect(manager.queryFileMeta(ignoredSdkFile)).toBeUndefined();
    expect(manager.queryStaticSdkPath('@ohos.static')).toBe(staticSdkFile);
    expect(manager.queryDynamicSdkPath('@ohos.dynamic')).toBe(dynamicSdkFile);
    expect(manager.queryStaticInteropSdkPath('@ohos.staticInterop')).toBe(staticInteropFile);
    expect(manager.queryDynamicInteropSdkPath('@ohos.dynamicInterop')).toBe(dynamicInteropFile);
    expect(manager.queryStaticSdkPath('nonPlatform')).toBeUndefined();
    expect(manager.queryStaticSdkPath('@ohos.unknown')).toBeUndefined();
    expect(manager.queryDynamicSdkPath('@ohos.unknown')).toBeUndefined();
    expect(manager.queryStaticInteropSdkPath('@ohos.unknown')).toBeUndefined();
    expect(manager.queryDynamicInteropSdkPath('@ohos.unknown')).toBeUndefined();
    expect(manager.isDynamicSourceFile(staticInteropFile)).toBe(false);
    expect(manager.isStaticSourceFile(dynamicInteropFile)).toBe(false);
    expect(manager.queryInteropDeclarationFile(staticInteropFile)).toBeUndefined();
    expect(manager.queryInteropDeclarationFile(dynamicInteropFile)).toBeUndefined();

    const staticMeta = manager.queryFileMeta(staticModuleFile);
    const dynamicMeta = manager.queryFileMeta(dynamicModuleFile);
    expect(staticMeta).toMatchObject({ language: Language.STATIC, owner: Owner.MODULE });
    expect(dynamicMeta).toMatchObject({ language: Language.DYNAMIC, owner: Owner.MODULE });
    expect(staticMeta?.module).toBe(dynamicMeta?.module);
    expect(staticMeta?.module).toMatchObject({
      language: Language.HYBRID,
      moduleName: 'entry',
      packageName: 'entry.package',
      packageVersion: '1.0.0',
      staticFiles: new Set([staticModuleFile]),
      dynamicFiles: new Set([dynamicModuleFile]),
    });
    expect(manager.queryModuleInfo('entry.package')).toBe(staticMeta?.module);
    expect(manager.queryModuleInfo('unknown.package')).toBeUndefined();
    const staticDeclarationFile = path.join(projectRootPath, 'declgen-v1', 'src/static/Static.d.ets');
    const dynamicDeclarationFile = path.join(projectRootPath, 'declgen-v2', 'src/dynamic/Dynamic.d.ets');
    expect(manager.queryInteropDeclarationFile(staticModuleFile)).toBe(staticDeclarationFile);
    expect(manager.queryInteropDeclarationFile(dynamicModuleFile)).toBe(dynamicDeclarationFile);
    expect(manager.querySourceFile(staticDeclarationFile)).toBe(staticModuleFile);
    expect(manager.querySourceFile(dynamicDeclarationFile)).toBe(dynamicModuleFile);
    expect(manager.isStaticSourceFile(staticModuleFile)).toBe(true);
    expect(manager.isDynamicSourceFile(staticModuleFile)).toBe(false);
    expect(manager.isSourceFile(staticModuleFile)).toBe(true);
    expect(manager.isDynamicSourceFile(dynamicModuleFile)).toBe(true);
    expect(manager.isStaticSourceFile(dynamicModuleFile)).toBe(false);
    expect(manager.isSourceFile(dynamicModuleFile)).toBe(true);
    expect(manager.isStaticInteropFile(staticDeclarationFile)).toBe(true);
    expect(manager.isDynamicInteropFile(staticDeclarationFile)).toBe(false);
    expect(manager.isDynamicInteropFile(dynamicDeclarationFile)).toBe(true);
    expect(manager.isStaticInteropFile(dynamicDeclarationFile)).toBe(false);
    expect(manager.isSourceFile(staticDeclarationFile)).toBe(false);
    expect(manager.isSourceFile(dynamicDeclarationFile)).toBe(false);
    const unknownFile = path.join(projectRootPath, 'unknown.ets');
    expect(manager.queryFileMeta(unknownFile)).toBeUndefined();
    expect(manager.isStaticSourceFile(unknownFile)).toBe(false);
    expect(manager.isDynamicSourceFile(unknownFile)).toBe(false);
    expect(manager.isSourceFile(unknownFile)).toBe(false);
    expect(manager.isStaticInteropFile(unknownFile)).toBe(false);
    expect(manager.isDynamicInteropFile(unknownFile)).toBe(false);
    expect(manager.queryInteropDeclarationFile(staticSdkFile)).toBeUndefined();
    expect(manager.querySourceFile(staticInteropFile)).toBeUndefined();
    expect(manager.isStaticSourceFile(staticSdkFile)).toBe(false);
    expect(manager.isSourceFile(staticSdkFile)).toBe(false);
    expect(manager.isSourceFile(dynamicSdkFile)).toBe(false);
    expect(manager.isSourceFile(staticInteropFile)).toBe(false);
    expect(manager.isSourceFile(dynamicInteropFile)).toBe(false);
    expect(manager.isStaticInteropFile(staticInteropFile)).toBe(false);
    expect(manager.staticSourceFiles).toEqual(new Set([staticModuleFile]));
    expect(manager.dynamicSourceFiles).toEqual(new Set([dynamicModuleFile]));
  });

  it('recognizes files under a static standard library path', () => {
    const staticStdLibFile = createFile('stdlib/core/Runtime.d.ets');
    const manager = new FileManagerBuilder().addStaticStdLibPath(path.join(projectRootPath, 'stdlib')).build();

    expect(manager.isStaticStdLibFile(staticStdLibFile)).toBe(true);
    expect(manager.isStaticStdLibFile(path.join(projectRootPath, 'other/Runtime.ets'))).toBe(false);
    expect(manager.isSdkFile(staticStdLibFile)).toBe(false);
    expect(manager.queryFileMeta(staticStdLibFile)).toBeUndefined();
  });

  it('does not map module sources without a declaration output path', () => {
    const staticModuleFile = createFile('entry/Static.ets');
    const dynamicModuleFile = createFile('entry/Dynamic.ets');
    const manager = new FileManagerBuilder()
      .addModule({
        packageName: 'entry.package',
        modulePath: path.join(projectRootPath, 'entry'),
        staticFiles: [staticModuleFile],
        dynamicFiles: [dynamicModuleFile],
      })
      .build();

    expect(manager.queryInteropDeclarationFile(staticModuleFile)).toBeUndefined();
    expect(manager.queryInteropDeclarationFile(dynamicModuleFile)).toBeUndefined();
  });

  it('rejects duplicate module package names', () => {
    const moduleConfig: FileManagerModuleInput = {
      packageName: 'entry.package',
      modulePath: path.join(projectRootPath, 'entry'),
      staticFiles: [],
      dynamicFiles: [],
    };
    const builder = new FileManagerBuilder().addModule(moduleConfig);

    expect(() => builder.addModuleList([moduleConfig])).toThrow('Module "entry.package" has already been added.');
  });

  function createFile(relativePath: string): string {
    const filePath = path.join(projectRootPath, relativePath);
    fs.mkdirSync(path.dirname(filePath), { recursive: true });
    fs.writeFileSync(filePath, '');
    return filePath;
  }
});
