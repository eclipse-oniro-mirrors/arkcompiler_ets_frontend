/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

import { BaseMode } from './base_mode';
import {
  BuildConfig,
  ES2PANDA_MODE,
} from '../types';

export class BuildMode extends BaseMode {
  constructor(buildConfig: BuildConfig) {
    super(buildConfig);
  }

  public run(): void {
    const buildMode = this.buildConfig.es2pandaMode;
    if (buildMode === ES2PANDA_MODE.RUN_PARALLEL || buildMode === ES2PANDA_MODE.RUN_CONCURRENT) {
      this.logger.printInfo('Run sequential fallback for worker mode');
    } else if (buildMode === ES2PANDA_MODE.RUN_SIMULTANEOUS) {
      this.logger.printInfo('Run simultaneous');
    } else {
      this.logger.printInfo('Run ordinary');
    }
    super.run();
  }
}
