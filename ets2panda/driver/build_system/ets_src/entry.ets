import { Logger } from './logger';
import { PRE_DEFINE_READY } from './pre_define';
import { BUILD_CONFIG_NATIVE_HANDLE } from './types';
import { initBuildConfig } from './init/process_build_config';
import { BaseMode } from './build/base_mode';
import { BuildMode } from './build/build_mode';
import { generateArkTSConfig } from './build/generate_arktsconfig';
import { DriverError } from './util/error';
import { Ets2panda } from './util/ets2panda';
import { Graph } from './util/graph';
import { normalizePath } from './util/utils';
import { ObfuscationConfig } from './obfuscation/obfuscation_config';
import { PluginsDriver } from './plugins/plugins_driver';
import { nullptr } from '@arkts-bindings';

export function build(): boolean {
  Logger.getInstance();
  initBuildConfig();
  new BaseMode().run();
  new BuildMode().run();
  generateArkTSConfig();
  new DriverError();
  new Ets2panda();
  new Graph();
  normalizePath('');
  new ObfuscationConfig();
  new PluginsDriver();
  return PRE_DEFINE_READY && BUILD_CONFIG_NATIVE_HANDLE == nullptr;
}
