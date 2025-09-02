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
declare namespace prompt {

    interface ShowToastOptions {
        message: string;
        duration?: number;
        bottom?: string | number;
    }
    interface Button {
        text: string;
        color: string;
    }
    interface ShowDialogSuccessResponse {
        index: number;
    }
    interface ShowDialogOptions {
        title?: string;
        message?: string;
        buttons?: [
            Button,
            Button?,
            Button?
        ];
    }
    interface ActionMenuSuccessResponse {
        index: number;
    }
    interface ActionMenuOptions {
        title?: string;
        buttons: [
            Button,
            Button?,
            Button?,
            Button?,
            Button?,
            Button?
        ];
    }
    function showToast(options: ShowToastOptions): void;
    function showDialog(options: ShowDialogOptions, callback: AsyncCallback<ShowDialogSuccessResponse>): void;
    function showDialog(options: ShowDialogOptions): Promise<ShowDialogSuccessResponse>;
    function showActionMenu(options: ActionMenuOptions, callback: AsyncCallback<ActionMenuSuccessResponse>): void;
    function showActionMenu(options: ActionMenuOptions): Promise<ActionMenuSuccessResponse>;
}
export default prompt;
