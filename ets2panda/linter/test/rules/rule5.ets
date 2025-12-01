/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

// fixable
let bar1 = 1;
var foo1 = bar1;

let bar2 = 1;
let most2 = 1;
var foo2 = bar2, toast2 = most2;

let bar3 = 1;
let most3 = 1;
var foo3 = bar3; let toast3 = most3;

let b4 = [];
for (var a4 of b4) { console.log(a4); }

let b5 = [];
for (var a5 in b5) { console.log(a5); }

let b6 = [];
for (let a6 of b6) { var c6 = 1; console.log(c6); }

let list7 = [];
for (var i7 = 0; i7 < list7.length; ++i7) { foo(i7) }

var {a8, b8 = a8} = {} // not fixable

var a9 = b9; var b9 = 1

var foo10 = 1

declare var foo11 = 2;

// not fixable
var fx12 = function (i12 = 0) { if (i12 < 5) { return fx12(i12 + 1); } console.log(i12); }; fx12();

var foo13 = function () { foo13() };

var foo14 = () => foo14();

// fixable
var bar15 = foo15; var foo15 = function () { foo15(); };

var bar16 = function () { foo16(); }; var foo16 = function() {};

// not fixable
for (var i17 = 0, i17 = 0; false;);

var i18 = 0; for (var i18 = 1; false;); console.log(i18);

var a19, b19, c19; var a19;

let b20 = 1;
var a20; if (b20) { var a20; }

let foo21 = 1;
if (foo21) { var a21, b21, c21; } a21;

for (var i22 = 0; i22 < 10; ++i22) {} i22;

let obj23 = [];
for (var a23 in obj23) {} a23;

let list24 = [];
for (var a24 of list24) {} a24;

let a25 = 1;
switch (a25) { case 0: var b25 = 1 }

let b26 = [];
let arr26 = [];
for (var a26 of b26) { arr26.push(() => a26); }

let b27 = [];
for (let a27 of b27) { var c27; console.log(c27); c27 = 'hello'; }

var a28 = a28

var {a29 = a29} = {}

var {a30 = b30, b30} = {}

var a31 = b31, b31 = 1

function foo32() { a32 } var a32 = 1; foo32()

let foo33 = 1;
if (foo33) var bar33 = 1;

// var foo34 = 1

// { var foo35 = 1 }

if (true) { var foo36 = 1 }

function foo37() { var let; }

// how to test it, let keyword should stay let keyword
// function foo38() { var { let } = {}; }

var foo39 = (function () { foo39(); })();

let bar40 = function(a) {};
var foo40 = bar40(function () { foo40(); });

var bar41 = foo41, foo41 = function () { foo41(); };

var { foo42 = foo42 } = function () { foo42(); };

var { bar43 = foo43, foo43 } = function () { foo43(); };
