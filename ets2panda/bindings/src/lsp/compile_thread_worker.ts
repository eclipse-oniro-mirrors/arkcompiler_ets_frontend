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

import { parentPort, workerData } from 'worker_threads';
import * as fs from 'fs';
import { PluginDriver, PluginHook } from '../common/ui_plugins_driver';
import { LspDriverHelper } from '../common/driver_helper';
import { Es2pandaContextState } from '../generated/Es2pandaEnums';
import { JobInfo } from '../common/types';

const { workerId } = workerData;

function compileExternalProgram(jobInfo: JobInfo): void {
  PluginDriver.getInstance().initPlugins(jobInfo.buildConfig);
  let ets2pandaCmd = ['-', '--extension', 'ets', '--arktsconfig', jobInfo.arktsConfigFile];
  let lspDriverHelper = new LspDriverHelper();
  let config = lspDriverHelper.createCfg(ets2pandaCmd, jobInfo.filePath);
  if (!fs.existsSync(jobInfo.filePath) || fs.statSync(jobInfo.filePath).isDirectory()) {
    return;
  }
  const source = fs.readFileSync(jobInfo.filePath, 'utf8').replace(/\r\n/g, '\n');
  let context = lspDriverHelper.createCtx(source, jobInfo.filePath, config, jobInfo.globalContextPtr, true);
  PluginDriver.getInstance().getPluginContext().setContextPtr(context);
  lspDriverHelper.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_PARSED);
  PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
  lspDriverHelper.proceedToState(context, Es2pandaContextState.ES2PANDA_STATE_LOWERED);
}

parentPort?.on('message', (msg) => {
  if (msg.type === 'ASSIGN_TASK') {
    const job = msg.jobInfo;
    if (!job.isValid) {
      compileExternalProgram(job);
    }

    parentPort?.postMessage({
      type: 'TASK_FINISH',
      jobId: job.id,
      workerId
    });
  } else if (msg.type === 'EXIT') {
    process.exit(0);
  }
});
