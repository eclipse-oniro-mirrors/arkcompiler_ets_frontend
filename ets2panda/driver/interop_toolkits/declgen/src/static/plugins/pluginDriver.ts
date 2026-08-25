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

import { BuildConfig } from '../../buildConfig';
import * as common from '@interop-toolkits/common';
import type { KPointer } from '../libarkts';
export enum PluginHook {
  NEW = 'afterNew',
  PARSED = 'parsed',
  SCOPE_INITED = 'scopeInited',
  CHECKED = 'checked',
  LOWERED = 'lowered',
  ASM_GENERATED = 'asmGenerated',
  BIN_GENERATED = 'binGenerated',
  CLEAN = 'clean',
}

type PluginHandlerFunction = () => void;

type PluginHandlerObject = {
  order: 'pre' | 'post' | undefined;
  handler: PluginHandlerFunction;
};

type PluginHandler = PluginHandlerFunction | PluginHandlerObject;

interface Plugins {
  name: string;
  afterNew?: PluginHandler;
  parsed?: PluginHandler;
  scopeInited?: PluginHandler;
  checked?: PluginHandler;
  lowered?: PluginHandler;
  asmGenerated?: PluginHandler;
  binGenerated?: PluginHandler;
  clean?: PluginHandler;
}

type PluginExecutor = {
  name: string;
  handler: PluginHandler;
};

type PluginInitFunction = () => Plugins;

type RawPlugins = {
  name: string;
  init: PluginInitFunction | undefined;
};

class PluginContext {
  private ast: object | undefined;
  private program: object | undefined;
  private projectConfig: object | undefined;
  private fileManager: PluginFileManager | undefined;
  private contextPtr: KPointer | undefined;

  constructor() {
    this.ast = undefined;
    this.program = undefined;
    this.projectConfig = undefined;
    this.fileManager = undefined;
    this.contextPtr = undefined;
  }

  public setArkTSAst(ast: object): void {
    this.ast = ast;
  }

  public getArkTSAst(): object | undefined {
    return this.ast;
  }

  public setArkTSProgram(program: object): void {
    this.program = program;
  }

  public getArkTSProgram(): object | undefined {
    return this.program;
  }

  public setProjectConfig(projectConfig: object): void {
    this.projectConfig = projectConfig;
  }

  public getProjectConfig(): object | undefined {
    return this.projectConfig;
  }

  public setFileManager(fileManager: PluginFileManager): void {
    this.fileManager = fileManager;
  }

  public getFileManager(): PluginFileManager | undefined {
    return this.fileManager;
  }

  public setContextPtr(ptr: KPointer): void {
    this.contextPtr = ptr;
  }

  public getContextPtr(): KPointer | undefined {
    return this.contextPtr;
  }
}

enum PluginLanguageVersion {
  ARKTS_1_2 = '1.2',
  ARKTS_1_1 = '1.1',
  ARKTS_HYBRID = 'hybrid',
}

interface PluginFileManager {
  getLanguageVersionByFilePath(filePath: string): PluginLanguageVersion;
}

export class PluginDriver {
  private sortedPlugins: Map<PluginHook, PluginExecutor[] | undefined>;
  private allPlugins: Map<string, Plugins>;
  private context: PluginContext | undefined;

  constructor(private fileManager: common.fileManager.FileManager) {
    this.sortedPlugins = new Map<PluginHook, PluginExecutor[] | undefined>();
    this.allPlugins = new Map<string, Plugins>();
    this.context = new PluginContext();
  }

  public setPluginContext(ctxPeer: KPointer): void {
    this.context!.setContextPtr(ctxPeer);
  }

  public initPlugins(projectConfig: BuildConfig): void {
    if (!projectConfig.plugins) {
      return;
    }

    const pluginResults: RawPlugins[] = [];

    Object.entries(projectConfig.plugins).forEach(([key, value]) => {
      try {
        let pluginObject = require(value as string);
        let initFunction = Object.values(pluginObject)[0] as PluginInitFunction;
        if (typeof initFunction !== 'function') {
          throw Error('Failed to load plugin: plugin in wrong format');
        }

        pluginResults.push({
          name: key,
          init: initFunction,
        });
      } catch (error) {
        if (error instanceof Error) {
          throw new Error(`Failed to load plugin: ${key}, ${value}, ${error.message}`);
        }
      }
    });

    pluginResults.forEach((plugin: RawPlugins) => {
      if (plugin.init !== undefined) {
        this.allPlugins.set(plugin.name, plugin.init());
      }
    });

    if (this.context !== undefined) {
      this.context.setProjectConfig(projectConfig);
      this.context.setFileManager(this.getFileManager());
    }
  }

  private getFileManager(): PluginFileManager {
    const isStaticFile = (filePath: string): boolean => {
      const fileManager = this.fileManager;
      return (
        fileManager.isStaticSourceFile(filePath) ||
        fileManager.isStaticSdkFile(filePath) ||
        fileManager.isStaticStdLibFile(filePath) ||
        fileManager.isStaticInteropFile(filePath) ||
        fileManager.isStaticInteropSdkFile(filePath)
      );
    };
    const isDynamicFile = (filePath: string): boolean => {
      const fileManager = this.fileManager;
      return (
        fileManager.isDynamicSourceFile(filePath) ||
        fileManager.isDynamicSdkFile(filePath) ||
        fileManager.isDynamicInteropFile(filePath) ||
        fileManager.isDynamicInteropSdkFile(filePath)
      );
    };
    return {
      getLanguageVersionByFilePath: (filePath: string): PluginLanguageVersion => {
        if (isStaticFile(filePath)) {
          return PluginLanguageVersion.ARKTS_1_2;
        } else if (isDynamicFile(filePath)) {
          return PluginLanguageVersion.ARKTS_1_1;
        } else {
          throw new Error(`File ${filePath} is not a static or dynamic file.`);
        }
      },
    };
  }

  private getPlugins(hook: PluginHook): PluginExecutor[] | undefined {
    if (!this.sortedPlugins.has(hook)) {
      const sortedPlugins: PluginExecutor[] = this.getSortedPlugins(hook);
      if (sortedPlugins.length === 0) {
        this.sortedPlugins.set(hook, undefined);
      } else {
        this.sortedPlugins.set(hook, sortedPlugins);
      }
    }

    return this.sortedPlugins.get(hook);
  }

  private getSortedPlugins(hook: PluginHook): PluginExecutor[] {
    let pre: PluginExecutor[] = [];
    let normal: PluginExecutor[] = [];
    let post: PluginExecutor[] = [];

    this.allPlugins.forEach((pluginObject: Plugins, name: string) => {
      if (!pluginObject[hook]) {
        return;
      }

      let pluginName: string = pluginObject.name;
      let handler: PluginHandler = pluginObject[hook]!;
      let order: string | undefined = typeof handler === 'object' ? handler.order : undefined;

      let rawPluginHook: PluginExecutor = {
        name: pluginName,
        handler: typeof handler === 'object' ? handler.handler : handler,
      };

      if (order === 'pre') {
        pre.push(rawPluginHook);
      } else if (order === 'post') {
        post.push(rawPluginHook);
      } else {
        normal.push(rawPluginHook);
      }
    });

    return [...pre, ...normal, ...post];
  }

  public runPluginHook(hook: PluginHook): void {
    let plugins: PluginExecutor[] | undefined = this.getPlugins(hook);
    if (!plugins) {
      return;
    }
    plugins.forEach((executor: PluginExecutor) => {
      let context = this.getPluginContext();
      return (executor.handler as Function).apply(context);
    });
  }

  public getPluginContext(): PluginContext {
    if (this.context === undefined) {
      throw new Error('Plugin context not initialized, pls call setPluginContext before');
    }
    return this.context;
  }
}
