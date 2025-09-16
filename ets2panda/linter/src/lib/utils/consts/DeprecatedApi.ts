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

export const propertyAccessReplacements = new Map<string, string>([
  ['TextPickerDialog.show', 'showTextPickerDialog'],
  ['DatePickerDialog.show', 'showDatePickerDialog'],
  ['ActionSheet.show', 'showActionSheet'],
  ['AlertDialog.show', 'showAlertDialog'],
  ['componentSnapshot.createFromBuilder', 'getComponentSnapshot().createFromBuilder'],
  ['componentSnapshot.get', 'getComponentSnapshot().get'],
  ['MeasureText.measureTextSize', 'createMeasureText.measureTextSize'],
  ['MeasureText.measureText', 'createMeasureText.measureText'],
  ['dragController.getDragPreview', 'createDragController.getDragPreview'],
  ['dragController.createDragAction', 'createDragController.createDragAction'],
  ['dragController.executeDrag', 'createDragController.executeDrag'],
  ['LocalStorage.getShared', 'getSharedLocalStorage'],
  ['inspector.createComponentObserver', 'createInspector.createComponentObserver'],
  ['Animator.create', 'createAnimator'],
  ['mediaquery.matchMediaSync', 'createMediaQuery.matchMediaSync'],
  ['componentUtils.getRectangleById', 'getComponentUtils().getRectangleById'],
  ['promptAction.showToast', 'getPromptAction.showToast'],
  ['promptAction.showDialog', 'getPromptAction.showDialog'],
  ['promptAction.openCustomDialog', 'getPromptAction.openCustomDialog'],
  ['promptAction.closeCustomDialog', 'getPromptAction.closeCustomDialog'],
  ['promptAction.showActionMenu', 'getPromptAction.showActionMenu'],
  ['TimePickerDialog.show', 'showTimePickerDialog'],
  ['router.pushUrl', 'getRouter.pushUrl'],
  ['router.replaceUrl', 'getRouter.replaceUrl'],
  ['router.back', 'getRouter.back'],
  ['router.clear', 'getRouter.clear'],
  ['router.getLength', 'getRouter.getLength'],
  ['router.getState', 'getRouter.getState'],
  ['router.getStateByIndex', 'getRouter.getStateByIndex'],
  ['router.getStateByUrl', 'getRouter.getStateByUrl'],
  ['router.showAlertBeforeBackPage', 'getRouter.showAlertBeforeBackPage'],
  ['router.hideAlertBeforeBackPage', 'getRouter.hideAlertBeforeBackPage'],
  ['router.getParams', 'getRouter.getParams'],
  ['router.pushNamedRoute', 'getRouter.pushNamedRoute'],
  ['router.replaceNamedRoute', 'getRouter.replaceNamedRoute'],
  ['font.registerFont', 'createFont.registerFont'],
  ['font.getSystemFontList', 'createFont.getSystemFontList'],
  ['font.getFontByName', 'createFont.getFontByName']
]);

export const identifierReplacements = new Map<string, string>([
  ['px2lpx', 'px2lpx'],
  ['lpx2px', 'lpx2px'],
  ['px2fp', 'px2fp'],
  ['fp2px', 'fp2px'],
  ['px2vp', 'px2vp'],
  ['vp2px', 'vp2px'],
  ['getContext', 'getHostContext'],
  ['animateTo', 'animateTo']
]);
