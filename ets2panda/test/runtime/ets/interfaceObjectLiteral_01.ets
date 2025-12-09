/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

interface I {
  q?: string;

  bar () {
    return "Hey, ";
  };

  print_name1 (name: int): string { return "test"; };

  print_name2(name: string): string
  {
    return (name + ", I'm 'I'.");
  }

  print_name (name: string): int { return 0;} ;

  overload print_name { print_name2, print_name1, print_name }
}

let s = " Do you know me?";

let p: I = {
  q: s,
  print_name2 (name: string): string {
    return (this.bar() + name + c'!' + this.q);
  }
};

arktest.assertEQ(p.print_name("Bob"), "Hey, Bob! Do you know me?");

p = {
};

arktest.assertEQ(p.print_name("Bob"), "Bob, I'm 'I'.");

class C {
  a: string = "I said good bye ";
}

function main() {
  let x: string = "Bye, ";
  let c = new C();

  let p: I = {
    print_name2 (name: string): string {
      return (x + name + this.q);
    },
    q: "..."
  };

  arktest.assertEQ(p.print_name("Bob"), "Bye, Bob...");

  x = "Good bye, ";
  arktest.assertEQ(p.print_name("Bob"), "Good bye, Bob...");

  p = {
    print_name2 (name: string): string {
      return (c.a + name + c'!');
    }
  };

  arktest.assertEQ(p.print_name("Bob"), "I said good bye Bob!");
}
