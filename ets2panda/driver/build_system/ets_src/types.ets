import { EtsNativePointer, nullptr } from '@arkts-bindings';

export interface BuildConfig {
  packageName?: string;
}

export const BUILD_CONFIG_NATIVE_HANDLE: EtsNativePointer = nullptr;
