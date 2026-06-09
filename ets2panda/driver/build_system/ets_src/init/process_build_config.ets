import { BuildConfig } from '../types';

export function initBuildConfig(buildConfigPath: string): BuildConfig {
  return { packageName: buildConfigPath };
}
