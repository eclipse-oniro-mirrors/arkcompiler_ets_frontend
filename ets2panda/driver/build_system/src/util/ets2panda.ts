/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as path from 'path';
import * as fs from 'fs';

import { initKoalaModules } from '../init/init_koala_modules';
import {
    BuildConfig,
    CompileJobInfo,
    FileInfo,
    DeclgenV1JobInfo,
    CompileJobType,
    AliasConfig,
    ArkTS
} from '../types';
import {
    Logger,
    LogDataFactory
} from '../logger';
import {
    changeDeclgenFileExtension,
    changeFileExtension,
    createFileIfNotExists,
    ensurePathExists,
    formEts2pandaCmd,
} from '../util/utils';
import {
    DECL_ETS_SUFFIX,
    DECL_TS_SUFFIX,
    STATIC_RECORD_FILE,
    STATIC_RECORD_FILE_CONTENT,
    TS_SUFFIX
} from '../pre_define';
import {
    PluginDriver,
    PluginHook
} from '../plugins/plugins_driver';
import { KitImportTransformer } from '../plugins/KitImportTransformer';
import { ErrorCode, DriverError } from '../util/error';
import {
    BS_PERF_FILE_NAME,
    RecordEvent,
    StatisticsRecorder,
} from '../util/statsRecorder'


enum Ets2pandaEvent {
    CREATE_INSTANCE = 'Create instance',
    PARSE = 'Parse stage',
    PLUGIN_PARSE = 'Parse plugins stage',
    DECLGEN = 'Declgen stage',
    CHECK = 'Checker stage',
    PLUGIN_CHECK = 'Checker plugins stage',
    EMIT = 'Emit binary stage',
    DESTROY_INSTANCE = 'Destroy instance'
}

function formEvent(event: Ets2pandaEvent): string {
    return '[Ets2panda] ' + event;
}

export class Ets2panda {
    private static instance?: Ets2panda;
    private readonly logger: Logger = Logger.getInstance();
    private readonly buildSdkPath: string;
    private readonly aliasConfig: Record<string, Record<string, AliasConfig>>;
    private readonly cacheDir: string;
    private readonly declgenV2OutDir?: string;
    private readonly pluginDriver: PluginDriver = PluginDriver.getInstance();
    private readonly recordType?: 'ON' | 'OFF';

    // NOTE: should be Ets2panda Wrapper Module
    // NOTE: to be refactored
    private readonly koalaModule: any;

    private constructor(buildConfig: BuildConfig) {
        this.koalaModule = initKoalaModules(buildConfig);
        this.buildSdkPath = buildConfig.buildSdkPath;
        this.aliasConfig = buildConfig.aliasConfig;
        this.cacheDir = buildConfig.cachePath;
        this.recordType = buildConfig.recordType;

        // NOTE: uncomment if you really need this
        // NOTE: decl files are internal files, not for external usage!!!!!
        this.declgenV2OutDir = undefined;
        // this.declgenV2OutDir = buildConfig.declgenV2OutPath;


        this.pluginDriver.initPlugins(buildConfig)
    }

    public static getInstance(buildConfig?: BuildConfig): Ets2panda {
        if (!Ets2panda.instance) {
            if (!buildConfig) {
                throw new Error('buildConfig is required for the first Ets2panda instantiation.');
            }
            Ets2panda.instance = new Ets2panda(buildConfig);
        }
        return Ets2panda.instance;
    }

    public static destroyInstance(): void {
        Ets2panda.instance = undefined;
    }

    public initalize() {
        const arkts: ArkTS = this.koalaModule.arkts;
        arkts.MemInitialize();
    }

    public finalize() {
        const arkts: ArkTS = this.koalaModule.arkts;
        arkts.MemFinalize();
    }

    private transformImportStatementsWithAliasConfig() {
        const { arkts, arktsGlobal } = this.koalaModule;
        let ast = arkts.EtsScript.fromContext();
        if (this.aliasConfig && Object.keys(this.aliasConfig).length > 0) {
            // if aliasConfig is set, transform aliasName@kit.xxx to default@ohos.xxx through the plugin
            this.logger.printDebug('Transforming import statements with alias config');
            let transformAst = new KitImportTransformer(
                arkts,
                arktsGlobal.compilerContext.program,
                this.buildSdkPath,
                this.aliasConfig
            ).transform(ast);
            this.pluginDriver.getPluginContext().setArkTSAst(transformAst);
        } else {
            this.pluginDriver.getPluginContext().setArkTSAst(ast);
        }
    }

    public compile(
        jobId: string,
        job: CompileJobInfo,
        isDebug: boolean = false,
        declGenCb?: () => void,
        compAbcCb?: () => void
    ): void {
        let statsRecorder = new StatisticsRecorder(
            path.resolve(this.cacheDir, BS_PERF_FILE_NAME),
            this.recordType,
            `Compile. Job id: ${jobId.slice(0, 5)}`
        );

        this.logger.printDebug(`Ets2panda.compile Job = ${JSON.stringify(job, null, 1)}`)

        const { input: inputFilePath, output: outputFilePath }: FileInfo = job.fileInfo;
        const source = fs.readFileSync(inputFilePath, 'utf-8');

        const ets2pandaCmd: string[] = formEts2pandaCmd(job.fileInfo, isDebug)
        this.logger.printDebug('ets2pandaCmd: ' + ets2pandaCmd.join(' '));

        const { arkts, arktsGlobal } = this.koalaModule;
        try {
            statsRecorder.record(formEvent(Ets2pandaEvent.CREATE_INSTANCE));
            arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
            arktsGlobal.filePath = inputFilePath;
            arktsGlobal.compilerContext = arkts.Context.createFromString(source);
            this.pluginDriver.getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);
            this.logger.printInfo('[Ets2panda] Created instance');

            statsRecorder.record(formEvent(Ets2pandaEvent.PARSE));
            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('[Ets2panda] Parsed');

            statsRecorder.record(formEvent(Ets2pandaEvent.PLUGIN_PARSE));
            this.transformImportStatementsWithAliasConfig()
            this.pluginDriver.runPluginHook(PluginHook.PARSED);
            this.logger.printInfo('[Ets2panda] Parser plugins finished');

            statsRecorder.record(formEvent(Ets2pandaEvent.CHECK));
            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('[Ets2panda] Checked');

            statsRecorder.record(formEvent(Ets2pandaEvent.DECLGEN));
            if (job.type & CompileJobType.DECL) {
                const outputAbcPath = job.fileInfo.output;
                const relativeDeclPath = changeFileExtension(
                    path.relative(this.cacheDir, outputAbcPath),
                    DECL_ETS_SUFFIX
                )
                const outputDeclFilePath = path.resolve(this.cacheDir, relativeDeclPath);
                ensurePathExists(outputDeclFilePath)

                // Generate 1.2 declaration files(a temporary solution while binary import not pushed)
                arkts.generateStaticDeclarationsFromContext(outputDeclFilePath);
                this.logger.printInfo(`[Ets2panda] Generated 1.2 decl file for ${inputFilePath}`)

                // Copy file to declgenV2OutDir
                if (this.declgenV2OutDir) {
                    const newPath = path.resolve(this.declgenV2OutDir, relativeDeclPath);
                    ensurePathExists(newPath)
                    fs.copyFileSync(outputDeclFilePath, newPath)
                }
                declGenCb?.();
            }
            if (job.type & CompileJobType.ABC) {
                ensurePathExists(outputFilePath);
                statsRecorder.record(formEvent(Ets2pandaEvent.PLUGIN_CHECK));
                let ast = arkts.EtsScript.fromContext();
                this.pluginDriver.getPluginContext().setArkTSAst(ast);
                this.pluginDriver.runPluginHook(PluginHook.CHECKED);
                this.logger.printInfo('[Ets2panda] Checker plugins finished');

                statsRecorder.record(formEvent(Ets2pandaEvent.EMIT));
                arkts.proceedToState(
                    arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED,
                    arktsGlobal.compilerContext.peer
                );
                compAbcCb?.();
                this.logger.printInfo(`[Ets2panda] Compiled abc file for ${inputFilePath}`)
            }
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                        'Compile abc files failed.',
                        error.message,
                        inputFilePath
                    )
                );
            }
        } finally {
            statsRecorder.record(formEvent(Ets2pandaEvent.DESTROY_INSTANCE));
            this.pluginDriver.runPluginHook(PluginHook.CLEAN);
            arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
            arkts.destroyConfig(arktsGlobal.config);
            statsRecorder.record(RecordEvent.END);
            statsRecorder.writeSumSingle();
        }
    }

    public compileSimultaneous(
        jobId: string,
        job: CompileJobInfo,
        isDebug: boolean = false,
        declGenCb?: () => void,
        compAbcCb?: () => void
    ): void {
        let statsRecorder = new StatisticsRecorder(
            path.resolve(this.cacheDir, BS_PERF_FILE_NAME),
            this.recordType,
            `Compile simultaneous. Job id: ${jobId.slice(0, 5)}`
        );

        this.logger.printDebug(`Ets2panda.compileSimultaneous Job = ${JSON.stringify(job, null, 1)}`)

        const { output: outputFilePath } = job.fileInfo;

        const ets2pandaCmd: string[] = formEts2pandaCmd(job.fileInfo, isDebug, true)
        this.logger.printDebug('ets2pandaCmd: ' + ets2pandaCmd.join(' '));

        let { arkts, arktsGlobal } = this.koalaModule;
        try {
            statsRecorder.record(formEvent(Ets2pandaEvent.CREATE_INSTANCE));
            arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
            arktsGlobal.compilerContext = arkts.Context.createContextGenerateAbcForExternalSourceFiles(job.fileList);
            this.pluginDriver.getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);
            this.logger.printInfo('[Ets2panda] Created instance');

            statsRecorder.record(formEvent(Ets2pandaEvent.PARSE));
            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('[Ets2panda] Parsed');

            statsRecorder.record(formEvent(Ets2pandaEvent.PLUGIN_PARSE));
            this.transformImportStatementsWithAliasConfig()
            this.pluginDriver.runPluginHook(PluginHook.PARSED);
            this.logger.printInfo('[Ets2panda] Parser plugins finished');

            statsRecorder.record(formEvent(Ets2pandaEvent.CHECK));
            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('[Ets2panda] Checked');

            statsRecorder.record(formEvent(Ets2pandaEvent.DECLGEN));
            if (job.type & CompileJobType.DECL) {
                for (const file of job.fileList) {
                    const relative: string = changeFileExtension(
                        path.relative(job.fileInfo.moduleRoot, file),
                        DECL_ETS_SUFFIX
                    )
                    const declEtsOutputPath: string = path.resolve(
                        this.cacheDir,
                        job.fileInfo.moduleName,
                        relative
                    )
                    ensurePathExists(declEtsOutputPath);
                    arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
                    this.logger.printInfo(`[Ets2panda] Generated 1.2 decl file for ${file}`)

                    // Copy file to declgenV2OutDir
                    if (this.declgenV2OutDir) {
                        const newPath = path.resolve(
                            this.declgenV2OutDir,
                            job.fileInfo.moduleName,
                            relative
                        )
                        ensurePathExists(newPath)
                        fs.copyFileSync(declEtsOutputPath, newPath)
                    }
                }
                declGenCb?.();
            }

            if (job.type & CompileJobType.ABC) {
                statsRecorder.record(formEvent(Ets2pandaEvent.PLUGIN_CHECK));
                ensurePathExists(outputFilePath);
                let ast = arkts.EtsScript.fromContext();
                this.pluginDriver.getPluginContext().setArkTSAst(ast);
                this.pluginDriver.runPluginHook(PluginHook.CHECKED);
                this.logger.printInfo('[Ets2panda] Checker plugins finished');

                statsRecorder.record(formEvent(Ets2pandaEvent.EMIT));
                arkts.proceedToState(
                    arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED,
                    arktsGlobal.compilerContext.peer
                );
                compAbcCb?.();
                this.logger.printInfo(`[Ets2panda] Compiled abc file for cycle ${jobId}`)
            }
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                        'Compile abc files failed.',
                        error.message,
                        job.fileInfo.input
                    )
                );
            }
        } finally {
            statsRecorder.record(formEvent(Ets2pandaEvent.DESTROY_INSTANCE));
            this.pluginDriver.runPluginHook(PluginHook.CLEAN);
            arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
            arkts.destroyConfig(arktsGlobal.config);
            statsRecorder.record(RecordEvent.END);
            statsRecorder.writeSumSingle();
        }
    }

    public declgenV1(
        jobInfo: DeclgenV1JobInfo,
        skipDeclCheck: boolean,
        genDeclAnnotations: boolean
    ): void {
        const inputFilePath = jobInfo.fileInfo.input;
        const source = fs.readFileSync(inputFilePath, 'utf8');
        const filePathFromModuleRoot: string = path.relative(jobInfo.fileInfo.moduleRoot, inputFilePath);
        const declEtsOutputPath: string = changeDeclgenFileExtension(
            path.resolve(jobInfo.declgenConfig.otuput, jobInfo.fileInfo.moduleName, filePathFromModuleRoot),
            DECL_ETS_SUFFIX
        );
        const etsOutputPath: string = changeDeclgenFileExtension(
            path.resolve(jobInfo.declgenConfig.bridgeCode, jobInfo.fileInfo.moduleName, filePathFromModuleRoot),
            TS_SUFFIX
        );
        ensurePathExists(declEtsOutputPath);
        ensurePathExists(etsOutputPath);
        const staticRecordPath = path.join(
            jobInfo.declgenConfig.otuput,
            STATIC_RECORD_FILE
        )
        const declEtsOutputDir = path.dirname(declEtsOutputPath);
        const staticRecordRelativePath = changeFileExtension(
            path.relative(declEtsOutputDir, staticRecordPath).replace(/\\/g, '\/'),
            "",
            DECL_TS_SUFFIX
        );
        createFileIfNotExists(staticRecordPath, STATIC_RECORD_FILE_CONTENT);
        let ets2pandaCmd = formEts2pandaCmd(jobInfo.fileInfo)
        this.logger.printDebug(`ets2panda cmd: ${ets2pandaCmd.join(' ')}`)

        let { arkts, arktsGlobal } = this.koalaModule;
        try {
            arktsGlobal.filePath = inputFilePath;
            arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
            arktsGlobal.compilerContext = arkts.Context.createFromStringWithHistory(source);
            this.pluginDriver.getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer, skipDeclCheck);
            let ast = arkts.EtsScript.fromContext();
            this.pluginDriver.getPluginContext().setArkTSAst(ast);
            this.pluginDriver.runPluginHook(PluginHook.PARSED);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer, skipDeclCheck);
            ast = arkts.EtsScript.fromContext();
            this.pluginDriver.getPluginContext().setArkTSAst(ast);
            this.pluginDriver.runPluginHook(PluginHook.CHECKED);

            // Generate 1.0 declaration files & 1.0 glue code
            arkts.generateTsDeclarationsFromContext(
                declEtsOutputPath,
                etsOutputPath,
                false,
                false,
                staticRecordRelativePath,
                genDeclAnnotations
            );
            this.logger.printInfo(`[Ets2panda] Generated 1.0 declaration file for ${inputFilePath}`)
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
                        'Failed to generate 1.0 declaration file.',
                        error.message,
                        inputFilePath
                    )
                );
            }
        } finally {
            this.pluginDriver.runPluginHook(PluginHook.CLEAN);
            arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
            arkts.destroyConfig(arktsGlobal.config);
        }
    }
}
