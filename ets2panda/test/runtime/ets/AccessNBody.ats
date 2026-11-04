/**
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

import { PI } from "std/math/consts";

// readonly expected : double = -1.3524862408537381;
export class Body  {
    readonly SOLAR_MASS : double = 4 * PI * PI;
     x : double ;
     y : double ;
     z : double ;
     vx : double ;
     vy : double ;
     vz : double ;
     mass : double ;
     public constructor(x : double, y : double, z : double, vx : double, vy : double, vz : double, mass : double) {
         this.x = x;
         this.y = y;
         this.z = z;
         this.vx = vx;
         this.vy = vy;
         this.vz = vz;
         this.mass = mass;
     }

     public offsetMomentum(px : double, py : double, pz : double): Body {
         this.vx = -px / this.SOLAR_MASS;
         this.vy = -py / this.SOLAR_MASS;
         this.vz = -pz / this.SOLAR_MASS;
         return this;
     }
}

class NBodySystem  {
     bodies : Body[] ;
     public constructor(bodies : Body[]) {
         this.bodies = bodies;
         let px : double = 0.0;
         let py : double = 0.0;
         let pz : double = 0.0;
         let size : int = Double.toInt(this.bodies.length);
         for (let i : int = 0; i < size; i++) {
             let b : Body = this.bodies[i];
             let m : double = b.mass;
             px += b.vx * m;
             py += b.vy * m;
             pz += b.vz * m;
         }
         this.bodies[0].offsetMomentum(px, py, pz);
     }

     public advance(dt : double): void {
         let dx : double ;
         let dy : double ;
         let dz : double ;
         let distance : double ;
         let mag : double ;
         let size : int = Double.toInt(this.bodies.length);
         for (let i : int = 0; i < size; i++) {
             let bodyi : Body = this.bodies[i];
             for (let j : int = i + 1; j < size; j++) {
                 let bodyj : Body = this.bodies[j];
                 dx = bodyi.x - bodyj.x;
                 dy = bodyi.y - bodyj.y;
                 dz = bodyi.z - bodyj.z;
                 distance = sqrt(dx * dx + dy * dy + dz * dz);
                 mag = dt / (distance * distance * distance);
                 bodyi.vx -= dx * bodyj.mass * mag;
                 bodyi.vy -= dy * bodyj.mass * mag;
                 bodyi.vz -= dz * bodyj.mass * mag;
                 bodyj.vx += dx * bodyi.mass * mag;
                 bodyj.vy += dy * bodyi.mass * mag;
                 bodyj.vz += dz * bodyi.mass * mag;
             }
         }
         for (let i : int = 0; i < size; i++) {
             let body : Body = this.bodies[i];
             body.x += dt * body.vx;
             body.y += dt * body.vy;
             body.z += dt * body.vz;
         }
     }
     public energy(): double {
         let dx : double ;
         let dy : double ;
         let dz : double ;
         let distance : double ;
         let e : double = 0.0;
         let size : int = Double.toInt(this.bodies.length);
         for (let i : int = 0; i < size; i++) {
             let bodyi : Body = this.bodies[i];
             e += 0.5 * bodyi.mass * (bodyi.vx * bodyi.vx + bodyi.vy * bodyi.vy + bodyi.vz * bodyi.vz);
             for (let j : int = i + 1; j < size; j++) {
                 let bodyj : Body = this.bodies[j];
                 dx = bodyi.x - bodyj.x;
                 dy = bodyi.y - bodyj.y;
                 dz = bodyi.z - bodyj.z;
                 distance = sqrt(dx * dx + dy * dy + dz * dz);
                 e -= (bodyi.mass * bodyj.mass) / distance;
             }
         }
         return e;
     }
 }

export class AccessNBody  {
    static readonly SOLAR_MASS : double = 4 * PI * PI;
    static readonly DAYS_PER_YEAR : double = 365.24;
    n1 : int = 3;
    n2 : int = 24;

    static jupiter(): Body {
        return new Body(4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01, 1.66007664274403694e-03 * AccessNBody.DAYS_PER_YEAR, 7.69901118419740425e-03 * AccessNBody.DAYS_PER_YEAR, -6.90460016972063023e-05 * AccessNBody.DAYS_PER_YEAR, 9.54791938424326609e-04 * AccessNBody.SOLAR_MASS);
    }
    static saturn(): Body {
        return new Body(8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01, -2.76742510726862411e-03 * AccessNBody.DAYS_PER_YEAR, 4.99852801234917238e-03 * AccessNBody.DAYS_PER_YEAR, 2.30417297573763929e-05 * AccessNBody.DAYS_PER_YEAR, 2.85885980666130812e-04 * AccessNBody.SOLAR_MASS);
    }
    static uranus(): Body {
        return new Body(1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01, 2.96460137564761618e-03 * AccessNBody.DAYS_PER_YEAR, 2.37847173959480950e-03 * AccessNBody.DAYS_PER_YEAR, -2.96589568540237556e-05 * AccessNBody.DAYS_PER_YEAR, 4.36624404335156298e-05 * AccessNBody.SOLAR_MASS);
    }
    static neptune(): Body {
        return new Body(1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01, 2.68067772490389322e-03 * AccessNBody.DAYS_PER_YEAR, 1.62824170038242295e-03 * AccessNBody.DAYS_PER_YEAR, -9.51592254519715870e-05 * AccessNBody.DAYS_PER_YEAR, 5.15138902046611451e-05 * AccessNBody.SOLAR_MASS);
    }
    static sun(): Body {
        return new Body(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, AccessNBody.SOLAR_MASS);
    }
    readonly expected : double = -1.3524862408537381;
    public run(): void {
        let ret : double = 0;
        for (let n : int = this.n1; n <= this.n2; n *= 2) {
            let bodies : NBodySystem = new NBodySystem([AccessNBody.sun(), AccessNBody.jupiter(), AccessNBody.saturn(), AccessNBody.uranus(), AccessNBody.neptune()]);
            let max : int = n * 100;
            ret += bodies.energy();
            for (let i : int = 0; i < max; i++) {
                bodies.advance(0.01);
            }
            ret += bodies.energy();
        }

        arktest.assertEQ(ret, this.expected,  "Incorrect result")
    }
}

function main(): void {
  let a = new AccessNBody;
  a.run();
}
