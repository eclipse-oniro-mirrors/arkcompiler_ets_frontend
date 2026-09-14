/**
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

#ifndef ES2PANDA_DECLGEN_IGNORED_LIST_H
#define ES2PANDA_DECLGEN_IGNORED_LIST_H

#include <string>
#include <unordered_set>

namespace ark::es2panda::declgen_ets2ts {

inline const std::unordered_set<std::string> DECLGEN_IGNORED_MODULES = {"@ohos.PiPWindow",
                                                                        "@ohos.app.ability.abilityDelegatorRegistry",
                                                                        "@ohos.data.preferences",
                                                                        "@ohos.file.AlbumPickerComponent",
                                                                        "@ohos.file.PhotoPickerComponent",
                                                                        "@ohos.file.RecentPhotoComponent",
                                                                        "@ohos.hilog",
                                                                        "@ohos.i18n",
                                                                        "@ohos.multimedia.MovingPhotoView",
                                                                        "@ohos.nearlink.remoteDevice",
                                                                        "@ohos.notificationManager",
                                                                        "@ohos.pluginComponent",
                                                                        "@ohos.promptAction",
                                                                        "@ohos.util.List",
                                                                        "@ohos.util",
                                                                        "@ohos.web.webview",
                                                                        "graphics3d.Scene"};

}  // namespace ark::es2panda::declgen_ets2ts

#endif  // ES2PANDA_DECLGEN_IGNORED_LIST_H
