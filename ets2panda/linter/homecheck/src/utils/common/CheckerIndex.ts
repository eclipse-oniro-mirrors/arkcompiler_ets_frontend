/*
 * Copyright (c) 2024 - 2025 Huawei Device Co., Ltd.
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

import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import { ObservedDecoratorCheck } from '../../checker/migration/ObservedDecoratorCheck';
import { ThisBindCheck } from '../../checker/migration/ThisBindCheck';
import { ObjectLiteralCheck } from '../../checker/migration/ObjectLiteralCheck';
import { AppStorageGetCheck } from '../../checker/migration/AppStorageGetCheck';
import { ModifyStateVarCheck } from '../../checker/migration/ModifyStateVarCheck';
import { NoMethodOverridingFieldCheck } from '../../checker/migration/NoMethodOverridingFieldCheck';
import { CustomBuilderCheck } from '../../checker/migration/CustomBuilderCheck';
import { InteropBackwardDFACheck } from '../../checker/migration/InteropBackwardDFACheck';
import { InteropBoxedTypeCheck } from '../../checker/migration/InteropBoxedTypeCheck';
import { InteropObjectLiteralCheck } from '../../checker/migration/InteropDynamicObjectLiteralsCheck';
import { InteropAssignCheck } from '../../checker/migration/InteropAssignCheck';
import { InteropJSModifyPropertyCheck } from '../../checker/migration/InteropJSModifyPropertyCheck';
import { NoTSLikeAsCheck } from '../../checker/migration/NoTSLikeAsCheck';
import { InteropS2DObjectLiteralCheck } from '../../checker/migration/InteropS2DObjectLiteralsCheck';
import { InteropDeprecatedBuiltInAPICheck } from '../../checker/migration/InteropDeprecatedBuiltInAPICheck';
import { NumericSemanticCheck } from '../../checker/migration/NumericSemanticCheck';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'CheckerIndex');

export const fileRules = {
    '@migration/arkts-instance-method-bind-this': ThisBindCheck,
    '@migration/arkui-data-observation': ObservedDecoratorCheck,
    '@migration/arkui-stateful-appstorage': AppStorageGetCheck,
    '@migration/arkui-no-update-in-build': ModifyStateVarCheck,
    '@migration/arkui-custombuilder-passing': CustomBuilderCheck,
    '@migration/no-method-overriding-field-check': NoMethodOverridingFieldCheck,
    '@migration/interop-boxed-type-check': InteropBoxedTypeCheck,
    '@migration/arkts-interop-s2d-object-literal': InteropS2DObjectLiteralCheck,
};

export const projectRules = {
    '@migration/arkts-obj-literal-generate-class-instance': ObjectLiteralCheck,
    '@migration/interop-backward-dfa': InteropBackwardDFACheck,
    '@migration/interop-assign': InteropAssignCheck,
    '@migration/interop-js-modify-property': InteropJSModifyPropertyCheck,
    '@migration/interop-dynamic-object-literals': InteropObjectLiteralCheck,
    '@migration/arkts-no-ts-like-as': NoTSLikeAsCheck,
    '@migration/arkts-interop-s2d-dynamic-call-builtin-api-not-in-static': InteropDeprecatedBuiltInAPICheck,
    '@migration/arkts-numeric-semantic': NumericSemanticCheck,
};

// 新增文件级的checker，需要在此处注册
export const file2CheckRuleMap: Map<string, any> = new Map(Object.entries(fileRules));
// 新增项目级checker，需要在此处注册
export const project2CheckRuleMap: Map<string, any> = new Map(Object.entries(projectRules));

export class ProxyChecker {
    static getClass(ruleId: string): any {
        const checker = file2CheckRuleMap.get(ruleId) ?? project2CheckRuleMap.get(ruleId);
        if (!checker) {
            logger.error(`${ruleId} is not matched to any checker`);
            return null;
        }
        return new checker();
    }
}
