import { Logger, LogDataFactory } from './logger';
import { BuildConfig } from './types';
import { initBuildConfig } from './init/process_build_config';
import { BuildMode } from './build/build_mode';
import { DriverError, ErrorCode } from './util/error';

function validateBuildConfigPath(buildConfigPath: string): void {
  if (buildConfigPath.length === 0) {
    throw new DriverError(LogDataFactory.newDetailedInstance(
      ErrorCode.BUILDSYSTEM_ENTRY_DISPATCH_FAIL,
      'Build config path is required.',
      'Expected exactly one build_config.json path argument.',
      '',
      ['Run the ArkTS driver with the demo build_config.json path.'],
      ''
    ));
  }
}

export function build(buildConfigPath: string): boolean {
  try {
    validateBuildConfigPath(buildConfigPath);
    let buildConfig: BuildConfig = initBuildConfig(buildConfigPath);
    let buildMode: BuildMode = new BuildMode(buildConfig);
    buildMode.run();
    return true;
  } catch (error) {
    if (error instanceof DriverError) {
      Logger.getInstance().printError(error.logData);
      return false;
    }
    Logger.getInstance().printWarn('Error occurred');
    Logger.getInstance().printWarn('Error is not DriverError');
    throw error;
  }
}
