/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import sdkOverloadJson from '../../data/SdkOverload.json';
import { createOverloadMapKey } from './OverloadBlacklist';

type ApiFuncArg = {
  name: string;
  type: string;
  is_optional: boolean;
  has_default: boolean;
};

export type OverloadInfo = {
  replacement: string;
  args: ApiFuncArg[];
};

export type OverloadApiFixMap = Map<string, OverloadInfo[]>;

export function initOverloadApiFixMap(): OverloadApiFixMap {
  const fixMap: OverloadApiFixMap = new Map();
  for (const entry of sdkOverloadJson.api_list) {
    const apiInfo = entry.api_info;
    const parentName = apiInfo.parent_api?.[0]?.api_name ?? '';
    const key = createOverloadMapKey(apiInfo.api_name, parentName, entry.file_path);
    const info: OverloadInfo = {
      replacement: apiInfo.api_fixed_name,
      args: apiInfo.api_func_args || []
    };
    const list = fixMap.get(key) ?? [];
    list.push(info);
    fixMap.set(key, list);
  }
  return fixMap;
}

export const COMMON_OVERLOAD_METHODS = ['on', 'off', 'once'];
export const COMMON_OVERLOAD_METHOD_PARAMETERS = ['type', 'event', 'eventType', 'evt'];
export const LIST_OVERLOAD_METHOD_PARAMETERS = ['type', 'nodeType'];
export const SDK_FILE_EXTENSIONS = ['d.ts', 'd.ets'];
export const GLOBAL_KEYWORD = 'global';
export const ON_KEY_EVENT = 'onKeyEvent';
export const LIST_OVERLOAD_METHODS: Set<string> = new Set([
  'on',
  'off',
  'once',
  'bindController',
  'copyDir',
  'createImageLattice',
  'createNode',
  'deleteAssets',
  'findElement',
  'getAttribute',
  'getEvent',
  'moveDir',
  'onKeyEvent'
]);

export const COMMON_OVERLOAD_METHOD_FILES: Set<string> = new Set([
  '@hms.ai.AgentFramework.d.ets',
  '@hms.ai.intelligentKws.d.ts',
  '@hms.ai.textReader.d.ets',
  '@hms.ai.textToSpeech.d.ts',
  '@hms.ai.vision.visionBase.d.ts',
  '@hms.ai.visionImageAnalyzer.d.ets',
  '@hms.bluetooth.hearingAid.d.ts',
  '@hms.carService.smartMobilityCommon.d.ts',
  '@hms.collaboration.awareness.d.ets',
  '@hms.collaboration.cameraSupplier.d.ts',
  '@hms.collaboration.collaborationAbility.d.ts',
  '@hms.collaboration.collaborationServiceManager.d.ts',
  '@hms.collaboration.harmonyShare.d.ts',
  '@hms.collaboration.inner.serviceDelivery.d.ts',
  '@hms.collaboration.networksharing.d.ts',
  '@hms.collaboration.serviceBrowser.d.ts',
  '@hms.collaboration.systemShare.d.ts',
  '@hms.core.appgalleryservice.moduleInstallManager.d.ts',
  '@hms.core.appgalleryservice.updateManager.d.ts',
  '@hms.core.gameservice.gamenearbytransfer.d.ts',
  '@hms.core.gameservice.gameperformance.d.ts',
  '@hms.core.gameservice.gameplayer.d.ts',
  '@hms.core.hiAnalytics.d.ts',
  '@hms.core.map.map.d.ts',
  '@hms.core.readerservice.readerComponent.d.ets',
  '@hms.core.scan.customScan.d.ts',
  '@hms.fusionConnectivityExt.d.ts',
  '@hms.gameAcceleration.assetDownloadManager.d.ts',
  '@hms.health.store.d.ts',
  '@hms.health.wearEngine.d.ts',
  '@hms.iot.webserver.d.ts',
  '@hms.nearlink.advertising.d.ts',
  '@hms.nearlink.dataTransfer.d.ts',
  '@hms.nearlink.manager.d.ts',
  '@hms.nearlink.scan.d.ts',
  '@hms.networkboost.handover.d.ts',
  '@hms.networkboost.netquality.d.ts',
  '@hms.officeservice.stylusInteraction.d.ts',
  '@hms.pcService.fileGuard.d.ts',
  '@hms.pcService.openFileBoost.d.ts',
  '@hms.pcService.recoveryKeyService.d.ts',
  '@hms.pcService.statusBarManager.d.ts',
  '@hms.resourceschedule.animationPolicy.d.ts',
  '@hms.security.appLock.d.ts',
  '@hms.security.dlpAntiPeep.d.ts',
  '@hms.security.securityAudit.d.ts',
  '@hms.security.superPrivacyManager.d.ts',
  '@hms.system.update.d.ts',
  '@hms.telephony.voipCall.d.ts',
  '@hms.xrGlassesService.xrGlassesAppService.d.ts',
  'RigidBody.d.ts',
  '@ohos.abilityAccessCtrl.d.ts',
  '@ohos.accessibility.config.d.ts',
  '@ohos.accessibility.d.ts',
  '@ohos.account.appAccount.d.ts',
  '@ohos.account.osAccount.d.ts',
  '@ohos.ai.intelligentVoice.d.ts',
  '@ohos.app.ability.abilityManager.d.ts',
  '@ohos.app.ability.autoStartupManager.d.ts',
  '@ohos.app.ability.continueManager.d.ts',
  '@ohos.app.ability.errorManager.d.ts',
  '@ohos.app.ability.missionManager.d.ts',
  '@ohos.app.ability.UIAbility.d.ts',
  '@ohos.app.form.formHost.d.ts',
  '@ohos.app.form.formObserver.d.ts',
  '@ohos.application.formHost.d.ts',
  '@ohos.arkui.dragController.d.ts',
  '@ohos.arkui.inspector.d.ts',
  '@ohos.arkui.observer.d.ts',
  '@ohos.arkui.UIContext.d.ts',
  '@ohos.arkui.uiExtension.d.ts',
  '@ohos.bluetooth.access.d.ts',
  '@ohos.bluetooth.baseProfile.d.ts',
  '@ohos.bluetooth.ble.d.ts',
  '@ohos.bluetooth.connection.d.ts',
  '@ohos.bluetooth.opp.d.ts',
  '@ohos.bluetooth.socket.d.ts',
  '@ohos.connectedTag.d.ts',
  '@ohos.continuation.continuationManager.d.ts',
  '@ohos.cooperate.d.ts',
  '@ohos.data.dataShare.d.ts',
  '@ohos.data.distributedDataObject.d.ts',
  '@ohos.data.distributedKVStore.d.ts',
  '@ohos.data.preferences.d.ts',
  '@ohos.data.relationalStore.d.ts',
  '@ohos.data.sendablePreferences.d.ets',
  '@ohos.display.d.ts',
  '@ohos.distributedDeviceManager.d.ts',
  '@ohos.distributedHardware.mechanicManager.d.ts',
  '@ohos.distributedMissionManager.d.ts',
  '@ohos.distributedsched.abilityConnectionManager.d.ts',
  '@ohos.distributedsched.linkEnhance.d.ts',
  '@ohos.distributedsched.proxyChannelManager.d.ts',
  '@ohos.dlpPermission.d.ts',
  '@ohos.file.cloudSync.d.ts',
  '@ohos.file.photoAccessHelper.d.ts',
  '@ohos.filemanagement.userFileManager.d.ts',
  '@ohos.geoLocationManager.d.ts',
  '@ohos.graphics.displaySync.d.ts',
  '@ohos.inputMethod.d.ts',
  '@ohos.inputMethodEngine.d.ts',
  '@ohos.mediaquery.d.ts',
  '@ohos.multimedia.audio.d.ts',
  '@ohos.multimedia.audioHaptic.d.ts',
  '@ohos.multimedia.avsession.d.ts',
  '@ohos.multimedia.camera.d.ts'
]);
