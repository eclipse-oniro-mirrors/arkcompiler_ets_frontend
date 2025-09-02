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

interface Composer {
    getCompositions(count: int = 100): string[]
    getIdentification(includeAge: boolean = true, prefix: string = "He is "): string
    getChildren(count: int, excludeLast: boolean = false): string[]
}

class JohannSebastianBach implements Composer {
    getCompositions(count: int = 8): string[] {
        let compositions: string[] = [
            "The Brandenburg Concertos",
            "The Goldberg Variations",
            "Concerto for Two Violins in D minor",
            "Air on the G String",
            "The Well-Tempered Clavier",
            "St Matthew Passion",
            "Toccata and Fugue in D minor",
            "Passacaglia and Fugue in C minor"
        ]

        let max: int = min(compositions.length, count) as int
        return [...compositions.slice(0, max)]
    }

    getIdentification(includeAge: boolean = true, prefix: string = "He is"): string {
        return `${prefix} Johann Sebastian Bach, the greatest composer that has ever lived${includeAge ? " who died at the age of 65." : "."}`;
    }

    getChildren(count: int, excludeLast: boolean = false): string[] {
        let children: string[] = [
            "Catharina Dorothea Bach",
            "Wilhelm Friedemann Bach",
            "Carl Philipp Emanuel Bach", 
            "Johann Gottfried Bernhard Bach", 
            "Gottfried Heinrich Bach",
            "Johann Christoph Friedrich Bach",
            "Johann Christian Bach III" 
        ]

        let max: int = min(children.length, count) as int
        return excludeLast ? [...children.slice(0, max - 1)] : [...children.slice(0, max)] 
    }
 }

function main() {
    let bach: JohannSebastianBach = {}
    let compositions_1: string[] = bach.getCompositions()
    let compositions_2: string[] = bach.getCompositions(5)
    let identification_1: string = bach.getIdentification()
    let identification_2: string = bach.getIdentification(false)
    let identification_3: string = bach.getIdentification(true, "He is the")
    let children_1: string[] = bach.getChildren(3)
    let children_2: string[] = bach.getChildren(7, true)

    arktest.assertEQ(compositions_1.length, 8)
    arktest.assertEQ(compositions_2.length, 5)
    arktest.assertEQ(compositions_1[3], "Air on the G String")
    arktest.assertEQ(compositions_2[4], "The Well-Tempered Clavier")
    arktest.assertEQ(identification_1, "He is Johann Sebastian Bach, the greatest composer that has ever lived who died at the age of 65.")
    arktest.assertEQ(identification_2, "He is Johann Sebastian Bach, the greatest composer that has ever lived.")
    arktest.assertEQ(identification_3, "He is the Johann Sebastian Bach, the greatest composer that has ever lived who died at the age of 65.")
    arktest.assertEQ(children_1.length, 3)
    arktest.assertEQ(children_2.length, 6)
    arktest.assertEQ(children_1[0], "Catharina Dorothea Bach")
    arktest.assertEQ(children_2[5], "Johann Christoph Friedrich Bach")
}