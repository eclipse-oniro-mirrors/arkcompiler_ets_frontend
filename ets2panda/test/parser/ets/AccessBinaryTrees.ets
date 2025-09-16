/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class TreeNode {
  private left: TreeNode | null;
  private right: TreeNode | null;
  private item: int;

  constructor(left: TreeNode | null, right: TreeNode | null, item: int) {
    this.left = left;
    this.right = right;
    this.item = item;
  }

  public itemCheck(): int {
    if (this.left == null)
      return this.item;
    else
      return this.item + this.left!.itemCheck() - this.right!.itemCheck();
  }
}

export class AccessBinaryTrees {
  static readonly startDepth = 4;
  static readonly endDepth = 7;
  static readonly expected = -4;

  static bottomUpTree(item: int, depth: int): TreeNode {
   if (depth > 0) {
     return new TreeNode(
       AccessBinaryTrees.bottomUpTree(2*item - 1, depth-1),
       AccessBinaryTrees.bottomUpTree(2*item, depth-1),
       item
     );
   }
   return new TreeNode(null, null, item);
  }

  public run(): void {
    let ret: int = 0;

    for (let n: int = AccessBinaryTrees.startDepth; n <= AccessBinaryTrees.endDepth; n++) {
      let minDepth: int = AccessBinaryTrees.startDepth;
      let maxDepth: int = max(minDepth + 2, n);
      let stretchDepth: int = maxDepth + 1;
      let check: int = AccessBinaryTrees.bottomUpTree(0, stretchDepth).itemCheck();

      let longLivedTree = AccessBinaryTrees.bottomUpTree(0, maxDepth);

      for (let depth = minDepth; depth <= maxDepth; depth += 2) {
        let iterations: int = 1 << (maxDepth - depth + minDepth);

        check = 0;
        for (let i: int = 1; i <= iterations; i++) {
          check += AccessBinaryTrees.bottomUpTree(i, depth).itemCheck();
          check += AccessBinaryTrees.bottomUpTree(-i, depth).itemCheck();
        }
      }

      ret += longLivedTree.itemCheck();
    }

    arktest.assertEQ(ret, AccessBinaryTrees.expected,  "Incorrect result");
  }
}

function main(): void {
  let a = new AccessBinaryTrees;
  a.run();
}
