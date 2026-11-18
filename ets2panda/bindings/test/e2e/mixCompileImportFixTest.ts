/*
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

import fs from 'fs';
import { getMultiModuleLsp, getRealPath } from '../utils';

const projectName = 'mixCompileImportFix';

describe('mixCompileImportFixTest', () => {
    const modules = [
        { name: 'entry', moduleType: 'har', srcPath: 'entry' },
        { name: 'library', moduleType: 'har', srcPath: 'library' }
    ];
    let lsp: any;

    const getLspInstance = (): any => {
        if (lsp === undefined) {
            lsp = getMultiModuleLsp(projectName, modules, []);
            lsp.initAstCache();
        }
        return lsp;
    };

    test('import fix offers both the module alias and the file-level form', () => {
        const file = getRealPath(projectName, 'entry/src/main/ets/pages/Index.ets');
        const source = fs.readFileSync(file, 'utf8');
        const offset = source.indexOf('foo');
        const res = getLspInstance().getCodeFixesAtPosition(file, offset, offset + 'foo'.length, [1005]);
        expect(res).toBeDefined();
        const actions = res ?? [];
        const descriptions = actions.map((action: any) => String(action.description.valueOf()));
        // eslint-disable-next-line no-console
        console.log('codefix actions:', descriptions);
        expect(descriptions).toContain("Add import {foo} from 'library'");
        expect(descriptions).toContain("Add import {foo} from 'library/ets/components/Calc'");
    });
});
