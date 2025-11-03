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

const TopLevelSym: string = "TopLevelSym";
const InnerSym: string = "InnerSym";

const _topLevelFunc = (x: number | undefined): number => {
    if (x === undefined) {
        return 12;
    }
    return x;
};

const _innerFunc = (arg: { x: number } | undefined): number => {
    if (arg === undefined) {
        return 12;
    }
    return arg.x;
};

type NumberFunc = () => number;

// Indexable type for items
type IndexableType = Record<string, NumberFunc>;

// Explicitly typed indexable object
const _items: IndexableType = {};
const innerFunction: NumberFunc = () => {
    return _innerFunc({ x: 12 });
};

// Fix: Assign computed property safely (done after object creation)
_items[InnerSym] = innerFunction;

const topLevelWrapper: NumberFunc = () => {
    return _topLevelFunc(12);
};

// Inner map using the same indexable type
const innerMap: IndexableType = {};
const innerMapFunction: NumberFunc = () => {
    const result = _innerFunc({ x: 12 });
    return result;
};

innerMap[InnerSym] = innerMapFunction;

// Define the exported structure explicitly
type ExportedType = Record<string, NumberFunc | IndexableType> & {
    items: IndexableType;
};

// Create a base object without computed properties
class ExportedStructure implements ExportedType {
    items: IndexableType;

    constructor() {
        this.items = innerMap;
    }

    [key: string]: NumberFunc | IndexableType;

    static createBase(): ExportedType {
        return new ExportedStructure() as ExportedType;
    }
}

// Create the exported structure and assign dynamic keys safely
const baseExportedStructure: ExportedType = ExportedStructure.createBase();
const _exportedStructure: ExportedType = { ...baseExportedStructure };

// Assign computed key after object creation (not in literal)
_exportedStructure[TopLevelSym] = topLevelWrapper;

const _exported: ExportedType = _exportedStructure;

export default _exported;

/* @@? 26:26 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 61:64 Error SyntaxError: Unexpected token '&'. */
/* @@? 61:66 Error SyntaxError: Unexpected token '{'. */
/* @@? 62:12 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 62:12 Error TypeError: Type name 'IndexableType' used in the wrong context */
/* @@? 66:36 Error TypeError: Interfaces cannot extend classes, only other interfaces. */
/* @@? 73:6 Error TypeError: Indexed signatures are not allowed. Use arrays instead! */
/* @@? 76:16 Error TypeError: Cannot cast type 'ExportedStructure' to 'Record<String,((() => Double))|Record<String,(() => Double)>>' */