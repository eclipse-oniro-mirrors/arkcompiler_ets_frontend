import { BuildConfig } from '../types';
import { BaseMode } from './base_mode';

export class BuildMode extends BaseMode {
  private buildConfig: BuildConfig;

  public constructor(buildConfig: BuildConfig) {
    super();
    this.buildConfig = buildConfig;
  }

  public run(): void {
    super.run();
  }
}
