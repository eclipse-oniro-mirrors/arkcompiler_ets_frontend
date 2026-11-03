---
title: 为缺少`declare`的顶层声明添加`declare`修饰符
---

ArkTS-Sta要求，声明文件中的所有声明必须是ambient声明。因此，Declgen会对所有的顶层声明添加`declare`修饰符。

输入：

{{sample:input}}

输出：

{{sample:output}}
