/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import { initBuildConfig } from './init/process_build_config';
import { BuildMode } from './build/build_mode';
import { Logger, LoggerGetter, getConsoleLogger, SubsystemCode } from './logger';
import { DriverError } from './util/error';
import { BuildConfig, BUILD_TYPE } from './types';

function initLogger(projectConfig: BuildConfig, loggerGetter: LoggerGetter | undefined): void {
  const hvigorLogger = projectConfig.getHvigorConsoleLogger as LoggerGetter | undefined;
  const defaultGetter: LoggerGetter = (code: SubsystemCode) => getConsoleLogger(code.valueOf());
  const getter = hvigorLogger ?? (loggerGetter ?? defaultGetter);
  Logger.getInstance(getter, projectConfig.enableDebugOutput);
}

export function build(projectConfig: BuildConfig, loggerGetter: LoggerGetter | undefined = undefined): void {
  initLogger(projectConfig, loggerGetter);
  const logger = Logger.getInstance();
  const buildConfig = initBuildConfig(projectConfig);

  try {
    if (projectConfig.enableDeclgenEts2Ts === true) {
      logger.printInfo('Declaration generation is deferred in ArkTS build system port.');
      return;
    }
    if (projectConfig.buildType === BUILD_TYPE.BUILD) {
      logger.printInfo('just build');
      const buildMode = new BuildMode(buildConfig);
      buildMode.run();
    }
  } catch (error) {
    if (error instanceof DriverError) {
      Logger.getInstance().printErrorAndExit((error as DriverError).logData);
    } else {
      throw error;
    }
  } finally {
    clean();
  }
}

function clean(): void {
  Logger.destroyInstance();
}
