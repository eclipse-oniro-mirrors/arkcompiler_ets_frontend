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

declare enum DataOperationType {

    ADD = 'add',

    DELETE = 'delete',

    EXCHANGE = 'exchange',

    MOVE = 'move',

    CHANGE = 'change',

    RELOAD = 'reload'
}

interface DataAddOperation {

    type: DataOperationType.ADD;

    index: number;

    count?: number;

    key?: string | Array<string>;
}

interface DataDeleteOperation {

    type: DataOperationType.DELETE;

    index: number;

    count?: number;
}

interface DataChangeOperation {

    type: DataOperationType.CHANGE;

    index: number;

    key?: string;
}

interface MoveIndex {

    from: number;

    to: number;
}

interface ExchangeIndex {

    start: number;

    end: number;
}

interface ExchangeKey {

    start: string;

    end: string;
}

interface DataMoveOperation {

    type: DataOperationType.MOVE;

    index: MoveIndex;

    key?: string;
}

interface DataExchangeOperation {

    type: DataOperationType.EXCHANGE;

    index: ExchangeIndex;

    key?: ExchangeKey;
}

interface DataReloadOperation {

    type: DataOperationType.RELOAD;
}

declare type DataOperation = DataAddOperation | DataDeleteOperation | DataChangeOperation | DataMoveOperation | DataExchangeOperation | DataReloadOperation;

declare interface DataChangeListener {

    onDataReloaded(): void;

    onDataAdded(index: number): void;

    onDataAdd(index: number): void;

    onDataMoved(from: number, to: number): void;

    onDataMove(from: number, to: number): void;

    onDataDeleted(index: number): void;

    onDataDelete(index: number): void;

    onDataChanged(index: number): void;

    onDataChange(index: number): void;

    onDatasetChange(dataOperations: DataOperation[]): void;
}

declare interface IDataSource {

    totalCount(): number;

    getData(index: number): any;

    registerDataChangeListener(listener: DataChangeListener): void;

    unregisterDataChangeListener(listener: DataChangeListener): void;
}

declare class LazyForEachAttribute extends DynamicNode<LazyForEachAttribute> {
}

interface LazyForEachInterface {

    (dataSource: IDataSource, itemGenerator: (item: any, index: number) => void, keyGenerator?: (item: any, index: number) => string): LazyForEachAttribute;
}

declare const LazyForEach: LazyForEachInterface;
