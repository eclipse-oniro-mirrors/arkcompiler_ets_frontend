/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
// Benchmark: many functions in a single file to measure EmitProgram overhead.
// The relative-path caching optimization ensures TryRelativeFilePathViaArkTsPaths
// is called once per Program rather than once per function.  With many functions
// this reduction is measurable in the EmitProgram phase.

// ---------------------------------------------------------------------------------------------------------------------
// utils_numeric.ets
// ---------------------------------------------------------------------------------------------------------------------

class MathUtil {
    abs(v: int): int { if (v < 0) { return -v }; return v }
    min(a: int, b: int): int { if (a < b) { return a }; return b }
    max(a: int, b: int): int { if (a > b) { return a }; return b }
    clamp(v: int, lo: int, hi: int): int { return this.min(this.max(v, lo), hi) }
    square(v: int): int { return v * v }
    cube(v: int): int { return v * v * v }
    isEven(v: int): boolean { return (v & 1) == 0 }
    isOdd(v: int): boolean { return (v & 1) != 0 }
    factorial(n: int): int {
        let r: int = 1
        for (let i: int = 2; i <= n; i++) { r = r * i }
        return r
    }
    gcd(a: int, b: int): int {
        let x: int = a
        let y: int = b
        while (y != 0) { let t: int = y; y = x % y; x = t }
        return x
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// utils_string.ets
// ---------------------------------------------------------------------------------------------------------------------

class StringUtil {
    static repeat(s: string, n: int): string {
        let r: string = ""
        for (let i: int = 0; i < n; i++) { r = r + s }
        return r
    }
    static isEmpty(s: string): boolean { return s == "" }
    static firstChar(s: string): string {
        if (s == "") { return "" }
        return s[0].toString()
    }
    static concat3(a: string, b: string, c: string): string { return a + b + c }
}

// ---------------------------------------------------------------------------------------------------------------------
// models_point.ets
// ---------------------------------------------------------------------------------------------------------------------

class Point {
    x: double = 0.0
    y: double = 0.0
    constructor(x: double, y: double) { this.x = x; this.y = y }
    distance(other: Point): double {
        let dx: double = this.x - other.x
        let dy: double = this.y - other.y
        return sqrt(dx * dx + dy * dy)
    }
    midpoint(other: Point): Point { return new Point((this.x + other.x) / 2.0, (this.y + other.y) / 2.0) }
    origin(): double { return sqrt(this.x * this.x + this.y * this.y) }
    toString(): string { return "(" + this.x.toString() + ", " + this.y.toString() + ")" }
}

class Point3D extends Point {
    z: double = 0.0
    constructor(x: double, y: double, z: double) { super(x, y); this.z = z }
    distance3(p: Point3D): double {
        let dxy: double = this.distance(p)
        let dz: double = this.z - p.z
        return sqrt(dxy * dxy + dz * dz)
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// models_rectangle.ets
// ---------------------------------------------------------------------------------------------------------------------

class Rectangle {
    origin: Point = new Point(0, 0)
    width: double = 0
    height: double = 0
    constructor(o: Point, w: double, h: double) { this.origin = o; this.width = w; this.height = h }
    area(): double { return this.width * this.height }
    perimeter(): double { return 2.0 * (this.width + this.height) }
    contains(p: Point): boolean {
        return p.x >= this.origin.x && p.x <= this.origin.x + this.width &&
               p.y >= this.origin.y && p.y <= this.origin.y + this.height
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// collections_stack.ets
// ---------------------------------------------------------------------------------------------------------------------

class IntStack {
    private data: double[] = new double[0] (0.0)
    push(v: double): void { this.data[this.data.length] = v }
    pop(): double {
        let v: double = this.data[this.data.length - 1]
        this.data.length = this.data.length - 1
        return v
    }
    peek(): double { return this.data[this.data.length - 1] }
    isEmpty(): boolean { return this.data.length == 0 }
    size(): int { return this.data.length }
}

// ---------------------------------------------------------------------------------------------------------------------
// algorithms_sort.ets
// ---------------------------------------------------------------------------------------------------------------------

class SortUtil {
    static swap(data: double[], i: int, j: int): void {
        let t: double = data[i]
        data[i] = data[j]
        data[j] = t
    }
    static bubble(data: double[]): double[] {
        let n: int = data.length
        for (let i: int = 0; i < n; i++) {
            for (let j: int = 0; j < n - i - 1; j++) {
                if (data[j] > data[j + 1]) { SortUtil.swap(data, j, j + 1) }
            }
        }
        return data
    }
    static isSorted(data: double[]): boolean {
        for (let i: int = 1; i < data.length; i++) { if (data[i - 1] > data[i]) { return false } }
        return true
    }
    static minIdx(data: double[]): int {
        let idx: int = 0
        for (let i: int = 1; i < data.length; i++) { if (data[i] < data[idx]) { idx = i } }
        return idx
    }
    static sum(data: double[]): double {
        let s: double = 0.0
        for (let i: int = 0; i < data.length; i++) { s += data[i] }
        return s
    }
    static avg(data: double[]): double { return SortUtil.sum(data) / data.length.toDouble() }
}

// ---------------------------------------------------------------------------------------------------------------------
// algorithms_search.ets
// ---------------------------------------------------------------------------------------------------------------------

class SearchUtil {
    static linear(data: double[], target: double): int {
        for (let i: int = 0; i < data.length; i++) { if (data[i] == target) { return i } }
        return -1
    }
    static binary(data: double[], target: double): int {
        let lo: int = 0
        let hi: int = data.length - 1
        while (lo <= hi) {
            let mid: int = lo + (hi - lo) / 2
            if (data[mid] == target) { return mid }
            if (data[mid] < target) { lo = mid + 1 } else { hi = mid - 1 }
        }
        return -1
    }
    static count(data: double[], target: double): int {
        let c: int = 0
        for (let i: int = 0; i < data.length; i++) { if (data[i] == target) { c++ } }
        return c
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// utils_formatter.ets
// ---------------------------------------------------------------------------------------------------------------------

class FmtUtil {
    static padLeft(s: string, len: int, ch: string): string {
        let r: string = ""
        let pad: int = len - s.length
        for (let i: int = 0; i < pad; i++) { r = r + ch }
        return r + s
    }
    static ellipsis(s: string, maxLen: int): string {
        if (s.length <= maxLen) { return s }
        return s[0].toString() + StringUtil.repeat(".", 3)
    }
    static formatBool(v: boolean): string { if (v) { return "yes" }; return "no" }
    static formatRatio(a: double, b: double): string {
        if (b == 0.0) { return "inf" }
        let r: double = a / b
        return r.toString()
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// counter.ets
// ---------------------------------------------------------------------------------------------------------------------

class Counter {
    private val: int = 0
    increment(): void { this.val = this.val + 1 }
    decrement(): void { this.val = this.val - 1 }
    reset(): void { this.val = 0 }
    getValue(): int { return this.val }
    setValue(v: int): void { this.val = v }
    add(n: int): void { this.val = this.val + n }
    sub(n: int): void { this.val = this.val - n }
    mul(n: int): int { let r: int = this.val; this.val = r * n; return this.val }
}

// ---------------------------------------------------------------------------------------------------------------------
// collections_linkedlist.ets
// ---------------------------------------------------------------------------------------------------------------------

class IntNode {
    value: int = 0
    next: IntNode | null = null
    constructor(v: int) { this.value = v }
    setNext(n: IntNode | null): void { this.next = n }
    getNext(): IntNode | null { return this.next }
}

class IntLinkedList {
    private head: IntNode | null = null
    private count: int = 0
    add(v: int): void {
        let node = new IntNode(v)
        if (this.head == null) { this.head = node; this.count = 1; return }
        let cur = this.head!
        while (cur.getNext() != null) { cur = cur.getNext()! }
        cur.setNext(node)
        this.count = this.count + 1
    }
    remove(v: int): boolean {
        if (this.head == null) { return false }
        if (this.head!.value == v) { this.head = this.head!.getNext(); this.count = this.count - 1; return true }
        let cur = this.head!
        while (cur.getNext() != null) {
            if (cur.getNext()!.value == v) {
                cur.setNext(cur.getNext()!.getNext())
                this.count = this.count - 1
                return true
            }
            cur = cur.getNext()!
        }
        return false
    }
    find(v: int): boolean {
        let cur = this.head
        while (cur != null) { if (cur.value == v) { return true }; cur = cur.getNext() }
        return false
    }
    size(): int { return this.count }
}

// ---------------------------------------------------------------------------------------------------------------------
// set_util.ets
// ---------------------------------------------------------------------------------------------------------------------

class SetUtil {
    static contains(data: int[], target: int): boolean {
        for (let i: int = 0; i < data.length; i++) { if (data[i] == target) { return true } }
        return false
    }
    static union(a: int[], b: int[]): int {
        let c: int = 0
        for (let i: int = 0; i < a.length; i++) { c = c + a[i] }
        for (let i: int = 0; i < b.length; i++) { if (!SetUtil.contains(a, b[i])) { c = c + b[i] } }
        return c
    }
    static intersection(a: int[], b: int[]): int {
        let c: int = 0
        for (let i: int = 0; i < a.length; i++) { if (SetUtil.contains(b, a[i])) { c = c + 1 } }
        return c
    }
    static isSubset(sub: int[], sup: int[]): boolean {
        for (let i: int = 0; i < sub.length; i++) { if (!SetUtil.contains(sup, sub[i])) { return false } }
        return true
    }
    static difference(a: int[], b: int[]): int {
        let c: int = 0
        for (let i: int = 0; i < a.length; i++) { if (!SetUtil.contains(b, a[i])) { c = c + 1 } }
        return c
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// stats_util.ets
// ---------------------------------------------------------------------------------------------------------------------

class StatUtil {
    static min(data: double[]): double {
        let m: double = data[0]
        for (let i: int = 1; i < data.length; i++) { if (data[i] < m) { m = data[i] } }
        return m
    }
    static max(data: double[]): double {
        let m: double = data[0]
        for (let i: int = 1; i < data.length; i++) { if (data[i] > m) { m = data[i] } }
        return m
    }
    static range(data: double[]): double { return StatUtil.max(data) - StatUtil.min(data) }
    static mean(data: double[]): double { return SortUtil.sum(data) / data.length.toDouble() }
    static variance(data: double[]): double {
        let m: double = StatUtil.mean(data)
        let s: double = 0.0
        for (let i: int = 0; i < data.length; i++) { let d: double = data[i] - m; s = s + d * d }
        return s / data.length.toDouble()
    }
    static stdDev(data: double[]): double { return sqrt(StatUtil.variance(data)) }
    static median(data: double[]): double {
        let n: int = data.length
        if ((n & 1) != 0) { return data[n / 2] }
        return (data[n / 2 - 1] + data[n / 2]) / 2.0
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// trig_util.ets
// ---------------------------------------------------------------------------------------------------------------------

class TrigUtil {
    static hypot(a: double, b: double): double { return sqrt(a * a + b * b) }
    static dist(x1: double, y1: double, x2: double, y2: double): double {
        return TrigUtil.hypot(x2 - x1, y2 - y1)
    }
    static lerp(a: double, b: double, t: double): double { return a + (b - a) * t }
    static clamp(v: double, lo: double, hi: double): double {
        if (v < lo) { return lo }; if (v > hi) { return hi }; return v
    }
    static normalize(v: double, min: double, max: double): double {
        return (v - min) / (max - min)
    }
    static smoothstep(edge0: double, edge1: double, x: double): double {
        let t: double = TrigUtil.clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
        return t * t * (3.0 - 2.0 * t)
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// bit_util.ets
// ---------------------------------------------------------------------------------------------------------------------

class BitUtil {
    static bitAnd(a: int, b: int): int { return a & b }
    static bitOr(a: int, b: int): int { return a | b }
    static bitXor(a: int, b: int): int { return a ^ b }
    static bitNot(a: int): int { return ~a }
    static shiftLeft(a: int, n: int): int { return a << n }
    static shiftRight(a: int, n: int): int { return a >> n }
    static isPowerOfTwo(v: int): boolean { return v > 0 && (v & (v - 1)) == 0 }
    static popCount(v: int): int {
        let c: int = 0
        while (v != 0) { c = c + (v & 1); v = v >> 1 }
        return c
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// string_builder.ets
// ---------------------------------------------------------------------------------------------------------------------

class StrBuilder {
    private buf: string = ""
    append(s: string): StrBuilder { this.buf = this.buf + s; return this }
    prepend(s: string): StrBuilder { this.buf = s + this.buf; return this }
    clear(): void { this.buf = "" }
    length(): int { return this.buf.length }
    charAt(idx: int): string { return this.buf[idx].toString() }
    substring(start: int, end: int): string {
        let r: string = ""
        for (let i: int = start; i < end; i++) { r = r + this.buf[i].toString() }
        return r
    }
    contains(s: string): boolean {
        let n: int = this.buf.length
        let m: int = s.length
        for (let i: int = 0; i <= n - m; i++) {
            let match: boolean = true
            for (let j: int = 0; j < m; j++) { if (this.buf[i + j] != s[j]) { match = false; break } }
            if (match) { return true }
        }
        return false
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// color_util.ets
// ---------------------------------------------------------------------------------------------------------------------

class ColorUtil {
    static rgb(r: int, g: int, b: int): int { return (r << 16) | (g << 8) | b }
    static red(c: int): int { return (c >> 16) & 0xff }
    static green(c: int): int { return (c >> 8) & 0xff }
    static blue(c: int): int { return c & 0xff }
    static luminance(r: int, g: int, b: int): double {
        return 0.299 * r.toDouble() + 0.587 * g.toDouble() + 0.114 * b.toDouble()
    }
    static invert(c: int): int { return (~c) & 0xffffff }
    static blend(c1: int, c2: int, t: double): int {
        let r: int = (ColorUtil.red(c2).toDouble() * t + ColorUtil.red(c1).toDouble() * (1.0 - t)).toDouble().toInt()
        let g: int = (ColorUtil.green(c2).toDouble() * t + ColorUtil.green(c1).toDouble() * (1.0 - t)).toDouble().toInt()
        let b: int = (ColorUtil.blue(c2).toDouble() * t + ColorUtil.blue(c1).toDouble() * (1.0 - t)).toDouble().toInt()
        return ColorUtil.rgb(r, g, b)
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------------------------------------------------

function main(): void {
    // MathUtil
    let m = new MathUtil()
    arktest.assertEQ(m.abs(-5), 5)
    arktest.assertEQ(m.square(6), 36)
    arktest.assertEQ(m.cube(3), 27)
    arktest.assertTrue(m.isEven(8))
    arktest.assertEQ(m.factorial(5), 120)
    arktest.assertEQ(m.gcd(48, 18), 6)
    arktest.assertEQ(m.clamp(15, 0, 10), 10)

    // StringUtil
    arktest.assertEQ(StringUtil.repeat("x", 4), "xxxx")
    arktest.assertTrue(StringUtil.isEmpty(""))
    arktest.assertEQ(StringUtil.firstChar("hello"), "h")

    // Point
    let p1 = new Point(0, 0)
    let p2 = new Point(3, 4)
    arktest.assertEQ(Double.toInt(p1.distance(p2)), 5)
    let p3 = new Point3D(0, 0, 0)
    let p4 = new Point3D(1, 2, 2)
    arktest.assertEQ(Double.toInt(p3.distance3(p4)), 3)

    // Rectangle
    let r = new Rectangle(new Point(0, 0), 10, 20)
    arktest.assertEQ(r.area(), 200)
    arktest.assertTrue(r.contains(new Point(5, 5)))

    // IntStack
    let s = new IntStack()
    s.push(1.0)
    s.push(2.0)
    arktest.assertEQ(Double.toInt(s.pop()), 2)
    arktest.assertEQ(s.size(), 1)

    // SortUtil & SearchUtil
    let data: double[] = new double[3] (0.0)
    data[0] = 3.0; data[1] = 1.0; data[2] = 2.0
    SortUtil.bubble(data)
    arktest.assertTrue(SortUtil.isSorted(data))
    arktest.assertEQ(SearchUtil.binary(data, 2.0), 1)
    arktest.assertEQ(SearchUtil.linear(data, 3.0), 2)

    // FmtUtil
    arktest.assertEQ(FmtUtil.padLeft("42", 4, "0"), "0042")
    arktest.assertEQ(FmtUtil.formatBool(true), "yes")

    // Counter
    let ctr = new Counter()
    ctr.setValue(10)
    ctr.increment()
    arktest.assertEQ(ctr.getValue(), 11)
    ctr.add(5)
    arktest.assertEQ(ctr.getValue(), 16)
    ctr.reset()
    arktest.assertEQ(ctr.getValue(), 0)

    // IntLinkedList
    let ll = new IntLinkedList()
    ll.add(1)
    ll.add(2)
    ll.add(3)
    arktest.assertEQ(ll.size(), 3)
    arktest.assertTrue(ll.find(2))
    ll.remove(2)
    arktest.assertEQ(ll.size(), 2)
    arktest.assertTrue(!ll.find(2))

    // SetUtil
    let setData: int[] = [1, 2, 3, 4, 5]
    arktest.assertTrue(SetUtil.contains(setData, 3))
    arktest.assertEQ(SetUtil.intersection(setData, setData), 5)

    // StatUtil
    let statsData: double[] = new double[5] (0.0)
    statsData[0] = 1.0; statsData[1] = 2.0; statsData[2] = 3.0; statsData[3] = 4.0; statsData[4] = 5.0
    arktest.assertEQ(Double.toInt(StatUtil.min(statsData)), 1)
    arktest.assertEQ(Double.toInt(StatUtil.max(statsData)), 5)
    arktest.assertEQ(Double.toInt(StatUtil.range(statsData)), 4)
    arktest.assertEQ(Double.toInt(StatUtil.mean(statsData)), 3)

    // TrigUtil
    arktest.assertEQ(Double.toInt(TrigUtil.hypot(3.0, 4.0)), 5)
    arktest.assertEQ(Double.toInt(TrigUtil.lerp(0.0, 10.0, 0.5)), 5)

    // BitUtil
    arktest.assertEQ(BitUtil.bitAnd(0xf, 0x3), 3)
    arktest.assertEQ(BitUtil.bitOr(0x8, 0x1), 9)
    arktest.assertTrue(BitUtil.isPowerOfTwo(8))
    arktest.assertEQ(BitUtil.popCount(7), 3)

    // StrBuilder
    let sb = new StrBuilder()
    sb.append("hello").append(" ").append("world")
    arktest.assertEQ(sb.length(), 11)

    // ColorUtil
    let white = ColorUtil.rgb(255, 255, 255)
    arktest.assertEQ(ColorUtil.red(white), 255)
    arktest.assertEQ(ColorUtil.invert(0x000000), 0xffffff)
}
