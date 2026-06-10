import { Logger, LogDataFactory } from '../../ets_src/logger';
import { ErrorCode } from '../../ets_src/util/error';
import { assertStringContains, assertTrue, fail } from './assertions';

function main(): void {
  let logger: Logger = Logger.getInstance();
  logger.printInfo('build started');
  try {
    logger.printErrorAndExit(LogDataFactory.newDetailedInstance(ErrorCode.BUILDSYSTEM_INVALID_CONFIG, 'Invalid config.', 'fatal path', '', [], ''));
  } catch (error) {
    let message: string = (error as Error).message;
    assertStringContains(message, 'Invalid config.');
    assertTrue(logger.hasErrors(), 'logger error state');
    console.log('logger-ok');
    return;
  }
  fail('Logger.printErrorAndExit must throw');
}
