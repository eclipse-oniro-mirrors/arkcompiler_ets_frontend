import { BuildConfig } from '../types';
import { BaseMode } from './base_mode';

export class BuildMode extends BaseMode {
  public constructor(buildConfig: BuildConfig) {
    super(buildConfig);
  }

  public run(): void {
    super.run();
  }
}
