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

export declare const NavRouterInstance: NavRouterAttribute;
export declare const NavRouter: NavRouterInterface;

export declare class NavRouterAttribute extends CommonMethod<NavRouterAttribute> {
    mode(mode: NavRouteMode): NavRouterAttribute;
    onStateChange(callback: (isActivated: boolean) => void): NavRouterAttribute;
}

export declare enum NavRouteMode{
    PUSH_WITH_RECREATE,
    PUSH,
    REPLACE
}

export declare interface NavRouterInterface {
    (): NavRouterAttribute
   (value: RouteInfo): NavRouterAttribute;
}

export declare interface RouteInfo {
    name: string;
    param?: unknown;
}