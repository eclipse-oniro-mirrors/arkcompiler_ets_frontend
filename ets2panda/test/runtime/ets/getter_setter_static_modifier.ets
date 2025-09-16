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

class Creature {
    public static legs: int = 1;
    public static brain: int = 2;

    public static set Legs(l: int) {
        Creature.legs = l;
    }

    public static set Brain(b: int) {
        Creature.brain = b;
    }
}

class Carry {
    public static jump: int = 100;
    public static move: int = 10;

    public static get Jump(): int {
        return Carry.jump;
    }

    public static get Move(): int {
        return Carry.move;
    }
}

class Glass {
    public static size: int = 1;
    public static transparent: int = 1;

    public static set Size(s: int) {
        Glass.size = s;
    }

    public static get Transparent(): int {
        return Glass.transparent;
    }

    public static get Size(): int {
        return Glass.size;
    }

    public static set Transparent(t: int) {
        Glass.transparent = t;
    }
}

function main(): void {
    Creature.Legs = 0;
    Creature.Brain = 3;
    const leg = Creature.legs;
    const brain = Creature.brain;

    arktest.assertEQ(leg, 0)
    arktest.assertEQ(brain, 3)

    const jump = Carry.Jump;
    const move = Carry.Move;

    arktest.assertEQ(jump, 100)
    arktest.assertEQ(move, 10)

    Glass.Size = 150;
    Glass.Transparent = 500;

    const size = Glass.Size;
    const transparent = Glass.Transparent;

    arktest.assertEQ(size, 150)
    arktest.assertEQ(Glass.Size, 150)
    arktest.assertEQ(transparent, 500)
    arktest.assertEQ(Glass.Transparent, 500)
}
