/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

@Entry
@Component
struct ContainerIndex {
  @State value: string = "parent";

  @Builder
  builder() {
    Text(this.value)
  }

  build() {
    Column() {
      MyContainer() {
        Text(this.value)
      }

      MyContainer({
        content: this.builder // arkui-buildparam-passing
      })

      MyContainer({
        content: () => {
          this.builder()
        }
      })

      MyContainer({
        content: this.builder.bind(this)
      })
    }
  }
}

@Component
struct MyContainer {
  @Require
  @BuilderParam content: () => void
  @State value: string = "child";

  build() {
    Row() {
      this.content();
    }
  }
}

@Entry
@Component
struct BuildParam1Index {
  @State value: string = "parent";

  @Builder
  builder() {
    Text(this.value)
  }

  build() {
    Column() {
      MyBuildParam1Container({
        content: this.builder // arkui-buildparam-passing
      })
    }
  }
}

@Component
struct MyBuildParam1Container {
  @BuilderParam content: () => void

  build() {
    Row() {
      this.content();
    }
  }
}

@Entry
@Component
struct BuildParam2Index {
  @State value: string = "parent";

  @LocalBuilder
  builder() {
    Text(this.value)
  }

  build() {
    Column() {
      MyBuildParam2Container({
        content: this.builder // arkui-buildparam-passing
      })
    }
  }
}

@Component
struct MyBuildParam2Container {
  @BuilderParam content: () => void

  build() {
    Row() {
      this.content();
    }
  }
}
