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

export declare interface CustomPopupOptions {
    maskColor?: Color | string | Resource | number;
}

export declare interface DragEvent {
    getX(): number;
    getY(): number;
}
declare class CommonMethod<T> {
    gridSpan(value: number): T;
    gridOffset(value: number): T;
}
export declare interface LayoutInfo {}
export declare interface LayoutChild {}
export declare interface LayoutBorderInfo {}
export declare interface TransitionOptions {}