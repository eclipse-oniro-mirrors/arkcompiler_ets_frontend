/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

declare enum Sticky {

    None,

    Normal,

    Opacity
}

declare enum EditMode {

    None,

    Deletable,

    Movable
}

declare enum SwipeEdgeEffect {

    Spring,

    None
}

declare enum SwipeActionState {

    COLLAPSED,

    EXPANDED,

    ACTIONING
}

declare interface SwipeActionItem {

    builder?: CustomBuilder;

    actionAreaDistance?: Length;

    onAction?: () => void;

    onEnterActionArea?: () => void;

    onExitActionArea?: () => void;

    onStateChange?: (state: SwipeActionState) => void;
}

declare interface SwipeActionOptions {

    start?: CustomBuilder | SwipeActionItem;

    end?: CustomBuilder | SwipeActionItem;

    edgeEffect?: SwipeEdgeEffect;

    onOffsetChange?: (offset: number) => void;
}

declare enum ListItemStyle {

    NONE = 0,

    CARD = 1
}

declare interface ListItemOptions {

    style?: ListItemStyle;
}

interface ListItemInterface {

    (value?: ListItemOptions): ListItemAttribute;

    (value?: string): ListItemAttribute;
}

declare class ListItemAttribute extends CommonMethod<ListItemAttribute> {

    sticky(value: Sticky): ListItemAttribute;

    editable(value: boolean | EditMode): ListItemAttribute;

    selectable(value: boolean): ListItemAttribute;

    selected(value: boolean): ListItemAttribute;

    swipeAction(value: SwipeActionOptions): ListItemAttribute;

    onSelect(event: (isSelected: boolean) => void): ListItemAttribute;
}

declare const ListItemInstance: ListItemAttribute;

declare const ListItem: ListItemInterface;
