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

import fs from 'fs';
import os from 'os';
import path from 'path';
import { global as Es2pandaGlobal } from '../../src/common/global';
import { Lsp } from '../../src/lsp/lsp_helper';
import { PathConfig } from '../../src';
import { ModuleDescriptor } from '../../src/lsp/generateBuildConfig';

// The fixture mirrors one consumer module ('entry') with two source files sharing the same
// generated arktsconfig - the per-file module info mapping must not duplicate the config's
// import keys - whose arktsconfig dependencies register the dynamic 'har' module under two
// import keys, both pointing at the same declaration file.
const declPath = path.resolve('/mock/har/decls/Index.d.ets');
const importerFile = path.resolve('/mock/entry/Importer.ets');
const queryFile = path.resolve('/mock/entry/Query.ets');
const arktsConfigFile = path.resolve('/mock/cache/entry/arktsconfig.json');
const importerContent = "import { HarHelper } from 'har';\n";

interface RecordedCalls {
  refresh: Array<[string, string, string]>;
  removedIndex: string[];
  prepared: any[][];
  indexed: Array<[string, string | undefined]>;
}

function createLspForTest(): { lsp: any; calls: RecordedCalls; memGuard: () => number } {
  const lsp = Object.create(Lsp.prototype) as any;
  lsp.moduleInfos = {
    [importerFile]: { packageName: 'entry', arktsConfigFile },
    [queryFile]: { packageName: 'entry', arktsConfigFile }
  };
  lsp.dynamicDeclDependenciesCache = new Map([
    [
      arktsConfigFile,
      {
        har: { path: declPath },
        'har/Index': { path: declPath }
      }
    ]
  ]);
  lsp.dynamicDeclImportKeys = undefined;
  lsp.indexedDynamicDecls = new Set([declPath]);
  lsp.filesMap = new Map([
    [
      importerFile,
      {
        fileContent: importerContent,
        fileConfig: 1,
        fileContext: 100,
        fileHash: 'hash',
        arktsConfigPath: arktsConfigFile
      }
    ]
  ]);
  lsp.inMemoryFileContents = new Map();
  lsp.consecutiveEditsCount = 0;
  lsp.pendingDeclIndexByPath = new Map();
  lsp.pendingDeclIndexTimer = undefined;
  lsp.failedDeclIndexHashes = new Map();
  lsp.disposed = false;

  const calls: RecordedCalls = { refresh: [], removedIndex: [], prepared: [], indexed: [] };
  // A native-shaped es2pandaPublic with only the refresh entry point: the prototype fallback
  // is replaced, so the capability check reports support and the flow records the calls.
  (Es2pandaGlobal as any)._es2pandaPublic = {
    _RefreshExternalProgram: (_context: any, importKey: string, decl: string, text: string): number => {
      calls.refresh.push([importKey, decl, text]);
      return 0;
    }
  };
  lsp.tryRemoveSymbolReferenceIndex = (file: string): void => {
    calls.removedIndex.push(file);
  };
  lsp.prepareFileCache = (...args: any[]): any => {
    calls.prepared.push(args);
  };
  lsp.indexDynamicDeclFile = (file: string, content?: string): void => {
    calls.indexed.push([file, content]);
  };
  let memGuardCalls = 0;
  lsp.checkAndProactivelyRecycleBeforeEdit = (): void => {
    memGuardCalls++;
  };
  lsp.scheduleIdleContextRecycle = (): void => {};
  return { lsp, calls, memGuard: () => memGuardCalls };
}

describe('Lsp dynamic decl refresh', () => {
  test('modifyFilesMap routes a dynamic decl edit through the external refresh', () => {
    const { lsp, calls, memGuard } = createLspForTest();
    const newDecl = 'declare class RenamedHelper {\n    static val: number;\n}\n';

    lsp.modifyFilesMap(declPath, { newDoc: newDecl });

    // The edit only refreshes the static content cache (no context-side program work on
    // the keystroke path): every import key that resolves to the declaration is refreshed
    // with the new content, the virtual dependency keys first and the absolute path
    // (direct introductions) last.
    expect(calls.refresh).toEqual([
      ['har', declPath, newDecl],
      ['har/Index', declPath, newDecl],
      [declPath, declPath, newDecl]
    ]);
    // The file becomes re-indexable; the stale index contributions are NOT dropped at
    // edit time - they pair with the deferred re-registration.
    expect(calls.removedIndex).toEqual([]);
    // No consumer recompile on the edit: it lands with the staged batch.
    expect(calls.prepared).toEqual([]);
    expect(calls.indexed).toEqual([]);
    expect(lsp.indexedDynamicDecls.has(declPath)).toBe(false);
    expect(lsp.pendingDeclIndexByPath.get(declPath)).toBe(newDecl);
    expect(lsp.pendingDeclIndexTimer).toBeDefined();
    // The declaration edit runs the memory guard as well.
    expect(memGuard()).toBe(1);

    // The staged batch lands once the burst settles: the context-side program refresh
    // re-runs against the active context, the stale contributions are removed and the
    // exports re-registered through a temporary context, and the active consumer is
    // force-recompiled so its imports re-resolve - the decl edit itself never becomes
    // the active file.
    lsp.flushPendingDeclIndex();
    expect(calls.refresh.length).toBe(6);
    expect(calls.removedIndex).toEqual([declPath]);
    expect(calls.indexed).toEqual([[declPath, newDecl]]);
    expect(calls.prepared).toEqual([[importerFile, { newDoc: importerContent }, false, true]]);
    expect(lsp.pendingDeclIndexByPath.size).toBe(0);

    expect(lsp.filesMap.size).toBe(1);
    expect(lsp.filesMap.has(declPath)).toBe(false);
    expect(lsp.inMemoryFileContents.get(declPath)).toBe(newDecl);
  });

  test('a burst of decl edits re-indexes once with the latest content', () => {
    const { lsp, calls } = createLspForTest();
    const first = 'declare class FirstEdit {}\n';
    const second = 'declare class SecondEdit {}\n';
    const third = 'declare class ThirdEdit {}\n';

    lsp.modifyFilesMap(declPath, { newDoc: first });
    lsp.modifyFilesMap(declPath, { newDoc: second });
    lsp.modifyFilesMap(declPath, { newDoc: third });

    // One shared timer, one staged content per declaration - the immediate text-cache
    // refresh still ran for every keystroke (the consumers see every intermediate).
    expect(calls.refresh.length).toBe(9);
    expect(lsp.pendingDeclIndexByPath.get(declPath)).toBe(third);

    lsp.flushPendingDeclIndex();

    // The batch carries exactly one context-side refresh, one registration and one
    // consumer recompile for the whole burst.
    expect(calls.refresh.length).toBe(12);
    expect(calls.indexed).toEqual([[declPath, third]]);
    expect(calls.prepared).toEqual([[importerFile, { newDoc: importerContent }, false, true]]);
  });

  test('modifyFilesMap keeps the regular edit path for project files', () => {
    const { lsp, calls } = createLspForTest();

    lsp.modifyFilesMap(importerFile, { newDoc: importerContent });

    expect(calls.refresh).toEqual([]);
    expect(calls.removedIndex).toEqual([]);
    expect(calls.prepared).toEqual([[importerFile, { newDoc: importerContent }, true]]);
    expect(lsp.consecutiveEditsCount).toBe(1);
  });

  test('deleteFile routes a dynamic decl through the refresh with empty content', () => {
    const { lsp, calls } = createLspForTest();
    lsp.inMemoryFileContents.set(declPath, 'old decl');

    lsp.deleteFile(declPath);

    expect(calls.removedIndex).toEqual([declPath]);
    // The empty content keeps the import resolvable-but-empty instead of serving stale exports.
    expect(calls.refresh).toEqual([
      ['har', declPath, ''],
      ['har/Index', declPath, ''],
      [declPath, declPath, '']
    ]);
    // The exports are not re-registered: a temporary context built through the disk fallback
    // would re-introduce the stale exports the refresh has just removed.
    expect(calls.indexed).toEqual([]);
    expect(lsp.pendingDeclIndexByPath.has(declPath)).toBe(false);
    // Staying after the flush: nothing stages a deleted declaration back into the index.
    lsp.flushPendingDeclIndex();
    expect(calls.indexed).toEqual([]);
    expect(calls.prepared).toEqual([[importerFile, { newDoc: importerContent }, false, true]]);
    expect(lsp.inMemoryFileContents.has(declPath)).toBe(false);
  });

  test('a deleted declaration cancels its staged re-index', () => {
    const { lsp, calls } = createLspForTest();
    lsp.modifyFilesMap(declPath, { newDoc: 'declare class DoomedExport {}\n' });
    expect(lsp.pendingDeclIndexByPath.has(declPath)).toBe(true);

    lsp.deleteFile(declPath);

    expect(lsp.pendingDeclIndexByPath.has(declPath)).toBe(false);
    lsp.flushPendingDeclIndex();
    expect(calls.indexed).toEqual([]);
  });

  test('the import key view is rebuilt after update regenerates the arktsconfig files', () => {
    const { lsp } = createLspForTest();
    expect(lsp.isDynamicDeclFile(declPath)).toBe(true);

    lsp.updateModuleInfos = undefined;
    lsp.dynamicDeclImportKeys = undefined;
    // update() resets the lazily built view; the next lookup rebuilds it from the (mocked)
    // arktsconfig dependencies cache, so an entry removed there stops matching.
    lsp.dynamicDeclDependenciesCache = new Map([[arktsConfigFile, {}]]);
    expect(lsp.isDynamicDeclFile(declPath)).toBe(false);
  });

  test('refresh without an active context still refreshes the static content cache', () => {
    const { lsp, calls } = createLspForTest();
    lsp.filesMap = new Map();
    const newDecl = 'declare class RenamedHelper {\n    static val: number;\n}\n';

    lsp.modifyFilesMap(declPath, { newDoc: newDecl });

    // The content cache refresh needs no live context; only the consumer recompile is
    // skipped because there is no active file to force-recompile. The re-registration is
    // staged (flushed later); the stale-index removal pairs with it there.
    expect(calls.refresh).toEqual([
      ['har', declPath, newDecl],
      ['har/Index', declPath, newDecl],
      [declPath, declPath, newDecl]
    ]);
    expect(calls.removedIndex).toEqual([]);
    expect(calls.prepared).toEqual([]);
  });

  test('a failed decl index build is not retried until the content changes', () => {
    const lsp = Object.create(Lsp.prototype) as any;
    lsp.indexedDynamicDecls = new Set();
    lsp.failedDeclIndexHashes = new Map();
    lsp.destroyContext = (): void => {};
    let createAttempts = 0;
    lsp.createContext = (..._args: any[]): any => {
      createAttempts++;
      return undefined; // the declaration cannot be checked on this SDK
    };
    const declsDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-decl-fail-'));
    try {
      const declPath = path.join(declsDir, 'Broken.d.ets');
      fs.writeFileSync(declPath, 'declare class BrokenExport {}\n');

      // Without the failure record every references/completion query of any consumer
      // would rebuild the full checked context for the uncheckable declaration.
      lsp.indexDynamicDeclFile(declPath);
      lsp.indexDynamicDeclFile(declPath);
      lsp.indexDynamicDeclFile(declPath);
      expect(createAttempts).toBe(1);
      expect(lsp.failedDeclIndexHashes.has(declPath)).toBe(true);

      // A regenerated declaration gets exactly one new attempt.
      fs.writeFileSync(declPath, 'declare class RegeneratedExport {}\n');
      lsp.indexDynamicDeclFile(declPath);
      expect(createAttempts).toBe(2);

      // A successful build registers the declaration and clears the record; the content
      // changed once more so the suppressed fingerprint no longer matches.
      fs.writeFileSync(declPath, 'declare class FixedExport {}\n');
      const previousNative = (Es2pandaGlobal as any)._es2panda;
      (Es2pandaGlobal as any)._es2panda = { _buildSymbolReferenceIndexForContext: (): void => {} };
      lsp.createContext = (..._args: any[]): any => {
        createAttempts++;
        return [{}, 1];
      };
      try {
        lsp.indexDynamicDeclFile(declPath);
      } finally {
        (Es2pandaGlobal as any)._es2panda = previousNative;
      }
      expect(createAttempts).toBe(3);
      expect(lsp.indexedDynamicDecls.has(declPath)).toBe(true);
      expect(lsp.failedDeclIndexHashes.has(declPath)).toBe(false);

      // A vanished declaration never reaches a context attempt.
      const goneAttempts = createAttempts;
      lsp.indexDynamicDeclFile(path.join(declsDir, 'Gone.d.ets'));
      expect(createAttempts).toBe(goneAttempts);
    } finally {
      fs.rmSync(declsDir, { recursive: true, force: true });
    }
  });

  test('applyConfigSync refreshes only the declarations changed since the last sync', () => {
    const { lsp, calls } = createLspForTest();
    const declsDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-mtime-'));
    const freshDecl = path.join(declsDir, 'Fresh.d.ets');
    const staleDecl = path.join(declsDir, 'Stale.d.ets');
    const freshContent = 'declare class FreshExport { static val: number; }\n';
    const staleContent = 'declare class StaleExport { static val: number; }\n';
    try {
      fs.writeFileSync(freshDecl, freshContent);
      fs.writeFileSync(staleDecl, staleContent);
      // The untouched declaration predates the last sync (mtime below the mark); the
      // regenerated one keeps its current mtime so the filter selects it.
      const beforeLastSync = new Date(500);
      fs.utimesSync(staleDecl, beforeLastSync, beforeLastSync);

      lsp.dynamicDeclDependenciesCache = new Map([
        [arktsConfigFile, { har: { path: freshDecl }, 'har/Stale': { path: staleDecl } }]
      ]);
      lsp.dynamicDeclImportKeys = undefined;
      lsp.lastDeclSyncTimestamp = 1000;

      lsp.applyConfigSync();

      // Only the changed declaration is read and refreshed under its import keys; the
      // untouched one is filtered out by its mtime and never reaches the refresh.
      expect(calls.refresh).toEqual([
        ['har', freshDecl, freshContent],
        [freshDecl, freshDecl, freshContent]
      ]);
      expect(calls.removedIndex).toEqual([freshDecl]);
      expect(calls.indexed).toEqual([[freshDecl, freshContent]]);
      // A declaration-only change force-recompiles the active consumer; the context survives.
      expect(calls.prepared).toEqual([[importerFile, { newDoc: importerContent }, false, true]]);
      expect(lsp.lastDeclSyncTimestamp).toBeGreaterThan(1000);
    } finally {
      fs.rmSync(declsDir, { recursive: true, force: true });
    }
  });

  test('a cleared editor buffer re-indexes the empty source instead of the disk text', () => {
    const { lsp, calls } = createLspForTest();
    const declsDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-empty-buffer-'));
    try {
      // The disk copy keeps the previous declaration (the buffer was cleared but not
      // saved): a truthiness-based source fallback would parse the disk text and
      // re-register the stale exports the refresh has just dropped.
      const diskDecl = path.join(declsDir, 'Index.d.ets');
      fs.writeFileSync(diskDecl, 'declare class DiskStaleExport { static val: number; }\n');
      lsp.dynamicDeclDependenciesCache = new Map([[arktsConfigFile, { har: { path: diskDecl } }]]);
      lsp.dynamicDeclImportKeys = undefined;
      lsp.failedDeclIndexHashes = new Map();
      // The real indexing path is needed: the recorded source is captured below the
      // context creation, at the native context entry point.
      delete lsp.indexDynamicDeclFile;
      lsp.getArkTsConfigPath = (): string => arktsConfigFile;
      lsp.proceedPreparedContext = (): void => {};
      lsp.destroyContext = (): void => {};
      const ctxSources: string[] = [];
      lsp.lspDriverHelper = {
        createCfg: (): any => ({}),
        createCtx: (source: string): any => {
          ctxSources.push(source);
          return 7;
        },
        proceedToState: (): void => {},
        destroyContext: (): void => {},
        destroyConfig: (): void => {}
      } as any;
      const previousNative = (Es2pandaGlobal as any)._es2panda;
      (Es2pandaGlobal as any)._es2panda = { _buildSymbolReferenceIndexForContext: (): void => {} };
      try {
        lsp.modifyFilesMap(diskDecl, { newDoc: '' });
        // The re-index is staged by the edit; run the deferred flush explicitly.
        lsp.flushPendingDeclIndex();
      } finally {
        (Es2pandaGlobal as any)._es2panda = previousNative;
      }

      // The temporary context parses the empty buffer as-is: the disk text must not leak
      // back in, and the declaration becomes re-indexable from the empty program.
      expect(ctxSources).toEqual(['']);
      // The edit refreshed the text cache, the flush re-ran the context-side refresh.
      expect(calls.refresh).toEqual([
        ['har', diskDecl, ''],
        [diskDecl, diskDecl, ''],
        ['har', diskDecl, ''],
        [diskDecl, diskDecl, '']
      ]);
      expect(lsp.indexedDynamicDecls.has(diskDecl)).toBe(true);
    } finally {
      fs.rmSync(declsDir, { recursive: true, force: true });
    }
  });

  test('a declaration written during the scan is picked up by the next sync', () => {
    const { lsp, calls } = createLspForTest();
    const declsDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-during-'));
    try {
      const declA = path.join(declsDir, 'A.d.ets');
      const declB = path.join(declsDir, 'B.d.ets');
      fs.writeFileSync(declA, 'declare class AExport { static val: number; }\n');
      fs.writeFileSync(declB, 'declare class BExport { static val: number; }\n');
      // The kernel rounds write timestamps up to its clock granularity, so a natural
      // mtime can sit a few milliseconds in the future of a later Date.now(): A's age is
      // pinned explicitly to keep it below every watermark this test records.
      const beforeSync = new Date(Date.now() - 60000);
      fs.utimesSync(declA, beforeSync, beforeSync);
      fs.utimesSync(declB, beforeSync, beforeSync);
      lsp.dynamicDeclDependenciesCache = new Map([[arktsConfigFile, { a: { path: declA }, b: { path: declB } }]]);
      lsp.dynamicDeclImportKeys = undefined;
      lsp.lastDeclSyncTimestamp = 0;

      // An external declgen task rewrites B's timestamps while the first sync is already
      // scanning: the write lands between the sync start (the watermark this sync records)
      // and its end. The busy-wait below keeps the sync running past the write.
      const realGetKeys = lsp.getDynamicDeclImportKeys.bind(lsp);
      let writeDuringScan = true;
      lsp.getDynamicDeclImportKeys = (): any => {
        if (writeDuringScan) {
          writeDuringScan = false;
          const duringScan = new Date(Date.now() + 1);
          fs.utimesSync(declB, duringScan, duringScan);
        }
        return realGetKeys();
      };
      const realIndex = lsp.indexDynamicDeclFile;
      lsp.indexDynamicDeclFile = (file: string, content?: string): void => {
        const syncEnd = Date.now() + 20;
        while (Date.now() < syncEnd) {}
        realIndex(file, content);
      };

      lsp.applyConfigSync();
      lsp.applyConfigSync();

      // B was rewritten after the first sync started and must be refreshed again by the
      // second one; A predates the first sync and stays filtered out.
      const refreshCount = (match: string): number => calls.refresh.filter(([importKey]) => importKey === match).length;
      expect(refreshCount('a')).toBe(1);
      expect(refreshCount('b')).toBe(2);
      expect(refreshCount(declB)).toBe(2);
    } finally {
      fs.rmSync(declsDir, { recursive: true, force: true });
    }
  });
});

describe('Lsp dynamic decl refresh on sync', () => {
  // update() regenerates the arktsconfig files in place and must drop the dependencies
  // parse cache together with the reverse view: a surviving path-keyed entry keeps
  // serving the pre-sync parse, and a freshly generated declaration entry stays
  // invisible to the refresh routing - its edits would take the regular project-file
  // path and its exports would never register. The flow mirrors a real session: a
  // query fills the cache with the first generation, declgen adds a declaration, the
  // sync rewrites the config - the rebuilt view must see the new entry.
  test('update drops the arktsconfig dependencies parse cache alongside the reverse view', () => {
    const projectRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-cache-project-'));
    const declgenOutDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-cache-declgen-'));
    const cacheDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-cache-config-'));
    const dynlibRoot = path.join(projectRoot, 'dynlib');
    const entryRoot = path.join(projectRoot, 'entry');
    const declsDir = path.join(declgenOutDir, 'dynlib', 'declgen', 'dynamic', 'decls');
    const manifestPath = path.join(declgenOutDir, 'dynlib', 'declgen', 'dynamic', 'decl-fileInfo.json');
    const writeDynlibDecls = (keys: string[]): void => {
      const files: Record<string, { declPath: string; filePath: string; ohmUrl: string }> = {};
      keys.forEach((key) => {
        const declPath = path.join(declsDir, `${key}.d.ets`);
        fs.mkdirSync(path.dirname(declPath), { recursive: true });
        fs.writeFileSync(declPath, `declare class ${key}Export { static val: number; }\n`);
        files[key] = { declPath, filePath: path.join(dynlibRoot, 'src/main/ets', `${key}.ets`), ohmUrl: '' };
      });
      fs.writeFileSync(manifestPath, JSON.stringify({ packageName: 'dynlib', files }, null, 2));
    };
    try {
      // entry consumes dynlib (a dynamic 1.1 module) through its oh-package dependencies.
      fs.mkdirSync(path.join(entryRoot, 'src/main/ets'), { recursive: true });
      fs.writeFileSync(path.join(entryRoot, 'src/main/ets/App.ets'), "'use static';\nexport let a = 1;\n");
      fs.writeFileSync(
        path.join(entryRoot, 'oh-package.json5'),
        JSON.stringify(
          { name: 'entry', main: '', version: '1.0.0', dependencies: { dynlib: 'file:../dynlib' } },
          null,
          2
        )
      );
      fs.mkdirSync(path.join(dynlibRoot, 'src/main/ets'), { recursive: true });
      fs.writeFileSync(path.join(dynlibRoot, 'src/main/ets/Index.ets'), 'export class DynIndex {}\n');
      fs.writeFileSync(
        path.join(dynlibRoot, 'oh-package.json5'),
        JSON.stringify({ name: 'dynlib', main: '', version: '1.0.0' }, null, 2)
      );

      const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
      const descriptors: ModuleDescriptor[] = [
        { name: 'entry', moduleType: 'har', srcPath: entryRoot },
        { name: 'dynlib', moduleType: 'har', srcPath: dynlibRoot }
      ];
      const lsp = Object.create(Lsp.prototype) as any;
      lsp.pathConfig = pathConfig;
      lsp.buildConfigs = {};
      lsp.moduleInfos = {};
      lsp.reverseModuleDeps = {};
      lsp.dynamicDeclDependenciesCache = new Map();
      lsp.dynamicDeclImportKeys = undefined;
      // The closure/invalidation collaborators are orthogonal to the cache contract.
      lsp.collectInvalidationRoots = (): string[] => [];
      lsp.buildReverseModuleDeps = (): Record<string, string[]> => ({});
      lsp.invalidateReverseClosureCache = (): void => {};
      let configSyncs = 0;
      lsp.applyConfigSync = (): void => {
        configSyncs++;
      };

      // First generation: the manifest covers exactly the declaration directory.
      writeDynlibDecls(['Index']);
      lsp.update(descriptors);

      // A query before the sync fills the parse cache with the first-generation parse.
      const indexDeclPath = path.join(declsDir, 'Index.d.ets');
      expect(lsp.isDynamicDeclFile(indexDeclPath)).toBe(true);
      expect(lsp.dynamicDeclDependenciesCache.size).toBeGreaterThan(0);

      // The declgen task generates a new declaration; the sync rewrites the arktsconfig
      // in place. Before it runs, the new entry is unknown to the view.
      writeDynlibDecls(['Index', 'New']);
      const newDeclPath = path.join(declsDir, 'New.d.ets');
      expect(lsp.isDynamicDeclFile(newDeclPath)).toBe(false);
      lsp.update(descriptors);

      // The parse cache is dropped, not reused: the sync's declaration diff rebuilds the
      // reverse view through a fresh parse of the rewritten config, so the repopulated
      // cache serves the new generation and the new declaration resolves with its import
      // key. (A surviving pre-sync entry would keep the new entry invisible.)
      expect(lsp.dynamicDeclDependenciesCache.size).toBeGreaterThan(0);
      expect(lsp.isDynamicDeclFile(newDeclPath)).toBe(true);
      expect(lsp.getDynamicDeclImportKeys().get(path.resolve(newDeclPath))).toContain('dynlib/New');
      expect(configSyncs).toBe(2);
    } finally {
      [projectRoot, declgenOutDir, cacheDir].forEach((dir) => fs.rmSync(dir, { recursive: true, force: true }));
    }
  });

  // A declaration removed between two syncs (its source was deleted and the manifest no
  // longer lists it) must leave the global symbol reference index: only Sync Now arrived,
  // no separate deleteFile notification, and destroying the active context does not clear
  // the per-file index contributions either - auto import would keep suggesting the
  // deleted symbols.
  test('update expunges the declarations removed by the sync', () => {
    const projectRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-drop-project-'));
    const declgenOutDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-drop-declgen-'));
    const cacheDir = fs.mkdtempSync(path.join(os.tmpdir(), 'lsp-sync-drop-config-'));
    const dynlibRoot = path.join(projectRoot, 'dynlib');
    const entryRoot = path.join(projectRoot, 'entry');
    const declsDir = path.join(declgenOutDir, 'dynlib', 'declgen', 'dynamic', 'decls');
    const manifestPath = path.join(declgenOutDir, 'dynlib', 'declgen', 'dynamic', 'decl-fileInfo.json');
    const writeDynlibDecls = (keys: string[]): void => {
      const files: Record<string, { declPath: string; filePath: string; ohmUrl: string }> = {};
      keys.forEach((key) => {
        const declPath = path.join(declsDir, `${key}.d.ets`);
        fs.mkdirSync(path.dirname(declPath), { recursive: true });
        fs.writeFileSync(declPath, `declare class ${key}Export { static val: number; }\n`);
        files[key] = { declPath, filePath: path.join(dynlibRoot, 'src/main/ets', `${key}.ets`), ohmUrl: '' };
      });
      fs.writeFileSync(manifestPath, JSON.stringify({ packageName: 'dynlib', files }, null, 2));
    };
    try {
      fs.mkdirSync(path.join(entryRoot, 'src/main/ets'), { recursive: true });
      fs.writeFileSync(path.join(entryRoot, 'src/main/ets/App.ets'), "'use static';\nexport let a = 1;\n");
      fs.writeFileSync(
        path.join(entryRoot, 'oh-package.json5'),
        JSON.stringify(
          { name: 'entry', main: '', version: '1.0.0', dependencies: { dynlib: 'file:../dynlib' } },
          null,
          2
        )
      );
      fs.mkdirSync(path.join(dynlibRoot, 'src/main/ets'), { recursive: true });
      fs.writeFileSync(path.join(dynlibRoot, 'src/main/ets/Index.ets'), 'export class DynIndex {}\n');
      fs.writeFileSync(
        path.join(dynlibRoot, 'oh-package.json5'),
        JSON.stringify({ name: 'dynlib', main: '', version: '1.0.0' }, null, 2)
      );

      const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
      const descriptors: ModuleDescriptor[] = [
        { name: 'entry', moduleType: 'har', srcPath: entryRoot },
        { name: 'dynlib', moduleType: 'har', srcPath: dynlibRoot }
      ];
      const lsp = Object.create(Lsp.prototype) as any;
      lsp.pathConfig = pathConfig;
      lsp.buildConfigs = {};
      lsp.moduleInfos = {};
      lsp.reverseModuleDeps = {};
      lsp.dynamicDeclDependenciesCache = new Map();
      lsp.dynamicDeclImportKeys = undefined;
      lsp.indexedDynamicDecls = new Set();
      const removedIndex: string[] = [];
      lsp.tryRemoveSymbolReferenceIndex = (file: string): void => {
        removedIndex.push(file);
      };
      lsp.collectInvalidationRoots = (): string[] => [];
      lsp.buildReverseModuleDeps = (): Record<string, string[]> => ({});
      lsp.invalidateReverseClosureCache = (): void => {};
      lsp.applyConfigSync = (): void => {};

      // First generation: the manifest covers both declarations and both are indexed.
      writeDynlibDecls(['Index', 'New']);
      lsp.update(descriptors);
      const keptDeclPath = path.join(declsDir, 'Index.d.ets');
      const removedDeclPath = path.join(declsDir, 'New.d.ets');
      lsp.indexedDynamicDecls.add(keptDeclPath);
      lsp.indexedDynamicDecls.add(removedDeclPath);

      // The source behind 'New' is deleted and the manifest drops it; the next sync
      // regenerates the config without the entry.
      writeDynlibDecls(['Index']);
      fs.rmSync(removedDeclPath);
      lsp.update(descriptors);

      // The dropped declaration leaves the global symbol index and the indexed set; the
      // kept one stays untouched.
      expect(removedIndex).toEqual([removedDeclPath]);
      expect(lsp.indexedDynamicDecls.has(removedDeclPath)).toBe(false);
      expect(lsp.indexedDynamicDecls.has(keptDeclPath)).toBe(true);
    } finally {
      [projectRoot, declgenOutDir, cacheDir].forEach((dir) => fs.rmSync(dir, { recursive: true, force: true }));
    }
  });
});
