/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

import type { OptionValues } from 'commander';
import { Command, Option } from 'commander';
import * as fs from 'node:fs';
import * as path from 'node:path';
import * as ts from 'typescript';
import type { CommandLineOptions } from '../lib/CommandLineOptions';
import { cookBookTag } from '../lib/CookBookMsg';
import { Logger } from '../lib/Logger';
import { ARKTS_IGNORE_DIRS_OH_MODULES } from '../lib/utils/consts/ArktsIgnorePaths';
import { getConfiguredRuleTags, getConfigureRulePath, getRulesFromConfig } from '../lib/utils/functions/ConfiguredRulesProcess';
import { extractRuleTags } from '../lib/utils/functions/CookBookUtils';
import { logTscDiagnostic } from '../lib/utils/functions/LogTscDiagnostic';

const TS_EXT = '.ts';
const TSX_EXT = '.tsx';
const ETS_EXT = '.ets';
const JS_EXT = '.js';

interface CommanderParseOptions {
  exitOnFail?: boolean;
  disableErrorOutput?: boolean;
}

interface ProcessedArguments {
  inputFiles: string[];
  responseFiles: string[];
}

interface ParsedCommand {
  opts: OptionValues;
  args: ProcessedArguments;
}

const getFiles = (dir: string): string[] => {
  const resultFiles: string[] = [];
  if (dir.includes(ARKTS_IGNORE_DIRS_OH_MODULES)) {
    return [];
  }

  const files = fs.readdirSync(dir);
  for (let i = 0; i < files.length; ++i) {
    const name = path.join(dir, files[i]);
    if (fs.statSync(name).isDirectory()) {
      resultFiles.push(...getFiles(name));
    } else {
      const extension = path.extname(name);
      if (extension === TS_EXT || extension === TSX_EXT || extension === ETS_EXT || extension === JS_EXT) {
        resultFiles.push(name);
      }
    }
  }

  return resultFiles;
};

function addProjectFolder(projectFolder: string, previous: string[]): string[] {
  return previous.concat([projectFolder]);
}

function processProgramArguments(args: string[]): ProcessedArguments {
  const processed: ProcessedArguments = {
    inputFiles: [],
    responseFiles: []
  };
  for (const arg of args) {
    if (arg.startsWith('@')) {
      processed.responseFiles.push(arg.slice(1));
    } else {
      processed.inputFiles.push(arg);
    }
  }
  return processed;
}

function parseCommand(program: Command, cmdArgs: string[]): ParsedCommand {
  program.parse(cmdArgs);
  return {
    opts: program.opts<OptionValues>(),
    args: processProgramArguments(program.args)
  };
}

function formSdkOptions(cmdOptions: CommandLineOptions, commanderOpts: OptionValues): void {
  if (commanderOpts.sdkExternalApiPath) {
    cmdOptions.sdkExternalApiPath = commanderOpts.sdkExternalApiPath;
  }
  if (commanderOpts.sdkDefaultApiPath) {
    cmdOptions.sdkDefaultApiPath = commanderOpts.sdkDefaultApiPath;
  }
  if (commanderOpts.arktsWholeProjectPath) {
    cmdOptions.arktsWholeProjectPath = commanderOpts.arktsWholeProjectPath;
  }
}

function formMigrateOptions(cmdOptions: CommandLineOptions, commanderOpts: OptionValues): void {
  if (commanderOpts.migrate) {
    cmdOptions.linterOptions.migratorMode = true;
    cmdOptions.linterOptions.enableAutofix = true;
  }
  if (commanderOpts.migrationBackupFile === false) {
    cmdOptions.linterOptions.noMigrationBackupFile = true;
  }
  if (commanderOpts.migrationMaxPass) {
    const num = Number(commanderOpts.migrationMaxPass);
    cmdOptions.linterOptions.migrationMaxPass = isNaN(num) ? 0 : num;
  }
  if (commanderOpts.migrationReport) {
    cmdOptions.linterOptions.migrationReport = true;
  }
  if (commanderOpts.arktsWholeProjectPath) {
    cmdOptions.linterOptions.wholeProjectPath = commanderOpts.arktsWholeProjectPath;
  }
}

function formIdeInteractive(cmdOptions: CommandLineOptions, commanderOpts: OptionValues): void {
  if (commanderOpts.ideInteractive) {
    cmdOptions.linterOptions.ideInteractive = true;
  }
  if (commanderOpts.checkTsAndJs) {
    cmdOptions.linterOptions.checkTsAndJs = true;
  }
  if (commanderOpts.autofixCheck) {
    cmdOptions.linterOptions.autofixCheck = true;
  }
}

function formArkts2Options(cmdOptions: CommandLineOptions, commanderOpts: OptionValues): void {
  if (commanderOpts.arkts2) {
    cmdOptions.linterOptions.arkts2 = true;
  }
  if (commanderOpts.skipLinter) {
    cmdOptions.skipLinter = true;
  }
  if (commanderOpts.homecheck) {
    cmdOptions.homecheck = true;
  }
  if (commanderOpts.outputFilePath) {
    cmdOptions.outputFilePath = path.normalize(commanderOpts.outputFilePath);
  }
  if (commanderOpts.verbose) {
    cmdOptions.verbose = true;
  }
  if (commanderOpts.enableInterop) {
    cmdOptions.scanWholeProjectInHomecheck = true;
  }
}

function formCommandLineOptions(parsedCmd: ParsedCommand): CommandLineOptions {
  const opts: CommandLineOptions = {
    inputFiles: parsedCmd.args.inputFiles.map((file) => {
      return path.normalize(file);
    }),
    linterOptions: {
      useRtLogic: true,
      interopCheckMode: false
    }
  };
  const options = parsedCmd.opts;
  if (options.TSC_Errors) {
    opts.logTscErrors = true;
  }
  if (options.devecoPluginMode) {
    opts.devecoPluginModeDeprecated = true;
  }
  if (options.checkTsAsSource !== undefined) {
    opts.linterOptions.checkTsAsSource = options.checkTsAsSource;
  }
  if (options.projectFolder) {
    doProjectFolderArg(options.projectFolder, opts);
    opts.linterOptions.projectFolderList = options.projectFolder;
  }
  if (options.project) {
    doProjectArg(options.project, opts);
  }
  if (options.autofix) {
    opts.linterOptions.enableAutofix = true;
  }
  if (options.warningsAsErrors) {
    opts.linterOptions.warningsAsErrors = true;
  }
  if (options.useRtLogic !== undefined) {
    opts.linterOptions.useRtLogic = options.useRtLogic;
  }
  processRuleConfig(opts, options);
  processAutofixRuleConfig(opts, options);
  formIdeInteractive(opts, options);
  formSdkOptions(opts, options);
  formMigrateOptions(opts, options);
  formArkts2Options(opts, options);
  return opts;
}

function processRuleConfig(commandLineOptions: CommandLineOptions, options: OptionValues): void {
  const configureRulePath = getConfigureRulePath(options);
  const configuredRulesMap = getRulesFromConfig(configureRulePath);
  const arkTSRulesMap = extractRuleTags(cookBookTag);
  commandLineOptions.linterOptions.ruleConfigTags = getConfiguredRuleTags(arkTSRulesMap, configuredRulesMap);
}

function processAutofixRuleConfig(commandLineOptions: CommandLineOptions, options: OptionValues): void {
  if (options.ruleConfig) {
    return;
  }
  const autofixConfigureRulePath = options.autofixRuleConfig;
  if (!autofixConfigureRulePath || autofixConfigureRulePath.length === 0) {
    return;
  }
  const stats = fs.statSync(path.normalize(options.autofixRuleConfig));
  if (!stats.isFile()) {
    Logger.error(`The file at ${options.autofixRuleConfig} path does not exist!`);
    return;
  }
  const configuredRulesMap = getRulesFromConfig(autofixConfigureRulePath);
  const arkTSRulesMap = extractRuleTags(cookBookTag);
  commandLineOptions.linterOptions.autofixRuleConfigTags = getConfiguredRuleTags(arkTSRulesMap, configuredRulesMap);
}

function createCommand(): Command {
  const program = new Command();
  program.
    name('tslinter').
    description('Linter for TypeScript sources').
    version('0.0.2').
    configureHelp({ helpWidth: 100 }).
    exitOverride();
  program.
    option('-E, --TSC_Errors', 'show error messages from Tsc').
    option('--check-ts-as-source', 'check TS files as source files').
    option('-p, --project <project_file>', 'path to TS project config file').
    option(
      '-f, --project-folder <project_folder>',
      'path to folder containing TS files to verify',
      addProjectFolder,
      []
    ).
    option('--autofix', 'automatically fix problems found by linter').
    option('--arkts-2', 'enable ArkTS 2.0 mode').
    option('--use-rt-logic', 'run linter with RT logic').
    option('-e, --sdk-external-api-path <paths...>', 'paths to external API files').
    option('-d, --sdk-default-api-path <path>', 'paths to default API files').
    option('--ide-interactive', 'Migration Helper IDE interactive mode').
    option('-w, --arkts-whole-project-path <path>', 'path to whole project').
    option('--migrate', 'run as ArkTS migrator').
    option('--skip-linter', 'skip linter rule validation and autofix').
    option('--homecheck', 'added homecheck rule validation').
    option('--no-migration-backup-file', 'Disable the backup files in migration mode').
    option('--migration-max-pass <num>', 'Maximum number of migration passes').
    option('--migration-report', 'Generate migration report').
    option('--check-ts-and-js', 'check ts and js files').
    option('--only-arkts2-syntax-rules', 'only syntax rules').
    option('-o, --output-file-path <path>', 'path to store all log and result files').
    option('--verbose', 'set log level to see debug messages').
    option('--enable-interop', 'scan whole project to report 1.1 import 1.2').
    option('--rule-config <path>', 'Path to the rule configuration file').
    option('--autofix-check', 'confirm whether the user needs automatic repair').
    option('--autofix-rule-config <path>', 'Path to the autofix rule configuration file').
    addOption(new Option('--warnings-as-errors', 'treat warnings as errors').hideHelp(true)).
    addOption(new Option('--no-check-ts-as-source', 'check TS files as third-party libary').hideHelp(true)).
    addOption(new Option('--no-use-rt-logic', 'run linter with SDK logic').hideHelp(true)).
    addOption(new Option('--deveco-plugin-mode', 'run as IDE plugin (obsolete)').hideHelp(true));
  program.argument('[srcFile...]', 'files to be verified');
  return program;
}

export function parseCommandLine(
  commandLineArgs: string[],
  commanderParseOpts: CommanderParseOptions = {}
): CommandLineOptions {
  const { exitOnFail = true, disableErrorOutput = false } = commanderParseOpts;
  const program = createCommand();
  if (disableErrorOutput) {
    program.configureOutput({
      writeErr: () => {},
      writeOut: () => {}
    });
  }

  // method parse() eats two first args, so make them dummy
  const cmdArgs: string[] = ['dummy', 'dummy'];
  cmdArgs.push(...commandLineArgs);
  let parsedCmd: ParsedCommand;
  try {
    parsedCmd = parseCommand(program, cmdArgs);
  } catch (error) {
    if (exitOnFail) {
      process.exit(-1);
    }
    throw error;
  }
  processResponseFiles(parsedCmd);
  return formCommandLineOptions(parsedCmd);
}

function processResponseFiles(parsedCmd: ParsedCommand): void {
  if (!parsedCmd.args.responseFiles.length) {
    return;
  }
  const rspFiles = parsedCmd.args.responseFiles;
  for (const rspFile of rspFiles) {
    try {
      const rspArgs = fs.
        readFileSync(rspFile).
        toString().
        split('\n').
        filter((e) => {
          return e.trimEnd();
        });
      const cmdArgs = ['dummy', 'dummy'];
      cmdArgs.push(...rspArgs);
      const parsedRsp = parseCommand(createCommand(), cmdArgs);
      Object.assign(parsedCmd.opts, parsedRsp.opts);
      parsedCmd.args.inputFiles.push(...parsedRsp.args.inputFiles);
    } catch (error) {
      Logger.error('Failed to read response file: ' + error);
      process.exit(-1);
    }
  }
}

function doProjectFolderArg(prjFolders: string[], opts: CommandLineOptions): void {
  for (let i = 0; i < prjFolders.length; i++) {
    const prjFolderPath = prjFolders[i];
    try {
      opts.inputFiles.push(...getFiles(prjFolderPath));
    } catch (error) {
      Logger.error('Failed to read folder: ' + error);
      process.exit(-1);
    }
  }
}

function doProjectArg(cfgPath: string, opts: CommandLineOptions): void {
  // Process project file (tsconfig.json) and retrieve config arguments.
  const configFile = cfgPath;

  const host: ts.ParseConfigFileHost = ts.sys as ts.System & ts.ParseConfigFileHost;

  const diagnostics: ts.Diagnostic[] = [];

  try {
    const oldUnrecoverableDiagnostic = host.onUnRecoverableConfigFileDiagnostic;
    host.onUnRecoverableConfigFileDiagnostic = (diagnostic: ts.Diagnostic): void => {
      diagnostics.push(diagnostic);
    };
    opts.parsedConfigFile = ts.getParsedCommandLineOfConfigFile(configFile, {}, host);
    host.onUnRecoverableConfigFileDiagnostic = oldUnrecoverableDiagnostic;

    if (opts.parsedConfigFile) {
      diagnostics.push(...ts.getConfigFileParsingDiagnostics(opts.parsedConfigFile));
    }

    if (diagnostics.length > 0) {
      // Log all diagnostic messages and exit program.
      Logger.error('Failed to read config file.');
      logTscDiagnostic(diagnostics, Logger.info);
      process.exit(-1);
    }
  } catch (error) {
    Logger.error('Failed to read config file: ' + error);
    process.exit(-1);
  }
}
