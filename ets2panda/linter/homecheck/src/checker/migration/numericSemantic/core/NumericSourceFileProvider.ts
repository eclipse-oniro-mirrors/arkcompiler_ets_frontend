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
    ArkField,
    FileSignature,
    Stmt,
} from 'arkanalyzer/lib';
import {
    ArkFile,
    AstTreeUtils,
    ts,
} from 'arkanalyzer';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'NumericSourceFileProvider');

export class NumericSourceFileProvider {
    private sourceFiles: Map<FileSignature, ts.SourceFile> = new Map<FileSignature, ts.SourceFile>();

    public clear(): void {
        this.sourceFiles = new Map<FileSignature, ts.SourceFile>();
    }

    public getSourceFile(field?: ArkField, issueStmt?: Stmt): ts.SourceFile | null {
        const arkFile = this.getArkFile(field, issueStmt);
        if (!arkFile) {
            return null;
        }
        let sourceFile = this.sourceFiles.get(arkFile.getFileSignature());
        if (!sourceFile) {
            sourceFile = AstTreeUtils.getASTNode(arkFile.getName(), arkFile.getCode());
            this.sourceFiles.set(arkFile.getFileSignature(), sourceFile);
        }
        return sourceFile;
    }

    private getArkFile(field?: ArkField, issueStmt?: Stmt): ArkFile | null {
        if (field) {
            return field.getDeclaringArkClass().getDeclaringArkFile();
        }
        if (issueStmt) {
            return issueStmt.getCfg().getDeclaringMethod().getDeclaringArkFile();
        }
        logger.error('Missing both issue stmt and field when generating auto fix info.');
        return null;
    }
}
