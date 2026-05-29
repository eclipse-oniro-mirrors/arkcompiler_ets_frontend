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

import {
    ArrayType,
    ClassType,
    FunctionType,
    TupleType,
    Type,
    UnionType,
} from 'arkanalyzer/lib';
import {
    NumberType,
    UnclearReferenceType,
} from 'arkanalyzer';
import {
    BuiltinNumberChange,
    BuiltinNumberChangePath,
    BuiltinNumberChangePathStep,
    NumberCategory,
} from '../../../core/NumericSemanticTypes';

interface BuiltinTypeChangePathCollectorOptions {
    getIntLongCategoryFromType(type: Type): NumberCategory.int | NumberCategory.long | null;
}

export class BuiltinTypeChangePathCollector {
    constructor(private options: BuiltinTypeChangePathCollectorOptions) {}

    public collect(dynType: Type, staType: Type, path: BuiltinNumberChangePath): BuiltinNumberChange[] {
        const directCategory = this.getDirectNumberChangeCategory(dynType, staType);
        if (directCategory) {
            return [{ path, category: directCategory }];
        }

        const changes: BuiltinNumberChange[] = [];
        this.collectFunctionChanges(dynType, staType, path, changes);
        this.collectArrayChanges(dynType, staType, path, changes);
        this.collectTupleChanges(dynType, staType, path, changes);
        this.collectGenericChanges(dynType, staType, path, changes);
        this.collectUnionChanges(dynType, staType, path, changes);
        return changes;
    }

    private collectFunctionChanges(
        dynType: Type,
        staType: Type,
        path: BuiltinNumberChangePath,
        changes: BuiltinNumberChange[]
    ): void {
        if (!(dynType instanceof FunctionType) || !(staType instanceof FunctionType)) {
            return;
        }
        const dynSignature = dynType.getMethodSignature();
        const staSignature = staType.getMethodSignature();
        const dynParams = dynSignature.getMethodSubSignature().getParameters();
        const staParams = staSignature.getMethodSubSignature().getParameters();
        const paramLength = Math.min(dynParams.length, staParams.length);
        for (let i = 0; i < paramLength; i++) {
            const paramPath = this.appendPath(path, { kind: 'functionParam', index: i });
            changes.push(...this.collect(dynParams[i].getType(), staParams[i].getType(), paramPath));
        }
        const returnPath = this.appendPath(path, { kind: 'functionReturn' });
        changes.push(...this.collect(dynSignature.getType(), staSignature.getType(), returnPath));
    }

    private collectArrayChanges(
        dynType: Type,
        staType: Type,
        path: BuiltinNumberChangePath,
        changes: BuiltinNumberChange[]
    ): void {
        if (!(dynType instanceof ArrayType) || !(staType instanceof ArrayType)) {
            return;
        }
        changes.push(...this.collect(dynType.getBaseType(), staType.getBaseType(), this.appendPath(path, { kind: 'arrayElement' })));
    }

    private collectTupleChanges(
        dynType: Type,
        staType: Type,
        path: BuiltinNumberChangePath,
        changes: BuiltinNumberChange[]
    ): void {
        if (!(dynType instanceof TupleType) || !(staType instanceof TupleType)) {
            return;
        }
        const dynTypes = dynType.getTypes();
        const staTypes = staType.getTypes();
        const length = Math.min(dynTypes.length, staTypes.length);
        for (let i = 0; i < length; i++) {
            const tuplePath = this.appendPath(path, { kind: 'tuple', index: i });
            changes.push(...this.collect(dynTypes[i], staTypes[i], tuplePath));
        }
    }

    private collectGenericChanges(
        dynType: Type,
        staType: Type,
        path: BuiltinNumberChangePath,
        changes: BuiltinNumberChange[]
    ): void {
        const dynTypes = this.getGenericTypes(dynType);
        const staTypes = this.getGenericTypes(staType);
        const length = Math.min(dynTypes.length, staTypes.length);
        for (let i = 0; i < length; i++) {
            const genericPath = this.appendPath(path, { kind: 'generic', index: i });
            changes.push(...this.collect(dynTypes[i], staTypes[i], genericPath));
        }
    }

    private collectUnionChanges(
        dynType: Type,
        staType: Type,
        path: BuiltinNumberChangePath,
        changes: BuiltinNumberChange[]
    ): void {
        if (!(dynType instanceof UnionType) || !(staType instanceof UnionType)) {
            return;
        }
        const dynTypes = dynType.getTypes();
        const staTypes = staType.getTypes();
        const length = Math.min(dynTypes.length, staTypes.length);
        for (let i = 0; i < length; i++) {
            const unionPath = this.appendPath(path, { kind: 'union', index: i });
            changes.push(...this.collect(dynTypes[i], staTypes[i], unionPath));
        }
    }

    private getDirectNumberChangeCategory(dynType: Type, staType: Type): NumberCategory.int | NumberCategory.long | null {
        if (!(dynType instanceof NumberType)) {
            return null;
        }
        return this.options.getIntLongCategoryFromType(staType) ?? this.getSingleIntLongCategoryFromUnionType(staType);
    }

    private getSingleIntLongCategoryFromUnionType(type: Type): NumberCategory.int | NumberCategory.long | null {
        if (!(type instanceof UnionType)) {
            return null;
        }
        const categories = new Set<NumberCategory.int | NumberCategory.long>();
        for (const unionType of type.getTypes()) {
            const category = this.options.getIntLongCategoryFromType(unionType);
            if (category) {
                categories.add(category);
            }
        }
        return categories.size === 1 ? [...categories][0] : null;
    }

    private getGenericTypes(type: Type): Type[] {
        if (type instanceof ClassType) {
            return type.getRealGenericTypes() ?? [];
        }
        if (type instanceof FunctionType) {
            return type.getRealGenericTypes() ?? [];
        }
        if (type instanceof UnclearReferenceType) {
            return type.getGenericTypes();
        }
        return [];
    }

    private appendPath(path: BuiltinNumberChangePath, step: BuiltinNumberChangePathStep): BuiltinNumberChangePath {
        return {
            root: path.root,
            argIndex: path.argIndex,
            steps: [...path.steps, step],
        };
    }
}
