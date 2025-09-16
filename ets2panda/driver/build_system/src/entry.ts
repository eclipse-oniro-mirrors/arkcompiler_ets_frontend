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

import * as fs from 'fs';
import * as path from 'path';

import { processBuildConfig } from './init/process_build_config';
import { BuildMode } from './build/build_mode';
import { Logger } from './logger';
import { ArkTSConfigGenerator } from './build/generate_arktsconfig';
import { PluginDriver } from './plugins/plugins_driver';
import { BuildConfig, BUILD_TYPE } from './types';
import { BuildFrameworkMode } from './build/build_framework_mode';
import { cleanKoalaModule } from './init/init_koala_modules';

export async function build(projectConfig: BuildConfig): Promise<void> {
  let logger: Logger = Logger.getInstance(projectConfig);
  let buildConfig: BuildConfig = processBuildConfig(projectConfig);

  buildConfig.entryFiles = buildConfig.compileFiles;
  if (projectConfig.frameworkMode === true) {
    let buildframeworkMode: BuildFrameworkMode = new BuildFrameworkMode(buildConfig);
    await buildframeworkMode.run();
    if (logger.hasErrors()) {
      clean();
      process.exit(1);
    }
  } else if (projectConfig.enableDeclgenEts2Ts === true) {
    let buildMode: BuildMode = new BuildMode(buildConfig);
    await buildMode.generateDeclaration();
  } else if (projectConfig.buildType === BUILD_TYPE.BUILD) {
    let buildMode: BuildMode = new BuildMode(buildConfig);
    await buildMode.run();
  }

  clean();
}

function clean(): void {
  Logger.destroyInstance();
  ArkTSConfigGenerator.destroyInstance();
  PluginDriver.destroyInstance();
  cleanKoalaModule();
}

function main(): void {
  const buildConfigPath: string = path.resolve(process.argv[2]);
  const projectConfig: BuildConfig = JSON.parse(fs.readFileSync(buildConfigPath, 'utf-8'));

  build(projectConfig);
}

if (require.main === module) {
  main();
}
