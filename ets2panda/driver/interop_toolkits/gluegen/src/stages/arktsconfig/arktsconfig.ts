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

export interface DependencyItem {
  readonly language: string;
  readonly path: string;
  readonly ohmUrl: string;
  readonly sourceFilePath?: string;
  readonly alias?: readonly string[];
  readonly mainFile?: string;
}

export interface CompilerOptions {
  readonly package: string;
  readonly baseUrl: string;
  readonly rootDir: string;
  readonly paths: Readonly<Record<string, readonly string[]>>;
  readonly dependencies: Readonly<Record<string, DependencyItem>>;
  readonly useEmptyPackage?: boolean;
  readonly cacheDir?: string;
  readonly declgenV2OutPath: string;
  readonly mock?: Readonly<Record<string, { readonly source: string }>>;
}

export interface ArkTSConfig {
  readonly compilerOptions: CompilerOptions;
}

/** The only value published by the arktsconfig Stage. */
export type ArkTSConfigPath = string & {
  readonly __arkTSConfigPath: 'ArkTSConfigPath';
};
