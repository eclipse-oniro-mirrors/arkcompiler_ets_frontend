export function fail(message: string): void {
  throw new Error(message);
}

export function assertTrue(value: boolean, message: string): void {
  if (!value) {
    fail(message);
  }
}

export function assertEquals(actual: string, expected: string): void {
  if (actual !== expected) {
    fail('expected ' + expected + ', got ' + actual);
  }
}

export function assertStringContains(actual: string, expected: string): void {
  if (actual.indexOf(expected) < 0) {
    fail('expected string to contain ' + expected);
  }
}
