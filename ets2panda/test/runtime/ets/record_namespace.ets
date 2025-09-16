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

namespace Image{
    export interface PixelMapBase{
        field:int;
    }
    
    export class PixelMap{
        field:int;
        constructor(a:int){
            this.field = a;
        }
    }

    export enum AccessType{
        READ = 1,
        WRITE = 2,
        READWRITE = 3
    }

    export namespace PhotoAsset{
         export class PhotoAssetClass{
            field:int;
            constructor(a:int){
                this.field = a;
            }
        }
    }
}

function main(){
    let PixelMapBaseRecord:Record<string,Image.PixelMapBase> = {"a":{field:1},"b":{field:2}}
    arktest.assertEQ(PixelMapBaseRecord["a"]!.field,1)
    arktest.assertEQ(PixelMapBaseRecord["b"]!.field,2)
    PixelMapBaseRecord["a"] = {field:3};
    arktest.assertEQ(PixelMapBaseRecord["a"]!.field,3)
    arktest.assertEQ(PixelMapBaseRecord["b"]!.field,2)

    let PixelMapRecord:Record<string,Image.PixelMap> = {"a":new Image.PixelMap(1),"b":new Image.PixelMap(2)}
    arktest.assertEQ(PixelMapRecord["a"]!.field,1)
    arktest.assertEQ(PixelMapRecord["b"]!.field,2)
    PixelMapRecord["a"] = new Image.PixelMap(3);
    arktest.assertEQ(PixelMapRecord["a"]!.field,3)
    arktest.assertEQ(PixelMapRecord["b"]!.field,2)

    let AccessTypeRecord:Record<string,Image.AccessType> = {"a":Image.AccessType.READ,"b":Image.AccessType.WRITE}
    arktest.assertEQ(AccessTypeRecord["a"]!,Image.AccessType.READ)
    arktest.assertEQ(AccessTypeRecord["b"]!,Image.AccessType.WRITE)
    AccessTypeRecord["a"] = Image.AccessType.READWRITE;
    arktest.assertEQ(AccessTypeRecord["a"]!,Image.AccessType.READWRITE)
    arktest.assertEQ(AccessTypeRecord["b"]!,Image.AccessType.WRITE)

    let PhotoAssetRecord:Record<string,Image.PhotoAsset.PhotoAssetClass> = {"a":new Image.PhotoAsset.PhotoAssetClass(1),"b":new Image.PhotoAsset.PhotoAssetClass(2)}
    arktest.assertEQ(PhotoAssetRecord["a"]!.field,1)
    arktest.assertEQ(PhotoAssetRecord["b"]!.field,2)
    PhotoAssetRecord["a"] = new Image.PhotoAsset.PhotoAssetClass(3);
    arktest.assertEQ(PhotoAssetRecord["a"]!.field,3)
    arktest.assertEQ(PhotoAssetRecord["b"]!.field,2)
}
