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
import router from './sdk/api/@system.router';

class L{
  enableAlertBeforeBackPage() {
    router.enableAlertBeforeBackPage({  //error
      message: 'Message Info',
      success: ()=> {
        console.log('success');
      },
      cancel: ()=> {
        console.log('cancel');
      }
    });
  }
}

class RouterParams {
  data1: string;

  constructor(str: string) {
    this.data1 = str;
  }
}

@Entry
@Component
struct RouterExample {

  aboutToAppear(): void {
    router.replaceNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    });

    router.replaceNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    }, (err) => {
    });

    router.replaceNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    }, router.RouterMode.Standard);

    router.replaceNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    }, router.RouterMode.Standard, (err) => {
    });

    router.pushNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    });

    router.pushNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    }, (err) => {
    })

    router.pushNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    }, router.RouterMode.Standard);

    router.pushNamedRoute({ // error
      name: 'myPage',
      params: new RouterParams('message')
    }, router.RouterMode.Standard, (err) => {
    })

    let obj = router.getParams(); // error
    router.hideAlertBeforeBackPage(); // error
    router.showAlertBeforeBackPage({ // error
      message: 'Message Info'
    });
    let options: Array<router.RouterState> = router.getStateByUrl('pages/index'); // error
    let options1: router.RouterState | undefined = router.getStateByIndex(1); // error
    let page = router.getState(); // error
    let length = router.getLength(); // error
    router.clear(); // error
    router.back({ url: 'pages/detail' }); // error
    router.back(1); // error
    router.back(1, { info: 'From Home' }); // error
    router.replaceUrl({ // error
      url: 'pages/detail',
      params: new RouterParams('message')
    });
    router.replaceUrl({ // error
      url: 'pages/detail',
      params: new RouterParams('message')
    }, (err) => {
    });
    router.replaceUrl({ // error
      url: 'pages/detail',
      params: new RouterParams('message')
    }, router.RouterMode.Standard);
    router.replaceUrl({ // error
      url: 'pages/detail',
      params: new RouterParams('message')
    }, router.RouterMode.Standard, (err) => {
    });

    router.pushUrl({ // error
      url: 'pages/routerpage2',
      params: {name: 'message', age: 1}
    });

    router.pushUrl({ // error
      url: 'pages/routerpage2',
      params: new RouterParams('message')
    }, (err) => {
    });

    router.pushUrl({ // error
      url: 'pages/routerpage2',
      params: new RouterParams('message')
    }, router.RouterMode.Standard);

    router.pushUrl({ // error
      url: 'pages/routerpage2',
      params: new RouterParams('message')
    }, router.RouterMode.Standard, (err) => {
    })
  }

  build() {
    Column() {
    }.width('100%')
  }
}