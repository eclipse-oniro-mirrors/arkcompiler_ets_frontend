/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import fs from 'fs';
import os from 'os';
import path from 'path';
import { PathConfig } from '../../src';
import { generateBuildConfigs, ModuleDescriptor } from '../../src/lsp/generateBuildConfig';
import { generateArkTsConfigs } from '../../src/lsp/generateArkTSConfig';

interface GeneratedArkTSConfig {
  compilerOptions: {
    paths: Record<string, string[]>;
    dependencies: Record<string, { language: string; path: string; sourceFilePath?: string; ohmUrl?: string }>;
  };
}

const mockProject = path.resolve('./test/ut/mock_hap/test_with_dynamic_dependency');

function createTempDir(): string {
  return fs.mkdtempSync(path.join(os.tmpdir(), 'arktsconfig-ut-'));
}

interface DeclFileEntry {
  declPath: string;
  filePath: string;
  ohmUrl: string;
}

function buildDeclFiles(): Record<string, DeclFileEntry> {
  return {
    index: {
      declPath: 'index.d.ets',
      filePath: path.join(mockProject, 'dynlib', 'index.ets'),
      ohmUrl: '@normalized:N&undefined&mock&dynlib/index&1.0.0'
    },
    Utils: {
      declPath: 'Utils.d.ets',
      filePath: path.join(mockProject, 'dynlib', 'Utils.ets'),
      ohmUrl: '@normalized:N&undefined&mock&dynlib/Utils&1.0.0'
    },
    'src/main/ets/Deep': {
      declPath: 'Deep.d.ets',
      filePath: path.join(mockProject, 'dynlib', 'src', 'main', 'ets', 'Deep.ets'),
      ohmUrl: '@normalized:N&undefined&mock&dynlib/src/main/ets/Deep&1.0.0'
    }
  };
}

function resolveDeclPaths(files: Record<string, DeclFileEntry>, declsDir: string): Record<string, DeclFileEntry> {
  const resolved: Record<string, DeclFileEntry> = {};
  Object.entries(files).forEach(([key, entry]) => {
    resolved[key] = { ...entry, declPath: path.join(declsDir, entry.declPath) };
  });
  return resolved;
}

function writeDeclFileInfo(declgenOutDir: string): void {
  const declDir = path.join(declgenOutDir, 'dynlib', 'declgen');
  fs.mkdirSync(path.join(declDir, 'decls'), { recursive: true });
  fs.mkdirSync(path.join(declDir, 'dynamic'), { recursive: true });
  fs.writeFileSync(
    path.join(declDir, 'decls', 'index.d.ets'),
    'declare function dMul(a: number, b: number): number;\n'
  );
  fs.writeFileSync(path.join(declDir, 'decls', 'Utils.d.ets'), 'declare class DUtil { static val: number; }\n');
  fs.writeFileSync(path.join(declDir, 'decls', 'Deep.d.ets'), 'declare class Deep {}\n');
  fs.writeFileSync(
    path.join(declDir, 'dynamic', 'decl-fileInfo.json'),
    JSON.stringify(
      { packageName: 'dynlib', files: resolveDeclPaths(buildDeclFiles(), path.join(declDir, 'decls')) },
      null,
      2
    )
  );
}

function writeBuildDeclFileInfo(buildDeclDir: string, declsDir: string): void {
  fs.mkdirSync(buildDeclDir, { recursive: true });
  fs.mkdirSync(declsDir, { recursive: true });
  fs.writeFileSync(path.join(declsDir, 'index.d.ets'), 'declare function dMul(a: number, b: number): number;\n');
  fs.writeFileSync(path.join(declsDir, 'Utils.d.ets'), 'declare class DUtil { static val: number; }\n');
  fs.writeFileSync(path.join(declsDir, 'Deep.d.ets'), 'declare class Deep {}\n');
  fs.writeFileSync(
    path.join(buildDeclDir, 'decl-fileInfo.json'),
    JSON.stringify({ packageName: 'dynlib', files: resolveDeclPaths(buildDeclFiles(), declsDir) }, null, 2)
  );
}

function generateStaticAppConfig(declgenOutDir: string, cacheDir: string): GeneratedArkTSConfig {
  const pathConfig: PathConfig = {
    buildSdkPath: '',
    projectPath: mockProject,
    declgenOutDir,
    cacheDir
  };
  const modules: ModuleDescriptor[] = [
    { name: 'staticapp', moduleType: 'har', srcPath: path.join(mockProject, 'staticapp') },
    { name: 'dynlib', moduleType: 'har', srcPath: path.join(mockProject, 'dynlib') }
  ];
  const buildConfigs = generateBuildConfigs(pathConfig, modules);
  expect(buildConfigs['staticapp'].language).toBe('1.2');
  expect(buildConfigs['dynlib'].language).toBe('1.1');
  expect(buildConfigs['staticapp'].dependencies).toStrictEqual(['dynlib']);
  generateArkTsConfigs(buildConfigs);
  return JSON.parse(fs.readFileSync(path.join(cacheDir, 'staticapp', 'arktsconfig.json'), 'utf-8'));
}

describe('generateArkTsConfigs dynamic dependency alias test', () => {
  let cacheDir: string;

  beforeEach(() => {
    cacheDir = createTempDir();
  });

  afterEach(() => {
    fs.rmSync(cacheDir, { recursive: true, force: true });
  });

  test('dynamic module aliases include package entry when decl file exists', () => {
    const declgenOutDir = createTempDir();
    try {
      writeDeclFileInfo(declgenOutDir);
      const config = generateStaticAppConfig(declgenOutDir, cacheDir);
      const dependencies = config.compilerOptions.dependencies;

      expect(Object.keys(dependencies)).toContain('dynlib/index');
      expect(Object.keys(dependencies)).toContain('dynlib/Utils');
      expect(Object.keys(dependencies)).toContain('dynlib');
      expect(dependencies['dynlib']).toStrictEqual(dependencies['dynlib/index']);
      // 'filePath' from the decl file info is carried into the dependency entry.
      expect(dependencies['dynlib/Utils'].sourceFilePath).toBe(path.join(mockProject, 'dynlib', 'Utils.ets'));
      expect(dependencies['dynlib'].sourceFilePath).toBe(path.join(mockProject, 'dynlib', 'index.ets'));
      // Source-root transformed keys follow the build system ('src/main' stripped).
      expect(Object.keys(dependencies)).toContain('dynlib/src/main/ets/Deep');
      expect(Object.keys(dependencies)).toContain('dynlib/ets/Deep');
      expect(dependencies['dynlib/ets/Deep']).toStrictEqual(dependencies['dynlib/src/main/ets/Deep']);
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  test('dynamic module decl file is picked up from the module build output', () => {
    const emptyDeclgenOutDir = createTempDir();
    const buildDeclDir = path.join(mockProject, 'dynlib', 'build', 'default', 'intermediates', 'declgen', 'default');
    try {
      writeBuildDeclFileInfo(buildDeclDir, path.join(emptyDeclgenOutDir, 'decls'));
      const config = generateStaticAppConfig(emptyDeclgenOutDir, cacheDir);
      const dependencies = config.compilerOptions.dependencies;

      expect(Object.keys(dependencies)).toContain('dynlib/index');
      expect(Object.keys(dependencies)).toContain('dynlib/Utils');
      expect(Object.keys(dependencies)).toContain('dynlib');
      expect(dependencies['dynlib']).toStrictEqual(dependencies['dynlib/index']);
    } finally {
      fs.rmSync(path.join(mockProject, 'dynlib', 'build'), { recursive: true, force: true });
      fs.rmSync(emptyDeclgenOutDir, { recursive: true, force: true });
    }
  });

  test('dynamic module aliases fall back to file level when decl file is missing', () => {
    const emptyDeclgenOutDir = createTempDir();
    try {
      const config = generateStaticAppConfig(emptyDeclgenOutDir, cacheDir);
      const dependencies = config.compilerOptions.dependencies;

      expect(Object.keys(dependencies).filter((key) => key.includes('dynlib'))).toStrictEqual([]);
      expect(config.compilerOptions.paths['staticapp']).toBeUndefined();
    } finally {
      fs.rmSync(emptyDeclgenOutDir, { recursive: true, force: true });
    }
  });
});

interface TempModuleSpec {
  name: string;
  files: Record<string, string>;
  dependencies?: Record<string, string>;
  descriptor?: Partial<ModuleDescriptor>;
}

function createTempProject(modules: TempModuleSpec[]): { projectRoot: string; descriptors: ModuleDescriptor[] } {
  const projectRoot = createTempDir();
  modules.forEach((module) => {
    const moduleRoot = path.join(projectRoot, module.name);
    Object.entries(module.files).forEach(([relPath, content]) => {
      const file = path.join(moduleRoot, relPath);
      fs.mkdirSync(path.dirname(file), { recursive: true });
      fs.writeFileSync(file, content);
    });
    fs.writeFileSync(
      path.join(moduleRoot, 'oh-package.json5'),
      JSON.stringify({ name: module.name, main: '', version: '1.0.0', dependencies: module.dependencies ?? {} }, null, 2)
    );
  });
  const descriptors = modules.map((module) => ({
    name: module.name,
    moduleType: 'har',
    srcPath: path.join(projectRoot, module.name),
    ...module.descriptor
  }));
  return { projectRoot, descriptors };
}

function writeDeclEntries(declgenOutDir: string, moduleName: string, entries: Record<string, string>): void {
  const declDir = path.join(declgenOutDir, moduleName, 'declgen', 'dynamic', 'decls');
  const files: Record<string, { declPath: string; filePath: string; ohmUrl: string }> = {};
  Object.entries(entries).forEach(([key, sourcePath]) => {
    const declPath = path.join(declDir, `${key}.d.ets`);
    fs.mkdirSync(path.dirname(declPath), { recursive: true });
    fs.writeFileSync(declPath, 'declare class C {}\n');
    files[key] = { declPath, filePath: sourcePath, ohmUrl: '' };
  });
  fs.mkdirSync(path.dirname(path.join(declDir, '..', 'decl-fileInfo.json')), { recursive: true });
  fs.writeFileSync(
    path.join(declDir, '..', 'decl-fileInfo.json'),
    JSON.stringify({ packageName: moduleName, files }, null, 2)
  );
}

function generateConsumerConfig(
  projectRoot: string,
  descriptors: ModuleDescriptor[],
  consumerName: string
): GeneratedArkTSConfig {
  const declgenOutDir = createTempDir();
  const cacheDir = createTempDir();
  try {
    const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
    const buildConfigs = generateBuildConfigs(pathConfig, descriptors);
    generateArkTsConfigs(buildConfigs);
    return JSON.parse(fs.readFileSync(path.join(cacheDir, consumerName, 'arktsconfig.json'), 'utf-8'));
  } finally {
    fs.rmSync(declgenOutDir, { recursive: true, force: true });
    fs.rmSync(cacheDir, { recursive: true, force: true });
  }
}

describe('generateArkTsConfigs module language and entry alignment test', () => {
  let projectRoot: string;

  afterEach(() => {
    if (projectRoot) {
      fs.rmSync(projectRoot, { recursive: true, force: true });
      projectRoot = undefined as unknown as string;
    }
  });

  test('module language declared on the descriptor wins over the file scan', () => {
    // The library files carry no 'use static' marker: without the declared version the
    // file scan would classify it as a 1.1 dynamic module.
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { declaredlib: 'file:../declaredlib' }
      },
      {
        name: 'declaredlib',
        files: { 'src/main/ets/Lib.ets': 'export class Lib {}\n' },
        descriptor: { arktsversion: '1.2' }
      }
    ]);
    projectRoot = spec.projectRoot;

    const config = generateConsumerConfig(spec.projectRoot, spec.descriptors, 'app');
    expect(config.compilerOptions.paths['declaredlib']).toStrictEqual([path.join(projectRoot, 'declaredlib')]);
  });

  test('standard hvigor layout entry registers the bare module name', () => {
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { hvlib: 'file:../hvlib' }
      },
      {
        name: 'hvlib',
        files: { 'src/main/ets/Index.ets': 'export class Entry {}\n', 'src/main/ets/utils/Foo.ets': 'export class Foo {}\n' }
      }
    ]);
    projectRoot = spec.projectRoot;
    const hvlibRoot = path.join(projectRoot, 'hvlib');
    const entrySource = path.join(hvlibRoot, 'src/main/ets/Index.ets');

    const declgenOutDir = createTempDir();
    try {
      writeDeclEntries(declgenOutDir, 'hvlib', {
        'src/main/ets/Index': entrySource,
        'src/main/ets/utils/Foo': path.join(hvlibRoot, 'src/main/ets/utils/Foo.ets')
      });
      const cacheDir = createTempDir();
      try {
        const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
        const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
        generateArkTsConfigs(buildConfigs);
        const config: GeneratedArkTSConfig = JSON.parse(
          fs.readFileSync(path.join(cacheDir, 'app', 'arktsconfig.json'), 'utf-8')
        );
        const dependencies = config.compilerOptions.dependencies;

        expect(Object.keys(dependencies)).toContain('hvlib/src/main/ets/Index');
        expect(Object.keys(dependencies)).toContain('hvlib');
        expect(dependencies['hvlib']).toStrictEqual(dependencies['hvlib/src/main/ets/Index']);
        expect(dependencies['hvlib'].sourceFilePath).toBe(entrySource);
      } finally {
        fs.rmSync(cacheDir, { recursive: true, force: true });
      }
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  test('entry file declared on the descriptor is matched against decl keys', () => {
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { customlib: 'file:../customlib' }
      },
      {
        name: 'customlib',
        files: { 'src/main/ets/Custom.ets': 'export class Custom {}\n' },
        descriptor: { entryFile: 'src/main/ets/Custom.ets' }
      }
    ]);
    projectRoot = spec.projectRoot;
    const customlibRoot = path.join(projectRoot, 'customlib');
    const entrySource = path.join(customlibRoot, 'src/main/ets/Custom.ets');

    const declgenOutDir = createTempDir();
    try {
      writeDeclEntries(declgenOutDir, 'customlib', { 'src/main/ets/Custom': entrySource });
      const cacheDir = createTempDir();
      try {
        const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
        const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
        generateArkTsConfigs(buildConfigs);
        const config: GeneratedArkTSConfig = JSON.parse(
          fs.readFileSync(path.join(cacheDir, 'app', 'arktsconfig.json'), 'utf-8')
        );
        const dependencies = config.compilerOptions.dependencies;

        expect(Object.keys(dependencies)).toContain('customlib');
        expect(dependencies['customlib']).toStrictEqual(dependencies['customlib/src/main/ets/Custom']);
      } finally {
        fs.rmSync(cacheDir, { recursive: true, force: true });
      }
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  // A static (1.2) dependency also contributes its declaration aliases when a
  // decl-fileInfo exists: the build system resolves the same alias forms through the
  // binary dependency, and the declaration entries carry the transformed ('lib/ets/X')
  // and entry ('lib') keys the paths module root cannot resolve.
  test('static dependency with decl file also contributes dependency aliases', () => {
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { staticlib: 'file:../staticlib' }
      },
      {
        name: 'staticlib',
        files: { 'src/main/ets/Foo.ets': "'use static';\nexport class Foo {}\n" },
        descriptor: { entryFile: 'src/main/ets/Foo.ets' }
      }
    ]);
    projectRoot = spec.projectRoot;
    const staticlibRoot = path.join(projectRoot, 'staticlib');

    const declgenOutDir = createTempDir();
    try {
      writeDeclEntries(declgenOutDir, 'staticlib', {
        'src/main/ets/Foo': path.join(staticlibRoot, 'src/main/ets/Foo.ets')
      });
      const cacheDir = createTempDir();
      try {
        const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
        const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
        generateArkTsConfigs(buildConfigs);
        const config: GeneratedArkTSConfig = JSON.parse(
          fs.readFileSync(path.join(cacheDir, 'app', 'arktsconfig.json'), 'utf-8')
        );
        const dependencies = config.compilerOptions.dependencies;

        expect(Object.keys(dependencies)).toContain('staticlib/src/main/ets/Foo');
        expect(Object.keys(dependencies)).toContain('staticlib/ets/Foo');
        expect(Object.keys(dependencies)).toContain('staticlib');
        expect(dependencies['staticlib'].sourceFilePath).toBe(path.join(staticlibRoot, 'src/main/ets/Foo.ets'));
      } finally {
        fs.rmSync(cacheDir, { recursive: true, force: true });
      }
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  // Without a decl-fileInfo the static dependency stays on the paths module root only
  // (no dependency aliases, no warning).
  test('static dependency without decl file keeps paths-only behaviour', () => {
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { plainlib: 'file:../plainlib' }
      },
      {
        name: 'plainlib',
        files: { 'src/main/ets/Plain.ets': "'use static';\nexport class Plain {}\n" }
      }
    ]);
    projectRoot = spec.projectRoot;

    const config = generateConsumerConfig(spec.projectRoot, spec.descriptors, 'app');

    expect(config.compilerOptions.paths['plainlib']).toStrictEqual([path.join(projectRoot, 'plainlib')]);
    expect(Object.keys(config.compilerOptions.dependencies).filter((key) => key.startsWith('plainlib'))).toStrictEqual(
      []
    );
  });

  // The dependency collection is transitive, mirroring the build system's flattened
  // dependency graph: a dynamic module pulled in through another module also contributes
  // its declaration aliases to the consumer config.
  test('transitive dynamic dependency contributes its declaration aliases', () => {
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { middleware: 'file:../middleware' }
      },
      {
        name: 'middleware',
        files: { 'src/main/ets/Mid.ets': 'export class Mid {}\n' },
        dependencies: { translib: 'file:../translib' }
      },
      {
        name: 'translib',
        files: { 'src/main/ets/Trans.ets': 'export class Trans {}\n' },
        descriptor: { entryFile: 'src/main/ets/Trans.ets' }
      }
    ]);
    projectRoot = spec.projectRoot;
    const translibRoot = path.join(projectRoot, 'translib');

    const declgenOutDir = createTempDir();
    try {
      writeDeclEntries(declgenOutDir, 'translib', {
        'src/main/ets/Trans': path.join(translibRoot, 'src/main/ets/Trans.ets')
      });
      const cacheDir = createTempDir();
      try {
        const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
        const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
        generateArkTsConfigs(buildConfigs);
        const config: GeneratedArkTSConfig = JSON.parse(
          fs.readFileSync(path.join(cacheDir, 'app', 'arktsconfig.json'), 'utf-8')
        );
        const dependencies = config.compilerOptions.dependencies;

        expect(Object.keys(dependencies)).toContain('translib/src/main/ets/Trans');
        expect(Object.keys(dependencies)).toContain('translib');
      } finally {
        fs.rmSync(cacheDir, { recursive: true, force: true });
      }
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  test('hybrid dependency keeps the module-root mapping alongside per-file aliases', () => {
    const spec = createTempProject([
      {
        name: 'app',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" },
        dependencies: { hyblib: 'file:../hyblib' }
      },
      {
        name: 'hyblib',
        files: {
          'Static.ets': "'use static';\nexport class Static {}\n",
          'Plain.ets': 'export class Plain {}\n'
        }
      }
    ]);
    projectRoot = spec.projectRoot;
    const hyblibRoot = path.join(projectRoot, 'hyblib');

    const config = generateConsumerConfig(spec.projectRoot, spec.descriptors, 'app');

    expect(config.compilerOptions.paths['hyblib']).toStrictEqual([hyblibRoot]);
    expect(config.compilerOptions.paths['hyblib/Static']).toStrictEqual([path.join(hyblibRoot, 'Static.ets')]);
  });
});

describe('generateArkTsConfigs self module dynamic dependency test', () => {
  let projectRoot: string;

  afterEach(() => {
    if (projectRoot) {
      fs.rmSync(projectRoot, { recursive: true, force: true });
      projectRoot = undefined as unknown as string;
    }
  });

  // Mirrors the build system (base_mode.ts getDependencyModules): a 1.1 module joins
  // its own dynamic dependency set, so its own decl-fileInfo entries appear in its own
  // arktsconfig as '<package>/<file>' dependencies ('entry/xxx/dynFile' keys).
  test('own dynamic decl entries are included for a 1.1 module', () => {
    const spec = createTempProject([
      {
        name: 'entry',
        files: {
          'src/main/ets/Dyn.ets': 'export class Dyn {}\n',
          'src/main/ets/other/Helper.ets': 'export class Helper {}\n'
        },
        descriptor: { entryFile: 'src/main/ets/Dyn.ets' }
      }
    ]);
    projectRoot = spec.projectRoot;
    const entryRoot = path.join(projectRoot, 'entry');
    const dynSource = path.join(entryRoot, 'src/main/ets/Dyn.ets');

    const declgenOutDir = createTempDir();
    try {
      writeDeclEntries(declgenOutDir, 'entry', {
        'src/main/ets/Dyn': dynSource,
        'src/main/ets/other/Helper': path.join(entryRoot, 'src/main/ets/other/Helper.ets')
      });
      const cacheDir = createTempDir();
      try {
        const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
        const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
        expect(buildConfigs['entry'].language).toBe('1.1');
        generateArkTsConfigs(buildConfigs);
        const config: GeneratedArkTSConfig = JSON.parse(
          fs.readFileSync(path.join(cacheDir, 'entry', 'arktsconfig.json'), 'utf-8')
        );
        const dependencies = config.compilerOptions.dependencies;

        expect(Object.keys(dependencies)).toContain('entry/src/main/ets/Dyn');
        expect(Object.keys(dependencies)).toContain('entry/ets/Dyn');
        expect(Object.keys(dependencies)).toContain('entry/src/main/ets/other/Helper');
        expect(Object.keys(dependencies)).toContain('entry');
        expect(dependencies['entry']).toStrictEqual(dependencies['entry/src/main/ets/Dyn']);
        expect(dependencies['entry'].sourceFilePath).toBe(dynSource);
      } finally {
        fs.rmSync(cacheDir, { recursive: true, force: true });
      }
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  // The module's own decl-fileInfo is the materialized interop-declaration dynamic
  // entry list, so it decides on its own: hvigor classifies such a module as 1.1/hybrid
  // while the descriptor 'arktsversion' (or the '.ets'-only file scan that misses the
  // '.ts' dynamic file) reports '1.2' - the entries must be emitted regardless.
  test('own dynamic decl entries are included for a module classified 1.2 by the descriptor', () => {
    const spec = createTempProject([
      {
        name: 'entry',
        files: {
          'src/main/ets/Static.ets': "'use static';\nexport class Static {}\n",
          'src/main/ets/dyn/DynCalc.ts': 'export function dynCalc(): number { return 1; }\n'
        },
        descriptor: { arktsversion: '1.2', entryFile: 'src/main/ets/Static.ets' }
      }
    ]);
    projectRoot = spec.projectRoot;
    const entryRoot = path.join(projectRoot, 'entry');

    const declgenOutDir = createTempDir();
    try {
      writeDeclEntries(declgenOutDir, 'entry', {
        'src/main/ets/dyn/DynCalc': path.join(entryRoot, 'src/main/ets/dyn/DynCalc.ts')
      });
      const cacheDir = createTempDir();
      try {
        const pathConfig: PathConfig = { buildSdkPath: '', projectPath: projectRoot, declgenOutDir, cacheDir };
        const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
        expect(buildConfigs['entry'].language).toBe('1.2');
        generateArkTsConfigs(buildConfigs);
        const config: GeneratedArkTSConfig = JSON.parse(
          fs.readFileSync(path.join(cacheDir, 'entry', 'arktsconfig.json'), 'utf-8')
        );
        const dependencies = config.compilerOptions.dependencies;

        expect(Object.keys(dependencies)).toContain('entry/src/main/ets/dyn/DynCalc');
        expect(Object.keys(dependencies)).toContain('entry/ets/dyn/DynCalc');
        expect(dependencies['entry/ets/dyn/DynCalc'].sourceFilePath).toBe(
          path.join(entryRoot, 'src/main/ets/dyn/DynCalc.ts')
        );
      } finally {
        fs.rmSync(cacheDir, { recursive: true, force: true });
      }
    } finally {
      fs.rmSync(declgenOutDir, { recursive: true, force: true });
    }
  });

  // Without a decl-fileInfo manifest the module owns no interop dynamic entries and its
  // own dependencies stay absent, whatever the language classification reports.
  test('module without a decl manifest keeps its own dependencies absent', () => {
    const spec = createTempProject([
      {
        name: 'entry',
        files: { 'src/main/ets/Dyn.ets': 'export class Dyn {}\n' }
      }
    ]);
    projectRoot = spec.projectRoot;

    const config = generateConsumerConfig(spec.projectRoot, spec.descriptors, 'entry');
    const dependencies = config.compilerOptions.dependencies;

    expect(Object.keys(dependencies).filter((key) => key.startsWith('entry'))).toStrictEqual([]);
  });
});

describe('generateArkTsConfigs interop sdk dependency alias test', () => {
  let projectRoot: string;
  let interopApiPath: string;

  afterEach(() => {
    if (projectRoot) {
      fs.rmSync(projectRoot, { recursive: true, force: true });
      projectRoot = undefined as unknown as string;
    }
    if (interopApiPath) {
      fs.rmSync(interopApiPath, { recursive: true, force: true });
      interopApiPath = undefined as unknown as string;
    }
  });

  // Mirrors the build system (generate_arktsconfig.ts processDynamicFile): each interop
  // SDK dependency carries itself as an alias, so the dynamic declarations' own bare
  // imports (e.g. "@ohos.lang.interop") resolve through the dependency-alias lookup.
  test('interop sdk dependencies carry the self-referencing aliases', () => {
    const spec = createTempProject([
      {
        name: 'entry',
        files: { 'src/main/ets/App.ets': "'use static';\nexport let a = 1;\n" }
      }
    ]);
    projectRoot = spec.projectRoot;

    interopApiPath = createTempDir();
    const apiDir = path.join(interopApiPath, 'api');
    fs.mkdirSync(apiDir, { recursive: true });
    fs.writeFileSync(path.join(apiDir, '@ohos.lang.interop.d.ets'), 'declare namespace es {}\n');
    fs.writeFileSync(path.join(apiDir, '@ohos.hilog.d.ets'), 'export declare function debug(...args: string[]): void;\n');

    const cacheDir = createTempDir();
    try {
      const pathConfig: PathConfig = {
        buildSdkPath: '',
        projectPath: projectRoot,
        declgenOutDir: '',
        cacheDir,
        interopApiPath
      };
      const buildConfigs = generateBuildConfigs(pathConfig, spec.descriptors);
      generateArkTsConfigs(buildConfigs);
      const config: GeneratedArkTSConfig = JSON.parse(
        fs.readFileSync(path.join(cacheDir, 'entry', 'arktsconfig.json'), 'utf-8')
      );
      const dependencies = config.compilerOptions.dependencies;

      expect(dependencies['dynamic@ohos.lang.interop']).toMatchObject({
        language: 'js',
        alias: ['@ohos.lang.interop', 'dynamic@ohos.lang.interop']
      });
      expect(dependencies['dynamic@ohos.hilog']).toMatchObject({
        language: 'js',
        alias: ['@ohos.hilog', 'dynamic@ohos.hilog']
      });
    } finally {
      fs.rmSync(cacheDir, { recursive: true, force: true });
    }
  });
});

describe('generateBuildConfigs interop sdk path', () => {
  test('interop api path defaults to the dynamic sdk root without a declaration level', () => {
    const pathConfig: PathConfig = {
      buildSdkPath: path.resolve('test', 'ets', 'static'),
      projectPath: mockProject,
      declgenOutDir: ''
    };
    const modules: ModuleDescriptor[] = [
      {
        name: 'staticapp',
        moduleType: 'har',
        srcPath: path.join(mockProject, 'staticapp')
      }
    ];
    const buildConfigs = generateBuildConfigs(pathConfig, modules);
    expect(buildConfigs['staticapp'].interopApiPath).toBe(
      path.resolve(pathConfig.buildSdkPath, '../dynamic/build-tools/interop')
    );
  });

  test('interop api path keeps the explicit override', () => {
    const customInteropPath = path.resolve('test', 'ets', 'dynamic', 'build-tools', 'interop');
    const pathConfig: PathConfig = {
      buildSdkPath: path.resolve('test', 'ets', 'static'),
      projectPath: mockProject,
      declgenOutDir: '',
      interopApiPath: customInteropPath
    };
    const modules: ModuleDescriptor[] = [
      {
        name: 'staticapp',
        moduleType: 'har',
        srcPath: path.join(mockProject, 'staticapp')
      }
    ];
    const buildConfigs = generateBuildConfigs(pathConfig, modules);
    expect(buildConfigs['staticapp'].interopApiPath).toBe(customInteropPath);
  });
});
