'use static'
/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
import es from '@ohos.lang.interop';

export declare const es2st_array: es.Array<number>;
export declare const es2st_arrayOfArray: es.Array<es.Array<string>>;
export declare const es2st_arrayOfBoolean: es.Array<boolean>;
export declare const es2st_arrayOfObject: es.Array<Object>;

export class ArrayPropertyClass {
    public numbers: es.Array<number>;
    public strings: es.Array<string>;
    private privateArray: es.Array<boolean>;
    protected protectedArray: es.Array<Object>;
    readonly readonlyArray: es.Array<number>;

    constructor(nums: es.Array<number>) {
        this.numbers = nums;
        this.strings = new es.Array<string>();
        this.privateArray = new es.Array<boolean>();
        this.protectedArray = new es.Array<Object>();
        this.readonlyArray = nums;
    }
}

export class ArrayMethodClass {
    public getNumbers(): es.Array<number> {
        return new es.Array<number>();
    }

    public setStrings(arr: es.Array<string>): void {
    }

    public transformArray(input: es.Array<number>): es.Array<string> {
        return new es.Array<string>();
    }

    public mergeArrays(arr1: es.Array<number>, arr2: es.Array<number>): es.Array<number> {
        return new es.Array<number>();
    }

    public static createArray(): es.Array<number> {
        return new es.Array<number>();
    }
}

export declare const es2st_map: es.Map<string, number>;
export declare const es2st_mapOfMap: es.Map<string, es.Map<string, number>>;
export declare const es2st_mapWithArrayValue: es.Map<string, es.Array<number>>;

export class MapPropertyClass {
    public stringToNumber: es.Map<string, number>;
    public numberToString: es.Map<number, string>;
    private privateMap: es.Map<string, boolean>;
    protected protectedMap: es.Map<string, Object>;
    readonly readonlyMap: es.Map<string, number>;

    constructor(map: es.Map<string, number>) {
        this.stringToNumber = map;
        this.numberToString = new es.Map<number, string>();
        this.privateMap = new es.Map<string, boolean>();
        this.protectedMap = new es.Map<string, Object>();
        this.readonlyMap = map;
    }
}

export class MapMethodClass {
    public getMap(): es.Map<string, number> {
        return new es.Map<string, number>();
    }

    public setMap(map: es.Map<string, string>): void {
        
    }

    public transformMap(input: es.Map<string, number>): es.Map<number, string> {
        return new es.Map<number, string>();
    }

    public mergeMaps(map1: es.Map<string, number>, map2: es.Map<string, number>): es.Map<string, number> {
        return new es.Map<string, number>();
    }

    public static createMap(): es.Map<string, number> {
        return new es.Map<string, number>();
    }
}

export declare const es2st_set: es.Set<number>;
export declare const es2st_setOfString: es.Set<string>;
export declare const es2st_setOfObject: es.Set<Object>;

export class SetPropertyClass {
    public numberSet: es.Set<number>;
    public stringSet: es.Set<string>;
    private privateSet: es.Set<boolean>;
    protected protectedSet: es.Set<Object>;
    readonly readonlySet: es.Set<number>;

    constructor(set: es.Set<number>) {
        this.numberSet = set;
        this.stringSet = new es.Set<string>();
        this.privateSet = new es.Set<boolean>();
        this.protectedSet = new es.Set<Object>();
        this.readonlySet = set;
    }
}

export class SetMethodClass {
    public getSet(): es.Set<number> {
        return new es.Set<number>();
    }

    public setSet(set: es.Set<string>): void {
        
    }

    public transformSet(input: es.Set<number>): es.Set<string> {
        return new es.Set<string>();
    }

    public unionSets(set1: es.Set<number>, set2: es.Set<number>): es.Set<number> {
        return new es.Set<number>();
    }

    public static createSet(): es.Set<number> {
        return new es.Set<number>();
    }
}

export class MixedEsTypesClass {
    public array: es.Array<number>;
    public map: es.Map<string, number>;
    public set: es.Set<string>;

    constructor() {
        this.array = new es.Array<number>();
        this.map = new es.Map<string, number>();
        this.set = new es.Set<string>();
    }

    public arrayToMap(arr: es.Array<string>): es.Map<number, string> {
        return new es.Map<number, string>();
    }

    public mapToSet(map: es.Map<string, number>): es.Set<number> {
        return new es.Set<number>();
    }

    public setToArray(set: es.Set<string>): es.Array<string> {
        return new es.Array<string>();
    }

    public getNestedStructure(): es.Map<string, es.Array<es.Set<number>>> {
        return new es.Map<string, es.Array<es.Set<number>>>();
    }
}

export declare const st_array: Array<number>;
export declare const st_arrayOfArray: Array<Array<string>>;
export declare const st_arrayOfBoolean: Array<boolean>;
export declare const st_arrayOfObject: Array<Object>;

export class StArrayPropertyClass {
    public numbers: Array<number>;
    public strings: Array<string>;
    private privateArray: Array<boolean>;
    protected protectedArray: Array<Object>;
    readonly readonlyArray: Array<number>;

    constructor(nums: Array<number>) {
        this.numbers = nums;
        this.strings = new Array<string>();
        this.privateArray = new Array<boolean>();
        this.protectedArray = new Array<Object>();
        this.readonlyArray = nums;
    }
}

export class StArrayMethodClass {
    public getNumbers(): Array<number> {
        return new Array<number>();
    }

    public setStrings(arr: Array<string>): void {
    }

    public transformArray(input: Array<number>): Array<string> {
        return new Array<string>();
    }

    public mergeArrays(arr1: Array<number>, arr2: Array<number>): Array<number> {
        return new Array<number>();
    }

    public static createArray(): Array<number> {
        return new Array<number>();
    }
}

export declare const st_map: Map<string, number>;
export declare const st_mapOfMap: Map<string, Map<string, number>>;
export declare const st_mapWithArrayValue: Map<string, Array<number>>;

export class StMapPropertyClass {
    public stringToNumber: Map<string, number>;
    public numberToString: Map<number, string>;
    private privateMap: Map<string, boolean>;
    protected protectedMap: Map<string, Object>;
    readonly readonlyMap: Map<string, number>;

    constructor(map: Map<string, number>) {
        this.stringToNumber = map;
        this.numberToString = new Map<number, string>();
        this.privateMap = new Map<string, boolean>();
        this.protectedMap = new Map<string, Object>();
        this.readonlyMap = map;
    }
}

export class StMapMethodClass {
    public getMap(): Map<string, number> {
        return new Map<string, number>();
    }

    public setMap(map: Map<string, string>): void {
    }

    public transformMap(input: Map<string, number>): Map<number, string> {
        return new Map<number, string>();
    }

    public mergeMaps(map1: Map<string, number>, map2: Map<string, number>): Map<string, number> {
        return new Map<string, number>();
    }

    public static createMap(): Map<string, number> {
        return new Map<string, number>();
    }
}

export declare const st_set: Set<number>;
export declare const st_setOfString: Set<string>;
export declare const st_setOfObject: Set<Object>;

export class StSetPropertyClass {
    public numberSet: Set<number>;
    public stringSet: Set<string>;
    private privateSet: Set<boolean>;
    protected protectedSet: Set<Object>;
    readonly readonlySet: Set<number>;

    constructor(set: Set<number>) {
        this.numberSet = set;
        this.stringSet = new Set<string>();
        this.privateSet = new Set<boolean>();
        this.protectedSet = new Set<Object>();
        this.readonlySet = set;
    }
}

export class StSetMethodClass {
    public getSet(): Set<number> {
        return new Set<number>();
    }

    public setSet(set: Set<string>): void {
        
    }

    public transformSet(input: Set<number>): Set<string> {
        return new Set<string>();
    }

    public unionSets(set1: Set<number>, set2: Set<number>): Set<number> {
        return new Set<number>();
    }

    public static createSet(): Set<number> {
        return new Set<number>();
    }
}

export class MixedStTypesClass {
    public array: Array<number>;
    public map: Map<string, number>;
    public set: Set<string>;

    constructor() {
        this.array = new Array<number>();
        this.map = new Map<string, number>();
        this.set = new Set<string>();
    }

    public arrayToMap(arr: Array<string>): Map<number, string> {
        return new Map<number, string>();
    }

    public mapToSet(map: Map<string, number>): Set<number> {
        return new Set<number>();
    }

    public setToArray(set: Set<string>): Array<string> {
        return new Array<string>();
    }

    public getNestedStructure(): Map<string, Array<Set<number>>> {
        return new Map<string, Array<Set<number>>>();
    }
}

export interface StTypesInterface {
    arrayProp: Array<number>;
    mapProp: Map<string, string>;
    setProp: Set<boolean>;

    getArray(): Array<number>;
    setMap(map: Map<string, number>): void;
    processSet(set: Set<string>): Set<string>;
}

export function stProcessArray(arr: Array<number>): Array<string> {
    return new Array<string>();
}

export function stProcessMap(map: Map<string, number>): Map<number, string> {
    return new Map<number, string>();
}

export function stProcessSet(set: Set<string>): Set<number> {
    return new Set<number>();
}

export declare const mixed_esArrayInStMap: Map<string, es.Array<number>>;
export declare const mixed_stArrayInEsMap: es.Map<string, Array<number>>;
export declare const mixed_esSetInStArray: Array<es.Set<string>>;
export declare const mixed_stSetInEsArray: es.Array<Set<string>>;

export class MixedEsStPropertyClass {
    public esArrayOfStMap: es.Array<Map<string, number>>;
    public esMapWithStSet: es.Map<string, Set<number>>;
    public esSetOfStArray: es.Set<Array<string>>;

    public stArrayOfEsMap: Array<es.Map<string, number>>;
    public stMapWithEsSet: Map<string, es.Set<number>>;
    public stSetOfEsArray: Set<es.Array<string>>;

    public deeplyNested: es.Map<string, Array<es.Set<Map<number, string>>>>;

    constructor() {
        this.esArrayOfStMap = new es.Array<Map<string, number>>();
        this.esMapWithStSet = new es.Map<string, Set<number>>();
        this.esSetOfStArray = new es.Set<Array<string>>();
        this.stArrayOfEsMap = new Array<es.Map<string, number>>();
        this.stMapWithEsSet = new Map<string, es.Set<number>>();
        this.stSetOfEsArray = new Set<es.Array<string>>();
        this.deeplyNested = new es.Map<string, Array<es.Set<Map<number, string>>>>();
    }
}

export class MixedEsStMethodClass {
    public esArrayToStArray(input: es.Array<number>): Array<number> {
        return new Array<number>();
    }

    public stArrayToEsArray(input: Array<number>): es.Array<number> {
        return new es.Array<number>();
    }

    public esMapToStMap(input: es.Map<string, number>): Map<string, number> {
        return new Map<string, number>();
    }

    public stMapToEsMap(input: Map<string, number>): es.Map<string, number> {
        return new es.Map<string, number>();
    }

    public esSetToStSet(input: es.Set<string>): Set<string> {
        return new Set<string>();
    }

    public stSetToEsSet(input: Set<string>): es.Set<string> {
        return new es.Set<string>();
    }

    public mixedParams(esArr: es.Array<number>, stArr: Array<string>, esMap: es.Map<string, number>, stSet: Set<boolean>): void {
    }

    public getEsArrayOfStMaps(): es.Array<Map<string, number>> {
        return new es.Array<Map<string, number>>();
    }

    public getStMapOfEsArrays(): Map<string, es.Array<number>> {
        return new Map<string, es.Array<number>>();
    }

    public static createMixedStructure(): es.Map<string, Array<es.Set<number>>> {
        return new es.Map<string, Array<es.Set<number>>>();
    }
}

export interface MixedEsStInterface {
    esArray: es.Array<number>;
    stArray: Array<number>;
    esMapWithStValue: es.Map<string, Array<number>>;
    stMapWithEsValue: Map<string, es.Array<number>>;
    convertEsToSt(input: es.Array<string>): Array<string>;
    convertStToEs(input: Map<string, number>): es.Map<string, number>;
    processMixed(esSet: es.Set<number>, stSet: Set<string>): es.Array<Map<string, number>>;
}


export function convertEsArrayToStArray(input: es.Array<number>): Array<number> {
    return new Array<number>();
}

export function convertStMapToEsMap(input: Map<string, number>): es.Map<string, number> {
    return new es.Map<string, number>();
}

export function processMixedTypes(
    esArr: es.Array<string>,
    stMap: Map<number, boolean>,
    esSet: es.Set<number>
): Array<es.Map<string, Set<number>>> {
    return new Array<es.Map<string, Set<number>>>();
}


export class TypeConverter {
    
    public convertArrayEsToSt<T>(input: es.Array<T>): Array<T> {
        return new Array<T>();
    }

    public convertArrayStToEs<T>(input: Array<T>): es.Array<T> {
        return new es.Array<T>();
    }

    
    public convertMapEsToSt<K, V>(input: es.Map<K, V>): Map<K, V> {
        return new Map<K, V>();
    }

    public convertMapStToEs<K, V>(input: Map<K, V>): es.Map<K, V> {
        return new es.Map<K, V>();
    }

    
    public convertSetEsToSt<T>(input: es.Set<T>): Set<T> {
        return new Set<T>();
    }

    public convertSetStToEs<T>(input: Set<T>): es.Set<T> {
        return new es.Set<T>();
    }
}


export class ComplexNestedStructure {
    
    public esStEs: es.Array<Map<string, es.Set<number>>>;
    
    
    public stEsSt: Array<es.Map<string, Set<number>>>;
    
    
    public deepAlternating: es.Map<string, Array<es.Set<Map<number, es.Array<string>>>>>;

    constructor() {
        this.esStEs = new es.Array<Map<string, es.Set<number>>>();
        this.stEsSt = new Array<es.Map<string, Set<number>>>();
        this.deepAlternating = new es.Map<string, Array<es.Set<Map<number, es.Array<string>>>>>();
    }

    public processComplexStructure(
        input: es.Map<string, Array<es.Set<number>>>
    ): Array<es.Map<number, Set<es.Array<string>>>> {
        return new Array<es.Map<number, Set<es.Array<string>>>>();
    }
}
