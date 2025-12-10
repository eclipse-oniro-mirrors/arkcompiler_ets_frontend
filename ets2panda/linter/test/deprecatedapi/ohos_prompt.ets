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

import prompt from '@ohos.prompt';

const toastOptions: prompt.ShowToastOptions = {
  message: 'Message Info', //error
  duration: 2000
};

prompt.showDialog(toastOptions)//error
  .then(data => {
    console.info('showDialog success, click button: ' + data.index);
  })
  .catch((err: Error) => {
    console.info('showDialog error: ' + err);
  })

const dialogOptions: prompt.ShowDialogOptions = { // error
  title: 'Title Info',
  message: 'Message Info'
};

prompt.showDialog(dialogOptions)// error
  .then(data => {
    console.info('showDialog success');
  })

prompt.showDialog({
  title: 'showDialog Title Info', // error
  message: 'Message Info',
  buttons: [
    {
      text: 'button1',
      color: '#000000'
    },
    {
      text: 'button2',
      color: '#000000'
    }
  ]
}, (err, data) => {
  if (err) {
    console.info('showDialog err: ' + err);
    return;
  }
  console.info('showDialog success callback, click button: ' + data.index); // error
});
prompt.showActionMenu({
  title: 'Title Info',
  buttons: [// error
    {
      text: 'button1',
      color: '#000000'
    },
    {
      text: 'button2',
      color: '#000000'
    }
  ],
})
  .then(data => {
    console.info('showActionMenu success, click button: ' + data.index);
  })
  .catch((err: Error) => {
    console.info('showActionMenu error: ' + err);
  })

const button: prompt.Button = {
  text: 'test',
  color: 'red'  // error
};

prompt.showDialog({
  title: 'Title Info',
  message: 'Message Info',
  buttons: [button],
})
  .then(data => {
    console.info('showDialog success, click button: ' + data.index);
  })
  .catch((err: Error) => {
    console.info('showDialog error: ' + err);
  })

prompt.showToast({ // error
  message: 'Message Info',
  duration: 2000
});

let response: Promise<prompt.ShowDialogSuccessResponse> = prompt.showDialog({ // error
  title: 'Title Info',
  message: 'Message Info',
  buttons: [
    {
      text: 'button1',
      color: '#000000'
    },
    {
      text: 'button2',
      color: '#000000'
    }
  ],
})
  .then(data => {
    console.info('showDialog success, click button: ' + data.index);
    return data;
  })
  .catch((err: Error) => {
    console.info('showDialog error: ' + err);
    throw err;
  });

prompt.showToast({ // error
  message: 'Message Info',
  duration: 2000
});

