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

export class MathCordic  {
    static readonly AG_CONST : double = 0.6072529350;
    static readonly TARGET_ANGLE : double = 28.027;
    static readonly expected : double = 10362.570468755888;
    static readonly ANGLES : double[] = [MathCordic.fnFixed(45.0), MathCordic.fnFixed(26.565), MathCordic.fnFixed(14.0362), MathCordic.fnFixed(7.12502), MathCordic.fnFixed(3.57633), MathCordic.fnFixed(1.78991), MathCordic.fnFixed(0.895174), MathCordic.fnFixed(0.447614), MathCordic.fnFixed(0.223811), MathCordic.fnFixed(0.111906), MathCordic.fnFixed(0.055953), MathCordic.fnFixed(0.027977)];

    static fnFixed(x : double): double {
        return x * 65536.0;
    }

    static fnFloat(x : double): double {
        return x / 65536.0;
    }

    static fnDegToRad(x : double): double {
        return 0.017453 * x;
    }

    static cordicsincos(target : double): double {
        let x : double;
        let y : double;
        let targetAngle : double = MathCordic.fnFixed(target);
        let currAngle : double = 0;
        let step : int ;
        x = MathCordic.fnFixed(MathCordic.AG_CONST * cos(0));
        y = sin(0);
        for (step = 0; step < 12; step++) {
            let newX : double ;
            if (targetAngle > currAngle) {
                newX = x - (y.toInt() >> step);
                y = (x.toInt() >> step) + y;
                x = newX;
                currAngle += MathCordic.ANGLES[step];
            }
            else {
                newX = x + (y.toInt() >> step);
                y = -(x.toInt() >> step) + y;
                x = newX;
                currAngle -= MathCordic.ANGLES[step];
            }
        }
        return MathCordic.fnFloat(x) * MathCordic.fnFloat(y);
    }

    static cordic(runs : int): double {
        let total : double = 0;
        for (let i : int = 0; i < runs; i++) {
            total += MathCordic.cordicsincos(MathCordic.TARGET_ANGLE);
        }
        return total;
    }

    n : int;

    public  run(): void {
        this.n = 25000;
        let total : double = MathCordic.cordic(this.n);

        arktest.assertEQ(total, MathCordic.expected,  "Incorrect result");
    }
}

function main(): void {
  let a = new MathCordic;
  a.run();
}
