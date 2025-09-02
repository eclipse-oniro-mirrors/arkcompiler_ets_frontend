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

const fs = require('fs');
const sourceMap = require('source-map');
const path = require('path');

const PROJECT_ROOT = path.join(__dirname, '..', '..');
const COVERAGE_DIR = path.join(PROJECT_ROOT, 'coverage');
const COVERAGE_FILE = path.join(COVERAGE_DIR, 'coverage.json');
const NEW_COVERAGE_FILE = path.join(COVERAGE_DIR, 'newCoverage.json');

/**
 * Processes statement map data using source map consumer
 * @param {Object} statementMap - Statement map data
 * @param {Object} consumer - Source map consumer
 */
function processStatementMap(statementMap, consumer) {
    for (const id in statementMap) {
        const statement = statementMap[id];
        const startPos = consumer.originalPositionFor(statement.start);
        const endPos = consumer.originalPositionFor(statement.end);
        
        statement.start = { 
            line: startPos.line, 
            column: 0 
        };
        statement.end = {
            line: endPos.line,
            column: Number.MAX_SAFE_INTEGER 
        };
    }
}

/**
 * Processes function map data using source map consumer
 * @param {Object} functionMap - Function map data
 * @param {Object} consumer - Source map consumer
 */
function processFunctionMap(functionMap, consumer) {
    for (const id in functionMap) {
        const func = functionMap[id];
        
        const declStart = consumer.originalPositionFor(func.decl.start);
        const declEnd = consumer.originalPositionFor(func.decl.end);
        
        func.decl = {
            start: { 
                line: declStart.line, 
                column: 0  
            },
            end: {
                line: declEnd.line,
                column: Number.MAX_SAFE_INTEGER  
            }
        };
        
        func.loc = {
            start: { line: declStart.line, column: 0 },
            end: { line: declEnd.line, column: Number.MAX_SAFE_INTEGER }
        };
        
        func.line = declStart.line;
    }
}

/**
 * Processes branch map data using source map consumer
 * @param {Object} branchMap - Branch map data
 * @param {Object} consumer - Source map consumer
 */
function processBranchMap(branchMap, consumer) {
    for (const id in branchMap) {
        const branch = branchMap[id];
        
        // Process locations
        branch.locations.forEach(location => {
            const startPos = consumer.originalPositionFor(location.start);
            const endPos = consumer.originalPositionFor(location.end);
            
            location.start = { line: startPos.line, column: startPos.column };
            location.end = { line: endPos.line, column: endPos.column };
        });
        
        // Process loc
        const locStart = consumer.originalPositionFor(branch.loc.start);
        const locEnd = consumer.originalPositionFor(branch.loc.end);
        
        branch.loc = {
            start: { line: locStart.line, column: locStart.column },
            end: { line: locEnd.line, column: locEnd.column }
        };
        
        branch.line = locStart.line;
    }
}


/**
 * Filter out coverage data before line 16 and remove function declaration coverage
 * @param {Object} newCoverageData Original coverage data
 * @returns {Object} Filtered coverage data
 */
function filterCoverageByLine(newCoverageData) {
    const filteredCoverage = {};
    for (const filePath in newCoverageData) {
        const fileCoverage = newCoverageData[filePath];
        const filteredFileCoverage = {
            ...fileCoverage,
            statementMap: {},
            fnMap: {},
            branchMap: {},
            s: {}, 
            f: {}, 
            b: {}  
        };
        for (const stmtId in fileCoverage.statementMap) {
            const stmt = fileCoverage.statementMap[stmtId];
            if (stmt.start.line >= 16) {
                filteredFileCoverage.statementMap[stmtId] = stmt;
                filteredFileCoverage.s[stmtId] = fileCoverage.s[stmtId];
            }
        }
        for (const fnId in fileCoverage.fnMap) {
            const fn = fileCoverage.fnMap[fnId];
            if (fn.decl.start.line >= 16) {
                const newFn = {
                    ...fn,
                    decl: null, 
                    loc: {
                        start: { 
                            line: fn.decl.end.line + 1, 
                            column: 0 
                        },
                        end: fn.loc.end
                    },
                    line: fn.decl.end.line + 1 
                };
                filteredFileCoverage.fnMap[fnId] = newFn;
                filteredFileCoverage.f[fnId] = fileCoverage.f[fnId];
            }
        }
        for (const branchId in fileCoverage.branchMap) {
            const branch = fileCoverage.branchMap[branchId];
            if (branch.loc.start.line >= 16) {
                filteredFileCoverage.branchMap[branchId] = branch;
                filteredFileCoverage.b[branchId] = fileCoverage.b[branchId];
            }
        }
        if (Object.keys(filteredFileCoverage.statementMap).length > 0 ||
            Object.keys(filteredFileCoverage.fnMap).length > 0 ||
            Object.keys(filteredFileCoverage.branchMap).length > 0) {
            filteredCoverage[filePath] = filteredFileCoverage;
        }
    }
    return filteredCoverage;
}


/**
 * Collects and processes coverage data using source maps
 */
async function collectCoverage() {
    if (!fs.existsSync(COVERAGE_FILE)) {
        throw new Error(`Coverage file not found: ${COVERAGE_FILE}`);
    }

    const coverageData = JSON.parse(fs.readFileSync(COVERAGE_FILE, 'utf8'));
    const newCoverageData = {};

    for (const file in coverageData) {
        const mapFile = `${file}.map`;
        
        if (!fs.existsSync(mapFile)) {
            console.warn(`Source map not found for: ${file}`);
            continue;
        }

        const sourceMapData = JSON.parse(fs.readFileSync(mapFile, 'utf8'));
        const sources = sourceMapData.sources;
        const newFile = path.join(path.dirname(mapFile), sources[0]);

        await sourceMap.SourceMapConsumer.with(sourceMapData, null, (consumer) => {
            const fileCoverage = { ...coverageData[file] };
            fileCoverage.path = newFile;
            
            processStatementMap(fileCoverage.statementMap, consumer);
            processFunctionMap(fileCoverage.functionMap, consumer);
            processBranchMap(fileCoverage.branchMap, consumer);
            
            newCoverageData[newFile] = fileCoverage;
        });
    }

    const filteredCoverage = filterCoverageByLine(newCoverageData);

    fs.writeFileSync(
        NEW_COVERAGE_FILE,
        JSON.stringify(filteredCoverage, null, 4)
    );
}

// Execute and handle errors
collectCoverage()
    .then(() => console.log('Coverage collection completed successfully'))
    .catch(error => {
        console.error('Error collecting coverage:', error);
        process.exit(1);
    });