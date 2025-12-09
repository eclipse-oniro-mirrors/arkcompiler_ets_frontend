## 可选方法已弃用

**规则：** `sdk-optional-methods`

**规则解释：**

ArkTS1.2不支持类中的可选方法。

**变更原因：**

在ArkTS1.2中，类的方法由所有实例共享。增加可选方法会增加开发者判断空值的成本，从而影响性能。

**适配建议：**

用可选属性代替可选方法。

**示例：**

**ArkTS1.1**
```typescript
// ArkTS1.1API定义
interface NativeMediaPlayerBridge {
  resumePlayer?(): void
}

// ArkTS1.1应用代码
import NativeMediaPlayerBridge from 'xxx'
class MediaPlayer implements NativeMediaPlayerBridge {
  resumePlayer?(): void { // ArkTS1.2中不支持
  }
}
const player1 = new MediaPlayer();
```

**ArkTS1.2**
```typescript
// ArkTS1.2API定义
type ResumePlayerFn = () => void;
interface NativeMediaPlayerBridge {
    resumePlayer?: ResumePlayerFn;
}

// ArkTS1.2应用代码
class MediaPlayer implements NativeMediaPlayerBridge {
    resumePlayer?: ResumePlayerFn;  // 用可选属性代替
    constructor(resumePlayer?: ResumePlayerFn) {
        this.resumePlayer = resumePlayer;
    }
}
const player1 = new MediaPlayer(() => { });
```