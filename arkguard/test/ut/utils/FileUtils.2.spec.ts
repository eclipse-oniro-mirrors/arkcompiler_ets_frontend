/**
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

import { describe, it } from 'mocha';
import { expect } from 'chai';
import sinon from 'sinon';
import proxyquire from 'proxyquire';
import { constants } from 'fs';

import { IOptions } from '../../../src/configs/IOptions';

const fsStub = {
  existsSync: sinon.stub(),
  accessSync: sinon.stub(),
  statSync: sinon.stub(),
};

const fsExtraStub = {
  readJsonSync: sinon.stub()
};

const { FileUtils, IOptions } = proxyquire('../../../src/utils/FileUtils', {
  'fs': fsStub,
  'fs-extra': fsExtraStub
});

describe('FileUtils.readFileAsJson', () => {
  let consoleErrorStub:sinon.SinonStub;
  beforeEach(() => {
    consoleErrorStub = sinon.stub(console, 'error');
  });

  afterEach(() => {
    sinon.restore();
  });

  it('should return undefined if file does not exist', () => {
    fsStub.statSync.throws(new Error('ENOENT'));
    const result = FileUtils.readFileAsJson('fakePath.json');
    expect(result).to.be.undefined;
    expect(consoleErrorStub.calledWith('File <fakePath.json> is not found.')).to.be.true;
  });

  it('should return undefined if readJsonSync throws an error', () => {
    fsStub.statSync.returns({ isFile: () => true, isDirectory: () => false });
    fsExtraStub.readJsonSync.throws(new Error('read error'));
    const result = FileUtils.readFileAsJson('fakePath.json');
    expect(result).to.be.undefined;
    expect(consoleErrorStub.calledWith('json file read error: fakePath.json')).to.be.true;
  });

  it('should return JSON content if file exists and is valid', () => {
    const jsonContent: IOptions = { mCompact: true };
    fsStub.statSync.returns({ isFile: () => true, isDirectory: () => false });
    fsExtraStub.readJsonSync.returns(jsonContent);
    const result = FileUtils.readFileAsJson('validPath.json');
    expect(result).to.deep.equal(jsonContent);
  });
});

describe('FileUtils.getFileExtension', () => {
  it('should return undefined for empty string', () => {
    const result = FileUtils.getFileExtension('');
    expect(result).to.be.undefined;
  });

  it('should return undefined for string without dot', () => {
    const result = FileUtils.getFileExtension('filename');
    expect(result).to.be.undefined;
  });

  it('should return undefined for path string without dot', () => {
    const result = FileUtils.getFileExtension('path/to.test/filename');
    expect(result).to.be.undefined;
  });

  it('should return undefined for string ending with a dot', () => {
    const result = FileUtils.getFileExtension('filename.');
    expect(result).to.equal('');
  });

  it('should return extension for valid file path', () => {
    const result = FileUtils.getFileExtension('/path/to/file.txt');
    expect(result).to.equal('txt');
  });

  it('should return extension for valid file name with multiple dots', () => {
    const result = FileUtils.getFileExtension('/path/to/file.name.with.dots.txt');
    expect(result).to.equal('txt');
  });
});

describe('FileUtils.isRelativePath', () => {
  it('should return true for relative paths starting with ./', () => {
    const result = FileUtils.isRelativePath('./file.txt');
    expect(result).to.be.true;
  });

  it('should return true for relative paths starting with ../', () => {
    const result = FileUtils.isRelativePath('../file.txt');
    expect(result).to.be.true;
  });

  it('should return true for relative paths starting with .\\', () => {
    const result = FileUtils.isRelativePath('.\\file.txt');
    expect(result).to.be.true;
  });

  it('should return true for relative paths starting with ..\\', () => {
    const result = FileUtils.isRelativePath('..\\file.txt');
    expect(result).to.be.true;
  });

  it('should return false for absolute paths', () => {
    const result = FileUtils.isRelativePath('/absolute/path/to/file.txt');
    expect(result).to.be.false;
  });

  it('should return false for Windows absolute paths', () => {
    const result = FileUtils.isRelativePath('C:\\absolute\\path\\to\\file.txt');
    expect(result).to.be.false;
  });
});

describe('FileUtils.isReadableFile', () => {
  afterEach(() => {
    sinon.restore();
  });

  it('should return true if file is readable', () => {
    fsStub.statSync.returns({ isFile: () => true, isDirectory: () => false });
    const result = FileUtils.isReadableFile('readableFile.txt');
    expect(result).to.be.true;
  });

  it('should return false if file does not exist', () => {
    fsStub.statSync.throws(new Error('ENOENT'));
    const result = FileUtils.isReadableFile('nonExistentFile.txt');
    expect(result).to.be.false;
  });

  it('should return false if file is not readable (accessSync fallback)', () => {
    fsStub.statSync.returns({ isFile: () => false, isDirectory: () => true });
    fsStub.accessSync.withArgs('unreadableFile.txt', constants.R_OK).throws(new Error('Not readable'));
    const result = FileUtils.isReadableFile('unreadableFile.txt');
    expect(result).to.be.false;
  });
});

describe('FileUtils.toUnixPath', () => {
  let osStub: sinon.SinonStub;
  let pathStub: sinon.SinonStub;

  beforeEach(() => {
    osStub = sinon.stub(require('os'), 'platform');
    pathStub = sinon.stub(require('path'), 'sep').value('/'); // Default to Unix-like separator
  });

  afterEach(() => {
    sinon.restore();
  });

  it('should convert Windows path to Unix path', () => {
    osStub.returns('win32');
    const proxyquireUtils = proxyquire('../../../src/utils/FileUtils', {
      'os': {
        platform: osStub
      },
      'path': {
        sep: '\\',
        posix: {
          join: (...paths: string[]) => paths.join('/')
        }
      }
    });
    const result = proxyquireUtils.FileUtils.toUnixPath('C:\\path\\to\\file.txt');
    expect(result).to.equal('C:/path/to/file.txt');
  });

  it('should return Unix path unchanged', () => {
    osStub.returns('linux');
    const result = FileUtils.toUnixPath('/path/to/file.txt');
    expect(result).to.equal('/path/to/file.txt');
  });
});

describe('FileUtils.getAbsPathBaseConfigPath', () => {
  it('should return absolute path', () => {
    expect(FileUtils.getAbsPathBaseConfigPath('/abs/path/abc', 'file.txt')).to.equal('/abs/path/file.txt')
  });

  // Return non-absolute path? May need to be fixed.
  it('should return non-absolute path', () => {
    expect(FileUtils.getAbsPathBaseConfigPath('not_abs/path/abc', 'file.txt')).to.equal('not_abs/path/file.txt')
  });

  it('should handle empty paths', () => {
    expect(FileUtils.getAbsPathBaseConfigPath('', '')).to.equal('.')
  });
});