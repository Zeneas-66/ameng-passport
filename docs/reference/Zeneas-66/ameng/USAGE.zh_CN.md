<p align="right">
  <strong>简体中文</strong> · <a href="USAGE.md">English</a>
</p>

# 阿猛 V1：构建、AI 配置与写入方法

## 按键

- 上 / 下：在 CALL、FEED、CHIN、BIRD、TALK 之间选择。
- OK：执行当前互动。
- 阿猛的状态保存在 NVS 中，普通重启后会继续。
- AI 是可选层；AI 未开启、未联网或请求失败时，TALK 会自动退回本地阿猛对白。

## 默认版本

因为当前仓库是公开仓库，默认配置刻意不保存任何密码或 API Key：

- 本地电子宠物逻辑：开启
- 状态持久化：开启
- AI 网络代码：已包含，但默认关闭
- Wi-Fi 密码 / API Key：不会写入 Git

使用 ESP-IDF 5.5.3 构建：

```bash
source <esp-idf-5.5.3>/export.sh
./tools/validate.sh
```

验证后的完整固件为：

```text
build/FoloToy-AI-Passport-full.bin
```

## 开启 AI 对话

执行：

```bash
idf.py menuconfig
```

进入 **Ameng companion**，开启 cloud AI dialogue，并填写：

- 2.4 GHz Wi-Fi 名称
- Wi-Fi 密码
- OpenAI-compatible Chat Completions 接口地址
- 模型名称
- API Key

这些信息只进入本地、被 Git 忽略的 `sdkconfig`，不要写进 `sdkconfig.defaults`，也不要提交到公开仓库。

然后重新运行：

```bash
./tools/validate.sh
```

目前屏幕使用 Passport 自带的 Montserrat 字体，因此 AI 被要求只输出很短的 ASCII 文本。这样可以避免中文缺字方框，同时减少 Flash 和 RAM 压力。

## 写入 AI Passport

完整刷新时，把验证后的 merged firmware 从 `0x0` 写入：

```bash
esptool.py --chip esp32c3 --port <PORT> write_flash 0x0 build/FoloToy-AI-Passport-full.bin
```

不要把 application-only 的 bin 写到 `0x0`。

完整 merged firmware 可能重置 NVS，也就是可能清掉阿猛已经积累的关系状态。以后迭代程序、又希望保留阿猛的数据时，应使用同一次验证构建产生的分段 `idf.py flash`，而不是每次都刷完整 merged image。

## 时间机制说明

当前基线硬件没有对应用暴露电池保持的实时时钟，所以阿猛使用持久化的“逻辑时间”：运行期间会正常推进，普通重启后会延续。但如果整机彻底断电，在没有联网校时或外置 RTC 的情况下，程序无法知道断电了多久。
