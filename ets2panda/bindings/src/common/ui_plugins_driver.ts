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

import { KNativePointer } from './InteropTypes';
import { BuildConfig } from './types';

export enum PluginHook {
  NEW = 'afterNew',
  PARSED = 'parsed',
  SCOPE_INITED = 'scopeInited',
  CHECKED = 'checked',
  LOWERED = 'lowered',
  ASM_GENERATED = 'asmGenerated',
  BIN_GENERATED = 'binGenerated',
  CLEAN = 'clean'
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
  private contextPtr: KNativePointer | undefined;
  private codingFilePath: string | undefined;

  constructor() {
    this.ast = undefined;
    this.program = undefined;
    this.projectConfig = undefined;
    this.contextPtr = undefined;
    this.codingFilePath = undefined;
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

  public setContextPtr(ptr: KNativePointer): void {
    this.contextPtr = ptr;
  }

  public getContextPtr(): KNativePointer | undefined {
    return this.contextPtr;
  }

  public setCodingFilePath(codingFilePath: string): void {
    this.codingFilePath = codingFilePath;
  }

  public getCodingFilePath(): string | undefined {
    return this.codingFilePath;
  }

  public isCoding(): boolean {
    return this.codingFilePath !== undefined;
  }
}

export class PluginDriver {
  private static instance: PluginDriver | undefined;
  private sortedPlugins: Map<PluginHook, PluginExecutor[] | undefined>;
  private allPlugins: Map<string, Plugins>;
  private context: PluginContext;

  constructor() {
    this.sortedPlugins = new Map<PluginHook, PluginExecutor[] | undefined>();
    this.allPlugins = new Map<string, Plugins>();
    this.context = new PluginContext();
  }

  public static getInstance(): PluginDriver {
    if (!this.instance) {
      this.instance = new PluginDriver();
    }
    return this.instance;
  }

  public static destroyInstance(): void {
    PluginDriver.instance = undefined;
  }

  public initPlugins(projectConfig: BuildConfig): void {
    const pluginResults: RawPlugins[] = Object.entries(projectConfig.plugins).map(([key, value]) => {
      let pluginObject = require(value as string);
      let initFunction = Object.values(pluginObject)[0] as PluginInitFunction;
      return {
        name: key,
        init: initFunction
      };
    });

    pluginResults.forEach((plugin: RawPlugins) => {
      if (plugin.init !== undefined) {
        this.allPlugins.set(plugin.name, plugin.init());
      }
    });

    this.context.setProjectConfig(projectConfig);
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
        handler: typeof handler === 'object' ? handler.handler : handler
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
      return (executor.handler as Function).apply(this.context);
    });
  }

  public getPluginContext(): PluginContext {
    return this.context;
  }
}
