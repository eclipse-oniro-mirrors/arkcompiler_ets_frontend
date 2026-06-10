import { LogData } from '../logger';

export class ErrorCode {
  public static readonly BUILDSYSTEM_COMPILE_ABC_FAIL: string = '11410001';
  public static readonly BUILDSYSTEM_LINK_ABC_FAIL: string = '11410002';
  public static readonly BUILDSYSTEM_SOURCEROOTS_NOT_SET_FAIL: string = '11410003';
  public static readonly BUILDSYSTEM_UNRECOGNIZED_MODULEROOTPATH: string = '11410004';
  public static readonly BUILDSYSTEM_MODULE_INFO_NOT_CORRECT_FAIL: string = '11410005';
  public static readonly BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_CORRECT_FAIL: string = '11410006';
  public static readonly BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL: string = '11410007';
  public static readonly BUILDSYSTEM_LOAD_PLUGIN_FAIL: string = '11410008';
  public static readonly BUILDSYSTEM_ARK_LINK_NOT_FOUND_FAIL: string = '11410009';
  public static readonly BUILDSYSTEM_SDK_NOT_EXIST_FAIL: string = '11410010';
  public static readonly BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_FOUND: string = '11410011';
  public static readonly BUILDSYSTEM_PLUGIN_CONTEXT_RESET_PROJECT_CONFIG: string = '11410012';
  public static readonly BUILDSYSTEM_DECLGEN_FAIL: string = '11410013';
  public static readonly BUILDSYSTEM_LOAD_HASH_CACHE_FAIL: string = '11410014';
  public static readonly BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL: string = '11410015';
  public static readonly BUILDSYSTEM_DEPENDENCY_ANALYZER_NOT_FOUND_FAIL: string = '11410016';
  public static readonly BUILDSYSTEM_ABC_FILE_MISSING_IN_BCHAR: string = '11410017';
  public static readonly BUILDSYSTEM_HANDLE_ENTRY_FILE: string = '11410018';
  public static readonly BUILDSYSTEM_PATH_RESOLVE_FAIL: string = '11410019';
  public static readonly BUILDSYSTEM_INTEROP_SDK_NOT_FIND: string = '11410020';
  public static readonly BUILDSYSTEM_INIT_ALIAS_CONFIG_FAILED: string = '11410021';
  public static readonly BUILDSYSTEM_PLUGIN_ALIAS_CONFIG_PARSING_FAIL: string = '11410022';
  public static readonly BUILDSYSTEM_ABC_FILE_NOT_EXIST_IN_BCHAR: string = '11410023';
  public static readonly BUILDSYSTEM_ALIAS_MODULE_PATH_NOT_EXIST: string = '11410024';
  public static readonly BUILDSYSTEM_ENTRY_FILE_NOT_EXIST: string = '11410025';
  public static readonly BUILDSYSTEM_COMPILE_FAILED_IN_WORKER: string = '11410026';
  public static readonly BUILDSYSTEM_DECLGEN_FAILED_IN_WORKER: string = '11410027';
  public static readonly BUILDSYSTEM_DYNAMIC_MODULE_DECL_FILE_NOT_FOUND: string = '11410028';
  public static readonly BUILDSYSTEM_ERRORS_OCCURRED: string = '11410029';
  public static readonly BUILDSYSTEM_GRAPH_ERROR: string = '11410030';
  public static readonly BUILDSYSTEM_PACKAGENAME_NOT_INCLUDED_IN_MODULEINFOS: string = '11410031';
  public static readonly BUILDSYSTEM_OBFUSCATION_CONFIG_FILE_ERROR: string = '11410032';
  public static readonly BUILDSYSTEM_INIT_OBFUSCATION_CONFIG_FAIL: string = '11410033';
  public static readonly BUILDSYSTEM_GEN_OBFUSCATION_CONFIG_FAIL: string = '11410034';
  public static readonly BUILDSYSTEM_OBFUSCATE_ABC_FAIL: string = '11410035';
  public static readonly BUILDSYSTEM_ARK_GUARD_NOT_FOUND_FAIL: string = '11410036';
  public static readonly BUILDSYSTEM_ARK_DISASM_FAIL: string = '11410037';
  public static readonly BUILDSYSTEM_PATH_TOO_LONG: string = '11410038';
  public static readonly BUILDSYSTEM_TRANSFORM_LIB_FAIL: string = '11410039';
  public static readonly BUILDSYSTEM_ENTRY_DISPATCH_FAIL: string = '11410040';
  public static readonly BUILDSYSTEM_INVALID_CONFIG: string = '11410041';
  public static readonly BUILDSYSTEM_MISSING_MODULE_PATH: string = '11410042';
  public static readonly BUILDSYSTEM_UNSUPPORTED_FEATURE: string = '11410043';
  public static readonly BUILDSYSTEM_MISSING_BUILD_CONFIG: string = '11410044';
  public static readonly BUILDSYSTEM_MISSING_NATIVE_LIBRARY: string = '11410045';
  public static readonly BUILDSYSTEM_MISSING_OUTPUT_ABC: string = '11410046';
  public static readonly BUILDSYSTEM_EMPTY_OUTPUT_ABC: string = '11410047';
  public static readonly INVALID_PARAMS: string = '11311001';
  public static readonly CONFIGURATION_FILE_FORMAT_ERROR: string = '11311002';
  public static readonly UNKNOWN_KEEP_OPTION: string = '11311003';
  public static readonly CLASS_SPECIFICATION_FORMAT_ERROR: string = '11311004';
  public static readonly NAME_CACHE_FORMAT_ERROR: string = '11311005';
  public static readonly NAME_CACHE_MODULE_FORMAT_ERROR: string = '11311006';
  public static readonly NAME_CACHE_FUNCTION_FORMAT_ERROR: string = '11311007';
  public static readonly UNKNOWN_NAME_GENERATOR_TYPE: string = '11311008';
  public static readonly GENERIC_ERROR: string = '11311900';
}

export class DriverError extends Error {
  public logData: LogData;

  public constructor(logData: LogData) {
    super(logData.toString());
    this.logData = logData;
  }

  public toString(): string {
    return this.logData.toString();
  }
}
