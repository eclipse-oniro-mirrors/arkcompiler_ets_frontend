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
const path = require('path');
const libCoverage = require('istanbul-lib-coverage');
const libReport = require('istanbul-lib-report');
const reports = require('istanbul-reports');

const projectRoot = path.resolve(__dirname, '../..');
const coverageDir = path.join(projectRoot, 'coverage');
const reportDir = path.join(coverageDir, 'arkcompiler/ets_frontend/ets2panda/linter/src');
const ABS_REPORT_DIR = 'arkcompiler/ets_frontend/ets2panda/linter/src';

const coverageFile = path.join(coverageDir, 'newCoverage.json');
if (!fs.existsSync(coverageFile)) {
  throw new Error(`Coverage file not found: ${coverageFile}`);
}

const coverageData = JSON.parse(fs.readFileSync(coverageFile, 'utf8'));

Object.keys(coverageData).forEach(filePath => {
  coverageData[filePath].fullPath = path.resolve(projectRoot, filePath);
});

const coverageMap = libCoverage.createCoverageMap(coverageData);
console.log(coverageMap);
const context = libReport.createContext({
  dir: reportDir,
  watermarks: {
    statements: [50, 80],
    branches: [50, 80],
    functions: [50, 80],
    lines: [50, 80]
  },
  coverageMap
});

reports.create('html', {}).execute(context);

function enhanceHtmlReports() {
  const indexPath = path.join(reportDir, 'index.html');
  if (!fs.existsSync(indexPath)) 
    {
        return;
    }

  let html = fs.readFileSync(indexPath, 'utf8');

  html = html.replace(
    /<a href="(.+?\/index\.html)">(.+?)<\/a>/g,
    (match, link, name) => {
      const absPath = path.join(ABS_REPORT_DIR, link.replace('/index.html', ''));
      return `<a href="${link}" title="${absPath}">${absPath}</a>`;
    }
  );
  
  fs.writeFileSync(indexPath, html);
  console.log(`Added full paths to: ${indexPath}`);
}

enhanceHtmlReports();
console.log(`View coverage at: file://${reportDir}/index.html`);