import { DECL_ETS_SUFFIX } from '../pre_define';

export type EnvironmentResolver = (name: string) => string;

export function toUnixPath(path: string): string {
  return path.replaceAll('\\', '/');
}

export function normalizePath(path: string): string {
  return normalizeUnixPath(path);
}

export function normalizeUnixPath(path: string): string {
  if (path.length === 0) {
    return '.';
  }
  let input: string = toUnixPath(path);
  let absolute: boolean = input.startsWith('/');
  let trailingSlash: boolean = input.length > 1 && input.endsWith('/');
  let parts: string[] = input.split('/');
  let stack: string[] = [];
  for (let i: int = 0; i < parts.length; i++) {
    let part: string = parts[i];
    if (part.length === 0 || part === '.') {
      continue;
    }
    if (part === '..') {
      if (stack.length > 0 && stack[stack.length - 1] !== '..') {
        stack.pop();
      } else if (!absolute) {
        stack.push(part);
      }
      continue;
    }
    stack.push(part);
  }
  let result: string = stack.join('/');
  if (absolute) {
    result = '/' + result;
  }
  if (result.length === 0) {
    result = absolute ? '/' : '.';
  }
  if (trailingSlash && result !== '/') {
    result += '/';
  }
  return result;
}

export function isAbsolutePath(path: string): boolean {
  return toUnixPath(path).startsWith('/');
}

export function basename(path: string): string {
  let normalized: string = toUnixPath(path);
  while (normalized.length > 1 && normalized.endsWith('/')) {
    normalized = normalized.substring(0, normalized.length - 1);
  }
  let index: int = normalized.lastIndexOf('/');
  if (index < 0) {
    return normalized;
  }
  return normalized.substring(index + 1);
}

export function dirname(path: string): string {
  let normalized: string = toUnixPath(path);
  while (normalized.length > 1 && normalized.endsWith('/')) {
    normalized = normalized.substring(0, normalized.length - 1);
  }
  let index: int = normalized.lastIndexOf('/');
  if (index < 0) {
    return '.';
  }
  if (index === 0) {
    return '/';
  }
  return normalized.substring(0, index);
}

export function extname(path: string): string {
  let base: string = basename(path);
  if (base.endsWith(DECL_ETS_SUFFIX)) {
    return DECL_ETS_SUFFIX;
  }
  let index: int = base.lastIndexOf('.');
  if (index <= 0) {
    return '';
  }
  return base.substring(index);
}

export function getFileExtension(filePath: string): string {
  return extname(filePath);
}

export function changeFileExtension(file: string, targetExt: string, originExt: string = ''): string {
  let currentExt: string = originExt.length === 0 ? getFileExtension(file) : originExt;
  if (currentExt.length === 0 || !file.endsWith(currentExt)) {
    return file + targetExt;
  }
  return file.substring(0, file.length - currentExt.length) + targetExt;
}

export function changeDeclgenFileExtension(file: string, targetExt: string): string {
  if (file.endsWith(DECL_ETS_SUFFIX)) {
    return changeFileExtension(file, targetExt, DECL_ETS_SUFFIX);
  }
  return changeFileExtension(file, targetExt);
}

export function joinPath(...paths: string[]): string {
  let result: string = '';
  for (let i: int = 0; i < paths.length; i++) {
    let part: string = toUnixPath(paths[i]);
    if (part.length === 0) {
      continue;
    }
    if (part.startsWith('/')) {
      result = part;
      continue;
    }
    if (result.length === 0 || result.endsWith('/')) {
      result += part;
    } else {
      result += '/' + part;
    }
  }
  if (result.length === 0) {
    return '.';
  }
  return normalizeUnixPath(result);
}

export function resolvePath(...paths: string[]): string {
  let result: string = '';
  for (let i: int = 0; i < paths.length; i++) {
    let part: string = toUnixPath(paths[i]);
    if (part.length === 0) {
      continue;
    }
    if (part.startsWith('/')) {
      result = part;
    } else if (result.length === 0 || result.endsWith('/')) {
      result += part;
    } else {
      result += '/' + part;
    }
  }
  if (result.length === 0) {
    return '.';
  }
  return normalizeUnixPath(result);
}

export function getAbsPathBaseConfigPath(configPath: string, relativePath: string): string {
  return joinPath(dirname(configPath), relativePath);
}

export function isSubPathOf(targetPath: string, parentDir: string): boolean {
  let parent: string = normalizeUnixPath(parentDir);
  let target: string = normalizeUnixPath(targetPath);
  if (parent.length > 1 && parent.endsWith('/')) {
    parent = parent.substring(0, parent.length - 1);
  }
  if (target.length > 1 && target.endsWith('/')) {
    target = target.substring(0, target.length - 1);
  }
  return target === parent || target.startsWith(parent + '/');
}

export function computeHash(str: string): string {
  let hash: int = 0;
  for (let i: int = 0; i < str.length; i++) {
    hash = ((hash * 31) + str.charCodeAt(i)) & 0x7fffffff;
  }
  return hash.toString();
}

function substituteEnvVarInString(str: string, resolver: EnvironmentResolver): string {
  let result: string = str;
  let start: int = result.indexOf('${');
  while (start >= 0) {
    let end: int = result.indexOf('}', start + 2);
    if (end < 0) {
      return result;
    }
    let name: string = result.substring(start + 2, end);
    let value: string = resolver(name);
    if (value.length === 0) {
      throw new Error(name + ' environment variable is not set');
    }
    result = result.substring(0, start) + value + result.substring(end + 1);
    start = result.indexOf('${', start + value.length);
  }
  return result;
}

export function substituteEnvVarsInString(str: string, resolver: EnvironmentResolver): string {
  return substituteEnvVarInString(str, resolver);
}

export function substituteEnvVarsInJSON(json: Object, resolver: EnvironmentResolver): Object {
  let text: string = JSON.stringify(json);
  let result: string = substituteEnvVarInString(text, resolver);
  return JSON.parse(result) as Object;
}

export function containWildcards(item: string): boolean {
  return item.indexOf('*') >= 0 || item.indexOf('?') >= 0;
}

export function getProfilePath(srcPath: string): string {
  return srcPath.replace('$profile:', '') + '.json';
}
