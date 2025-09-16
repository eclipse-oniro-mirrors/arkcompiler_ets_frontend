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
import {NavRouterInstance,NavRouter,NavRouterAttribute} from './sdk/api/nav_router';
import {NavRouteMode,NavRouterInterface,RouteInfo} from './sdk/api/nav_router';

@Entry
@Component
struct NavRouterExample {
  @State isActiveWLAN: boolean = false
  @State isActiveBluetooth: boolean = false

  build() {
    Navigation() {
      NavRouter() //error
        .mode(NavRouteMode.PUSH_WITH_RECREATE)  //error*3
      NavRouter({name:'',param:undefined}) //error *3
        .mode(NavRouteMode.PUSH_WITH_RECREATE)  //error*3
        .onStateChange((isActivated: boolean) => { //error
          console.log(NavRouteMode.REPLACE+'') //error*2
          const attr = new NavRouterAttribute(); //error
          attr.mode(NavRouteMode.PUSH); //error*3
        })
      NavRouterInstance.mode(NavRouteMode.REPLACE) //error*3
    }
  }
}
class Test implements NavRouterInterface {//error
  public (): NavRouterAttribute;//error
  public (value: RouteInfo): NavRouterAttribute;//error *2
  public (value?: RouteInfo): NavRouterAttribute|void {//error*2 
    const attribute: NavRouterAttribute|undefined = undefined;//error
  }
}
interface Demo extends NavRouterInterface{//error
}
