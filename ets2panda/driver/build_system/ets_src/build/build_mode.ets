import { Logger, LogDataFactory } from '../logger';
import { BuildConfig, ES2PANDA_MODE } from '../types';
import { DriverError, ErrorCode } from '../util/error';
import { BaseMode } from './base_mode';

export class BuildMode {
  private buildConfig: BuildConfig;
  private baseMode: BaseMode;

  public constructor(buildConfig: BuildConfig, baseMode: BaseMode | undefined = undefined) {
    this.buildConfig = buildConfig;
    this.baseMode = baseMode ?? new BaseMode(buildConfig);
  }

  public run(): void {
    this.assertSequentialMode();
    this.assertFirstIterationFeatures();
    Logger.getInstance().printInfo('Accepted sequential ArkTS build dispatch.');
    this.baseMode.run();
  }

  private assertSequentialMode(): void {
    if (this.buildConfig.es2pandaMode !== undefined && this.buildConfig.es2pandaMode !== ES2PANDA_MODE.RUN) {
      this.throwUnsupported('Unsupported es2panda mode.', 'unsupported-build-mode', this.buildConfig.es2pandaMode as string);
    }
    let rawConfig: Record<string, Object> = this.buildConfig as Record<string, Object>;
    if (this.hasEnabledDispatchField(rawConfig, 'externalProjectConfig') || this.hasEnabledDispatchField(rawConfig, 'externalProjectPath') ||
      this.hasEnabledDispatchField(rawConfig, 'processWorker') || this.hasEnabledDispatchField(rawConfig, 'threadWorker') ||
      this.hasEnabledDispatchField(rawConfig, 'useProcessWorker') || this.hasEnabledDispatchField(rawConfig, 'useThreadWorker')) {
      this.throwUnsupported('Unsupported first-iteration dispatch mode.', 'unsupported-build-mode');
    }
  }

  private assertFirstIterationFeatures(): void {
    if (this.buildConfig.obfuscationOptions !== undefined) {
      this.throwUnsupported('Unsupported obfuscation config.', 'unsupported-feature-obfuscation');
    }
    if (this.buildConfig.enableDeclgenEts2Ts === true) {
      this.throwUnsupported('Unsupported DeclgenV1 requirement.', 'unsupported-feature-declgen-v1');
    }
  }

  private hasEnabledDispatchField(rawConfig: Record<string, Object>, key: string): boolean {
    if (!Object.hasOwn(rawConfig, key)) {
      return false;
    }
    let value: Object = rawConfig[key];
    if (value === undefined || value === null) {
      return false;
    }
    if (value instanceof Boolean) {
      return value.valueOf();
    }
    return value.toString().length > 0 && value.toString() !== 'false';
  }

  private throwUnsupported(description: string, reason: string, moreInfo: string = ''): void {
    throw new DriverError(LogDataFactory.newDetailedInstance(
      ErrorCode.BUILDSYSTEM_UNSUPPORTED_FEATURE,
      description,
      reason,
      '',
      ['Use sequential ArkTS driver mode without first-iteration unsupported features.'],
      moreInfo
    ));
  }
}
