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

import router, {
  BackRouterOptions,
  DisableAlertBeforeBackPageOptions,
  EnableAlertBeforeBackPageOptions, RouterOptions, RouterState } from '@system.router';

class K {
  private router: RouterOptions = {
    uri: 'pages/routerpage2/routerpage2',
    params: { // error
      data1: 'message',
      data2: {
        data3: [123, 456, 789]
      }
    }
  }

  private options: BackRouterOptions = {
    uri: 'pages/detail/detail',  // error
    params: {    // error
      data1: 'message'
    }
  }

  getState() {
    let page: RouterState = router.getState(); // error
    console.log('current index = ' + page.index); // error
    console.log('current name = ' + page.name); // error
    console.log('current path = ' + page.path); // error
  }

  pushPage() {
    router.push(this.router);   // error
  }

  defaultBack() {
    router.back(this.options);   // error
  }
}

export default new K()

class L {
  private options: EnableAlertBeforeBackPageOptions = {
    message: 'Message Info',
    success: () => {
      console.log('success');
    },
    cancel: () => { // error
      console.log('cancel');
    },
    complete: () => { // error
      console.log('complete');
    }
  }

  enableAlertBeforeBackPage() {
    router.enableAlertBeforeBackPage(this.options);
  }
}

class M {
  enableAlertBeforeBackPage() {
    router.enableAlertBeforeBackPage({
      message: 'Message Info',
      success: () => { // error
        console.log('success');
      },
      cancel: () => {
        console.log('cancel');
      }
    });
  }
}

class Z {
  private options: DisableAlertBeforeBackPageOptions = {    // error
    success: () => {
      console.log('success');
    },
    cancel: () => {
      console.log('cancel');
    }
  }

  disableAlertBeforeBackPage() {
    router.disableAlertBeforeBackPage(this.options);
  }
}