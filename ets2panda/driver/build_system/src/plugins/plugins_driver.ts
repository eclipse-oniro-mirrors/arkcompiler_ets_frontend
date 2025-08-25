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

import {
  Logger,
  LogData,
  LogDataFactory
} from '../logger';
import { BuildConfig } from '../types';
import { ErrorCode } from '../error_code';
import { FileManager } from './FileManager';
import path from 'path'
import { MEMO_PLUGIN_PATH_FROM_SDK, UI_PLUGIN_PATH_FROM_SDK } from '../pre_define';

export enum PluginHook {
  NEW = 'afterNew',
  PARSED = 'parsed',
  SCOPE_INITED = 'scopeInited',
  CHECKED = 'checked',
  LOWERED = 'lowered',
  ASM_GENERATED = 'asmGenerated',
  BIN_GENERATED = 'binGenerated',
  CLEAN = 'clean',
};

type PluginHandlerFunction = () => void;

type PluginHandlerObject = {
  order: 'pre' | 'post' | undefined
  handler: PluginHandlerFunction
};

type PluginHandler = PluginHandlerFunction | PluginHandlerObject;

interface Plugins {
  name: string,
  afterNew?: PluginHandler,
  parsed?: PluginHandler,
  scopeInited?: PluginHandler,
  checked?: PluginHandler,
  lowered?: PluginHandler,
  asmGenerated?: PluginHandler,
  binGenerated?: PluginHandler,
  clean?: PluginHandler,
}

type PluginExecutor = {
  name: string
  handler: PluginHandler
};

type PluginInitFunction = () => Plugins;

type RawPlugins = {
  name: string,
  init: PluginInitFunction | undefined
};

class PluginContext {
  private ast: object | undefined;
  private program: object | undefined;
  private projectConfig: object | undefined;
  private fileManager: FileManager | undefined;
  private contextPtr: number | undefined;

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

  public setFileManager(projectConfig: BuildConfig): void {
    if (!this.fileManager) {
      FileManager.init(projectConfig);
      this.fileManager = FileManager.getInstance();
    }
  }

  public getFileManager(): FileManager | undefined{
    return this.fileManager;
  }

  public setContextPtr(ptr: number): void {
    this.contextPtr = ptr;
  }

  public getContextPtr(): number | undefined {
      return this.contextPtr;
  }
}

export class PluginDriver {
  private static instance: PluginDriver | undefined;
  private sortedPlugins: Map<PluginHook, PluginExecutor[] | undefined>;
  private allPlugins: Map<string, Plugins>;
  private context: PluginContext;
  private logger: Logger = Logger.getInstance();

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

  private initKoalaPlugins(projectConfig: BuildConfig): void {

    const uiPluginPath = path.resolve(projectConfig.buildSdkPath, UI_PLUGIN_PATH_FROM_SDK);
    const memoPluginPath = path.resolve(projectConfig.buildSdkPath, MEMO_PLUGIN_PATH_FROM_SDK);

    // TODO: need change in hvigor
    if (process.env.USE_KOALA_UI_PLUGIN) {
      projectConfig.plugins.ArkUI = uiPluginPath
    }

    if (process.env.USE_KOALA_MEMO_PLUGIN) {
      projectConfig.plugins['ArkUI-Memo'] = memoPluginPath
    }
  }

  public initPlugins(projectConfig: BuildConfig): void {
    if (!projectConfig || !projectConfig.plugins) {
      return;
    }

    this.initKoalaPlugins(projectConfig)

    const pluginResults: RawPlugins[] = Object.entries(projectConfig.plugins).map(([key, value]) => {
      try {
        let pluginObject = require(value as string);
        let initFunction = Object.values(pluginObject)[0] as PluginInitFunction;
        if (typeof initFunction !== 'function') {
          throw ('Failed to load plugin: plugin in wrong format');
        }
        this.logger.printInfo(`Loaded plugin: ', ${key}, ${pluginObject}`);

        return {
          name: key,
          init: initFunction
        };
      } catch (error) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_LOAD_PLUGIN_FAIL,
          'Failed to load plugin.',
          error as string
        );
        this.logger.printError(logData);
        return {
          name: key,
          init: undefined
        };
      }
    });

    pluginResults.forEach((plugin: RawPlugins) => {
      if (plugin.init !== undefined) {
        this.allPlugins.set(plugin.name, plugin.init());
      }
    });

    this.context.setProjectConfig(projectConfig);
    this.context.setFileManager(projectConfig);
  }

  private getPlugins(hook: PluginHook) : PluginExecutor[] | undefined {
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
      if (!(pluginObject[hook])) {
        return;
      }

      let pluginName: string = pluginObject.name;
      let handler: PluginHandler = pluginObject[hook];
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
      this.logger.printInfo(`executing plugin: ${executor.name}`);
      return (executor.handler as Function).apply(this.context);
    });
  }

  public getPluginContext(): PluginContext {
    return this.context;
  }
}