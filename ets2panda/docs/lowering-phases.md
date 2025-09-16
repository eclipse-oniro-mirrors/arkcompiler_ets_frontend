## Phases in `es2panda` as of 2024.10.20

- `plugins-after-parse`: call compiler plugins that work immediately after parsing, if any.

- `TopLevelStatements`: move top-level statements into a synthetic class. For example,
```
class C {
  public constructor() {}
}

let x = 33

function f(): int {
  return x
}
```
becomes 
```
class C {
  public constructor() {}
}

abstract class ETSGLOBAL {
  public static main() {}
  
  public static _$init$_() {
    x = 33;
  }
  
  public static x = 33;
  
  public static f(): int {
    return x;
  }
  
  
  public static _$trigger_cctor$_() {}
  
}
```

- `DefaultParameterLowering`: handle functions with default parameters
```
class C {
  f(x: string = "oh") {}
}
```
converts into
```
class C {
  public f() {
    this.f("oh");
  }
  
  public f(x: string) {}
}
```

- `AmbientLowering` creates getter and setter for each indexer method in ambient classes (14.3.1 spec point), e.g.:
```
declare class A  {
  [index: number]: string  // indexer declaration
}
```
is converted to:
```
declare class A  {
  $_get(index: number): string
  $_set(index: number, value: string): void
}
```

- `ScopesInitPhase`: create `Scope` structures for all scopes; create `Variable` structures for every class name, function name and variable name in the program

- `OptionalLowering`: handle `a?.f` constructions
```
c?.f()
```
is replaced by
```
({let gensym$_1 = c;
  ((gensym$_1 == null) ? undefined : gensym$_1.f())})
```

- `PromiseVoidInferencePhase`: return `undefined` instead of void from async functions
```
class C {
  public async f() {
    return;
  }
}
```
converts to
```
class C {
  public async f() {
    return undefined;
  }
}
```

- `StructLowering`: something to do with struct definitions, but those are not supported by the parser yet

- `ExpressionLambdaConstruction`: handle lambdas with expression bodies
```
(): int => 33
```
is replaced with
```
(): int => { return 33 }
```

- `InterfacePropertyDeclarationPhase`: handle property declarations in interfaces.
```
interface I {
  v: int
}
```
is converted to
```
interface I {
  get v(): int
  set v(v: int)
}
```

- `EnumLoweringPhase`: create auxiliary classes for enums.
```
enum E {
  ONE = 0,
  TWO = 1
}

```
is converted to
```
enum E {
  ONE = 0,
  TWO = 1
}

class #E {
  protected ordinal: int;
  
  public static <cctor>() {}
  
  public constructor(ordinal: int) {
    this.ordinal = ordinal;
  }
  
  protected static NamesArray: String[] = ["ONE", "TWO"];
  
  protected static ValuesArray: int[] = [0, 1];
  
  protected static StringValuesArray: String[] = ["0", "1"];
  
  protected static ItemsArray: E[] = [E.ONE, E.TWO];
  
  protected static BoxedItemsArray: #E[] = [new #E(E.ONE as int), new #E(E.TWO as int)];
  
  public static getName(ordinal: E): String {
    return NamesArray[ordinal as int];
  }
  
  public static getValueOf(name: String): E throws {
    for (let i = 0;(i < NamesArray.length);++i) {
      if ((name == NamesArray[i])) {
        return i as E;
      }
    }
    throw new Exception(("No enum constant E." + name));
  }
  
  public static valueOf(e: E): int {
    return ValuesArray[e as int];
  }
  
  public static toString(ordinal: E): String {
    return StringValuesArray[ordinal as int];
  }
  
  public static values(): E[] {
    return ItemsArray;
  }
  
  public static fromInt(ordinal: int): E throws {
    if ((ordinal < ItemsArray.length)) {
      return ItemsArray[ordinal];
    }
    throw new Exception(("No enum constant in E with ordinal value " + ordinal));
  }
  
  public unbox(): E {
    return ItemsArray[this.ordinal];
  }
  
  public static boxedfromInt(ordinal: int): #E throws {
    if ((ordinal < BoxedItemsArray.length)) {
      return BoxedItemsArray[ordinal];
    }
    throw new Exception(("No enum constant in E with ordinal value " + ordinal));
  }
}
```

- `ResolveIdentifiers`: assign variables to all identifier usages.

- `CheckerPhase`: assign types to all nodes that need to have a type. Subsequent phases are started under the assumption that all 
nodes have appropriate type

- `EnumPostCheckLoweringPhase`: support access to enums

```
enum Commands {
  Open = "fopen",
  Close = "fclose"
}

function f(d: Commands): boolean {
   return d == Commands.Close
}
```
gets translated to
```
enum Commands {
  Open = "fopen",
  Close = "fclose"
}

function f(d: Commands): boolean {
    return (#Commands.toString(d) == #Commands.toString(Commands.Close));
}
```

- `SpreadConstructionPhase` deals with spread elements in array literals.
```
    let a = [1, 2, 3];
    let b = [-1, -2, ...a, -3, -4];
```
becomes
```
    let a = [1, 2, 3];
    let b = ({let length: int = ((4 + a.length) + 0);
    type typeOfTempArray = double;
    
    let tempArrayVar: typeOfTempArray[] = new typeOfTempArray[length];
    let newArrayIndex = 0;
    tempArrayVar[newArrayIndex] = -1
    newArrayIndex++
    tempArrayVar[newArrayIndex] = -2
    newArrayIndex++
    let elementOfSpread2: double;
    for (elementOfSpread2 of a) {
      tempArrayVar[newArrayIndex] = elementOfSpread2;
      newArrayIndex++;
    }
    tempArrayVar[newArrayIndex] = -3
    newArrayIndex++
    tempArrayVar[newArrayIndex] = -4
    newArrayIndex++
    tempArrayVar});
```

- `plugins-after-check`:  call compiler plugins that work after checking types, if any.
(This phase should come immediately after `CheckerPhase`, its current position is a bug)

- `BigIntLowering`: convert bigint constants in field initializer to constructor calls; convert strict equality for bigints to normal equality.

```
class C {
  public v = 1234567890n;
}

let b = (new C().v === 987654321987654321n);
```
is converted to 
```
class C {
  public v = new BigInt("1234567890");
}

let b = (new C().v == 987654321987654321n);
```

- `OpAssignmentLowering`: convert compound assignments to simple ones
```
f()[h()]++
```
gets converted to
```
 ({const gensym$_1 = f() as int[];
   const gensym$_2 = h() as int;
   const gensym$_3 = gensym$_1[gensym$_2] as int;
   gensym$_1[gensym$_2] = (gensym$_3 + 1) as int
   gensym$_3})
```

- `ConstStringToCharLowering`: convert single-char string constants to char constants where appropriate
``` 
public static f(c: char) {}
f("c")
```
is converted to
```
public static f(c: char) {}
f(99)
```

- `BoxingForLocals`: box local variables that are both
  + effectively non-constant (get modified somewhere)
  + captured in a lambda or local class  
```
function f() {
  let c = 33;
  let lam = (): int => {
    return c;
  };
  c = 44;
}
```
converts to
```
function f() {
  let c = new IntBox(33 as int);
  let lam = (): int => {
    return c.get() as int;
  };
  c.set(44 as int) as int;
}
```

- `LambdaObjectConversion`: convert lambdas into classes that implement functional interfaces.
```
class C {
  private p = 1;
  
  public f(): int {
    let ll = 2;
    let lam = () {
      return (this.p + ll);
    };
    return lam();
  }
}
```
converts to
```
class C {
  private p = 1;
  
  public f(): int {
    let ll = 2;
    let lam = new %%lambda-lambda_invoke-0(this, ll);
    return lam.invoke0();
  }
  
  public lambda_invoke-0(ll: int): int {
    return (this.p + ll);
  }
  
}

final class %%lambda-lambda_invoke-0 implements Function0<Int> {
  public $this: C;
  
  public ll: int;
  
  public constructor($this: C, ll: int) {
    this.$this = $this;
    this.ll = ll;
  }
  
  public invoke0(): Object|null|undefined {
    return this.$this.lambda_invoke-0(this.ll) as Object|null|undefined;
  }
  
  public $_invoke(): int {
    return this.$this.lambda_invoke-0(this.ll);
  }
}
```

- `RecordLowering`: support object literals for the utility class `Record`
```
    let x : Record<number, string> = {
        1:"hello",
        2:"hello2",
    };
```
converts to
```
	let x: Record<number, string> = ({let gensym$_2 = new Record<Double, String>();
      gensym$_2.set(1, "hello")
      gensym$_2.set(2, "hello2")
      gensym$_2});
```

- `ObjectIndexLowering`: convert object index access into getter and setter calls
In
```
class C {
  private x: int[] = [1, 3, 5];
  
  public $_get(ind: int): int {
    return this.x[ind];
  }
  
  public $_set(ind: int, val: int): void {
    this.x[ind] = val;
  }
}

function main() {
  let c = new C();
  c[1] = c[2];
}
```
the last line converts to
```
  c.$_set(1, c.$_get(2));
```

- `ObjectIteratorLowering`: replace `for of` construction involving iterators into explicit iterator calls.
```
class A {
    data: string[] = ['a', 'b', 'c'];
    $_iterator() {
      return new CIterator(this);
    }
}

class CIterator implements Iterator<string> {
    index = 0;
    base: A;
    constructor (base: A) {
        this.base = base;
    }
    next(): IteratorResult<string> {
        if (this.index >= this.base.data.length) {
            return {
                done: true,
                value: undefined
            }
        }
        return {
          done: this.index >= this.base.data.length,
          value: this.base.data[this.index++]
        }
    }
}

function main(): void {
    let res = "";
    let a = new A();
    for (let x of a) res += x;
}
```
the `for of` loop converts to
```
    let gensym$_4 = a.$_iterator();
    let gensym$_5 = gensym$_4.next();
    while (!gensym$_5.done) {
      let x = gensym$_5.value!;
      res = (res + x) as String;
      gensym$_5 = gensym$_4.next();
    }
```

- `UnionLowering` does two things: 
  1. processes field access to union 
  1. cast to primitive type

If all union constituents has some common field then we need to generate special class for it for correct
processing in runtime, e.g. for this code:

```
class A { n: int = 42 }
class B { n: int = 43 }
class C { n: int = 44 }
function foo(x: A|B|C) {
  x.n
}
```
We will generate special class union_field with field named n which type is int. In the bytecode will look
like this:

```
.union_field i32 n

.function void ETSGLOBAL.foo(std.core.Object a0) <static, access.function=public> {
  ets.ldobj.name a0, n
  return.void
}
```
Runtime will look at `union_field` inside handle of `ets.ldobj.name` instruction and will get the value of this filed
from it.

Note: ets.ldobj.name instruction generation happens at `CodeGen` stage when `ETSUnionType` is set to
`ir::MemberExpression`.

Union cast to primitive will be updated soon and will look like this:
```
function foo(x: string|int) {
  console.log(x as int)
}
```
will be transformed to:
```
function foo(x: string|int) {
  console.log((x as Numeric).intValue())
}
```
It will return the value of `x` by virtual call if `x` is an instance of `Numeric` type and will throw RTE in the other cases.

- `ExpandBracketsPhase`: support for floats used as dimensions for array constructors
```
  let v = 4.0
  let a = new int[v]
```
changes to
```
    let v = 4;
    let a = ({let gensym$_2: double = v;
    if (!isSafeInteger(gensym$_2)) {
      throw new TypeError("Fractional part of index expression should be zero.");
    }
    new int[gensym$_2 as int]});
```

- `LocalClassConstructionPhase`: support for local classes and captured variables in them.
```
  function f() {
    let v = 4;

    class C {
      m(): int {
        return v;
      }
    }
    
    let c = new C();
  }

```
is converted to (roughly)
```
  function f() {
    let v = 4;
 
    class C {
      m(): int {
        return this.field#0;
      }
      
      constructor(v: int) {
        this.field#0 = v;
      }
      
      field#0: int;
    }
	
    let c = new C(v);
   }
```

- `InterfaceObjectLiteralLowering` creates synthetic classes for object literals of interface type:
```
interface I {
  n: int
  s: string
}

let o: I = {n: 33, s: "ouch"}
```
where `I` by this stage (after InterfacePropwertyDeclarationPhase) looks like
```
interface I {
  public abstract n(n: int)
  
  public abstract n(): int
  public abstract n(n: int)
  public abstract s(s: string)
  
  public abstract s(): string
  public abstract s(s: string)
  
}
```
the initialization is converted to (approximately)
```
let o: I = {n: 33, s: "ouch"} as $anonymous_class$I

class $anonymous_class$I implements I {
  private _n: int;
  
  set n(n: int) {
    this._n = n;
  }
  
  get n(): int {
    return this._n;
  }
  
  private _s: String;
  
  set s(s: String) {
    this._s = s;
  }
  
  get s(): String {
    return this._s;
  }
  
  constructor() {}
}
```

- `ObjectLiteralLowering` transforms object literals. In
```
class C {
  n: int
  s: string
}

let o: C = {n: 33, s: "ouch"}
```
initializer is converted to 
```
let o: C = ({let gensym$_2: C = new C();
  gensym$_2.n = 33
  gensym$_2.s = "ouch"
  gensym$_2});
}
```

- `StringConstructorLowering` removes superfluous calls to string constructors
```
s = new string("const string");
```
becomes
```
s = "const string"
```

- `StringComparisonLowering` replaces comparison operators for strings with explicit calls to `String.compareTo()`
```
s > "test"
```
becomes
```
s.compareTo("test") > 0
```

- `AnnotationCopyLowering` and `AnnotationCopyPostLowering` handle copying of annotation properties. These phases ensure that annotation properties are properly copied and processed during the lowering phase. The phases work together to:
  1. Copy annotation properties from annotation declarations to their usages
  2. Process and validate annotation properties
  3. Ensure proper handling of annotation metadata in the generated code

- `PartialExportClassGen` generates declarations for `Partial<C>` where `C` is exported
```
export class C {
  n: int = 0
  s: string = ""
}
```
leads to generation of
```
class C$partial {
  public n: int | undefined = undefined;
  public s: string | undefined = undefined;
}
```

- `plugins-after-lowering` call compiler plugins that work at the end of the lowering process, if any.

- `ConvertPrimitiveCastMethodCall` replaces all primitive cast method calls with corresponding 'as' expressions.
NOTE: To be removed after delivering 'primitive types refactoring' patch

Before phase:

```
let d: double = 3.14
let i: int = d.toInt()
```

After phase:

```
let d: double = 3.14
let i: int = d as int
```
