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

import { ArkClass, ArkFile } from 'arkanalyzer';
import { isMatchedFile, isMatchedNamespace, isMatchedClass, FieldMatcher, isMatchedField } from '../Matchers';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'matchFields');

export function matchFields(arkFiles: ArkFile[], matcher: FieldMatcher, callback: Function): void {
    for (let arkFile of arkFiles) {
        if (matcher.file && !isMatchedFile(arkFile, matcher.file)) {
            continue;
        }
        for (const ns of arkFile.getAllNamespacesUnderThisFile()) {
            if (matcher.namespace && !isMatchedNamespace(ns, matcher.namespace)) {
                continue;
            }
            ns.getClasses().forEach(cls => matchFieldsInClass(matcher, cls, callback));
        }
        arkFile.getClasses().forEach(cls => matchFieldsInClass(matcher, cls, callback));
    }
}

function matchFieldsInClass(matcher: FieldMatcher, arkClass: ArkClass, callback: Function): void {
    if (matcher.class && !isMatchedClass(arkClass, matcher.class)) {
        return;
    }
    for (const arkField of arkClass.getFields()) {
        if (isMatchedField(arkField, [matcher])) {
            try {
                callback(arkField);
            } catch (error) {
                logger.error('Error in field callback: ', error);
            }
        }
    }
}
