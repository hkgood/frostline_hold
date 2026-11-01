# Steam 上架指南（Frostline Hold）

面向中国大陆个人/小团队开发者，从注册 Steamworks 账号到把构建挂上 Steam 商店的完整路径。
文中 Valve 的规则均引自官方文档；涉及中国境内合规的部分来自公开法律评述，已在文中标明其性质。

---

## 总时间线

从付款到能正式发布，最快也要 **约 2.5–3 个月**，而且这个时间表有相当一部分是 Valve 规定的等待期，不可压缩。

| 阶段 | 时长 | 说明 |
| --- | --- | --- |
| 提交 Steam Direct 资料 → onboarding 完成 | 15–30 天 | 官方原文："This process takes between 15-30 days" |
| 税务信息第三方核验 | 最长再 +30 天 | 可与上面并行；核验完成前**不能修改**税务信息 |
| 付 App fee → 允许发布 | 21 天 / 30 天 | 官方两处口径不一致：Onboarding 文档写 21 天，Steam Direct 落地页写 30 天。**按 30 天规划** |
| 商店页 "Coming Soon" 公开可见 | ≥ 2 周 | Valve 强制要求，用于积累愿望单和试写宣传文案 |
| Valve 审核商店页 + 构建 | 1–5 天 | Valve 会**实际运行你的游戏**，检查是否正常、有无恶意行为 |
| 合计 | **约 2.5–3 个月** | 从今天开始算 |

---

## 第一步：注册 Steamworks 账号

入口是 [partner.steamgames.com/steamdirect](https://partner.steamgames.com/steamdirect)，页面底部有 "Sign In" 按钮，跳转到
`https://partner.steamgames.com/newpartner/?signup_type=1`。

> **登录提醒**：Steamworks 后台用的就是普通 Steam 账号体系。如果你的账号在网页登录时会被手机 App 的
> "新账号保护问答"拦住（从异常 IP 登录时触发），**先按那套问答的答法解决**，或者用电脑端 Steam 客户端
> 扫码登录后台。注册审核期间不要频繁换 IP、不要反复重试登录——每一次失败都在给账号加风控分。

注册需要你完成三件事，它们互相关联。

### 1. 身份信息（Company Identification）

Valve 要知道"你是谁、你在代表哪个法律主体签约"。填的是**法定身份**，不是网名：

- 法定姓名的名和姓，不要填别名或昵称。
- **Company Legal Name** 必须是拥有或有权在 Steam 上发行这个产品的法律主体，也正是签署 Steam 分销协议的主体。
- 公司形式（Company Form）如果你个人持有游戏版权，选 **Sole Proprietorship（个体独资）**，然后把 Company Name 填成你的法定姓名。
- 不要填 DBA、"friendly name" 或工作室名。

### 2. 收款银行信息（Payment information）

需要 routing number、bank account number、bank address 三项。**账户持有人姓名必须与上面填的法定身份姓名完全一致**——这是最常见的卡点。个人注册是允许的，但银行账户必须开在同一个名字下。

### 3. 税务信息（Tax information）

Valve 有一个简短的税务问卷，根据你的税务居民身份决定预扣税率：

- 美国公民 / 美国公司 → 填 **W-9** 形式的信息。
- 与美国有税收协定地位的国家（含中国大陆）→ 填 **W-8BEN** 形式的信息。

两条必须记住的约束：**提交后由第三方机构核验，可能耗时最长 30 天，且可能要求你补交材料；核验完成之前你无法再修改税务信息。** 所以第一次就填对，别靠边填边改。

### 4. 签电子文件 + 付 App Fee

流程会引导你电子签署 **保密协议（NDA）** 和 **Steam 分销协议**，然后付 Steam Direct Fee。

- **金额：每个产品 100 美元**（约合人民币 700 元上下，按实时汇率）。
- **支付方式**：你所在地区 Steam 支持的任何付款方式，但**不能使用 Steam 钱包余额**。
- **不可退款**，但当你的产品在 Steam 商店和内购累计产生 **1,000 美元 Adjusted Gross Revenue** 之后，会在你收到的付款中自动抵扣，抵扣金额在月度报表里单列一行。
- **一个容易忽略的细节**：Steam Direct Fee 的发票开给"付款时登录的那个个人账号"，不是公司主体。个人开发者的话，记得走一次报销流程。
- 支付成功后，会生成一个 **app credit**，它绑定在付款人的 Steam 账号上，只有付款人能激活。已有 partner 账号的，只有具备 Admin 权限的人能付这笔费。

---

## 第二步：拿到 App ID，搭建商店页

Onboarding 通过后你就能进 Steamworks 后台，创建第一个产品并拿到 **App ID**。商店页是后面所有事情的地基。

### 图形资产尺寸（2024 年 8 月新规）

Valve 在 2024 年 8 月上调了大部分胶囊图的尺寸，**旧尺寸不再接受**。必填项：

| 资产 | 尺寸 | 备注 |
| --- | --- | --- |
| Header Capsule | 920 × 430 | 必填 |
| Small Capsule | 462 × 174 | 必填 |
| Main Capsule | 1232 × 706 | 必填 |
| Vertical Capsule | 748 × 896 | 必填 |
| Screenshots | 最小 1920 × 1080，16:9 | 必填，多张 |
| Page Background | 1438 × 810 | 可选 |
| Shortcut Icon | 256 × 256 .ico 或 .png | 必填（社区/客户端图标） |
| App Icon | 184 × 184 .jpg | 必填（社区/客户端图标） |
| Library Capsule | 600 × 900 | 必填 |
| Library Hero | 3840 × 1240 .png | 必填 |
| Library Logo | 宽 1280 / 高 720 .png | 必填 |
| Library Header Capsule | 920 × 430 | 必填 |

Valve 提供了官方模板包（含图层、字体、安全区），在 Graphical Assets 页面的 "CURRENT TEMPLATES" 链接里。

### 商店页的其余必备项

- 简短描述、详细描述、系统要求（分 Windows / macOS）。
- **预告片**：YouTube 链接，或上传本地视频文件。
- **Content Survey（内容调查）**：分三部分——一般内容、成人内容、生成式 AI 内容。第一部分填完会生成若干区域性分级，用在商店页上；成人内容部分必须如实披露（包括游戏里访问不到、但打包在构建里的内容）；第三部分若你的游戏在开发中用了 AI 生成美术/音效/文本，或运行时接了 AI 服务，必须详细说明实现方式，实时生成还要说明你加了什么安全护栏。审查通过后，内容调查的部分答案就锁定了，要改得联系 Steam Support。
- **隐私政策 URL**：商店页必填项。
- 定价与销售区域。

---

## 第三步：中国开发者的两个硬约束

### 约束一：收款必须走跨境通道

Steam 不会直接给中国大陆的银行卡打美元。实务做法是在一家持牌跨境收款机构（Payoneer 派安盈 / 万里汇 WorldFirst / PingPong 等）申请一个**美元收款账号**，再把它的银行信息填进 Steamworks。

以万里汇的官方指南为例，路径是：申请收款账号时选择 Steam → 填开发者账号名称和收款币种 → 在"开发者账号管理"里点"收款账户详情"查看账户信息 → 登录 Steamworks 后台，进「用户与权限」→「公司详细资料」找到「银行详情」→ 点"更改支付信息"，选择银行账户所在国家/地区，把账户详情逐项填进去。万里汇的说明里有两个具体坑：**账户类型选"支票账户"**，**中间行信息留空**（他们没有中间商信息）；如果绑的是美元账户，"银行国际代码"要填账户详情上的 **ACH 路由号码**，不是 SWIFT。

三条要守住的红线：

1. **账户名称必须与 Steam 注册主体完全一致**。不一致会触发身份核验失败，延迟放款至少 14 天。
2. **不要频繁更换收款账户**，每次变更都会触发重新审核（7–14 天）。
3. 收到钱之后的**结汇申报是你自己的事**，不是 Steam 的事——境外软件服务收入需要自行申报。这一块建议找专业顾问，别照抄网上的税率数字。

### 约束二：版号——全球区不需要，蒸汽平台需要

这一段是本指南里唯一涉及法律判断的部分，来源是公开的游戏法律评述（游戏法务律师署名文章与专门的游戏法律站点），**不是 Valve 官方文件**。以下不构成法律意见，正式做法请让熟悉游戏法务的律师按你的实际运营方式出判断。

基本结论：

- **在 Steam 全球区上架游戏不需要版号。** 依据是《服务贸易具体承诺减让表》相关条款——Steam 是美国公司的平台，未进入中国市场，只为中国用户提供支付渠道和中文页面；中国玩家购买 Steam 上的视听服务产品，法律上是允许的。
- **中国玩家在 Steam 全球区购买，被定性为"跨境数字贸易"/个人自用的进口行为**，类似海淘。版号的强制要求，目前没有穿透适用到 Steam 全球区的境外开发者身上。
- **但"蒸汽平台"（Steam 中国，Valve 与完美世界合作，2021 年 2 月上线）是另一套体系**——在蒸汽平台上架仍然需要版号。别把两套账号/商店体系搞混。
- 实操上真正被盯上的不是"上架"，而是**营销行为**：在境内社交平台宣传推广，实质上是变相鼓动翻墙；提供境内服务器、内购、简体中文本地化，也会抬高风险敞口。行政处罚的属地管辖主要针对境内发生的行为，所以"不在境内做宣发"是关键分界线。
- 内容上有一条真正的红线：**地缘政治、国家主权、历史虚无主义类内容容忍度为零**，一旦被舆论倒逼，处置方式通常是技术封锁（商店页面甚至开发商连接被拦截），而不是诉讼。

对 Frostline Hold 而言，这是最有利的姿态：纯单机、离线、无服务器、无内购。**保持这个形态，不要上服务器，不要做境内宣发，不要在商店页承诺境内服务**，就是在最低风险区间里合法发行。

---

## 第四步：构建与上传

### Frostline 仓库里已经就绪的部分

| 位置 | 内容 |
| --- | --- |
| `platform/pc/src/pc_steam.cpp` / `.h` | Steamworks 封装：`SteamAPI_RestartAppIfNecessary` + `SteamAPI_InitEx`，`init` / `update` / `shutdown` 三段 |
| `platform/pc/CMakeLists.txt:149-179` | `FROSTLINE_STEAM` 开关；校验 `STEAMWORKS_SDK` 与 `FROSTLINE_STEAM_APPID`；自动拷贝 `libsteam_api.dylib` / `steam_api64.dll` / `libsteam_api.so`；开发构建生成 `steam_appid.txt`（打包时剔除） |
| `platform/pc/scripts/common.sh` | `steam_cmake_args()`：检查两个环境变量，产出 cmake 参数 |
| `platform/pc/scripts/package_mac.sh --steam` | macOS Steam 包；会移除 `steam_appid.txt` |
| `platform/pc/scripts/package_windows.sh --steam` | Windows Steam 包；会拷贝 `steam_api64.dll` |
| `platform/pc/scripts/upload_steam.sh` | 生成 `app_build.vdf` 和两个 depot vdf，用 `steamcmd +login` 上传，支持 `FROSTLINE_STEAM_BRANCH` 设置上线分支 |
| `platform/pc/icon/steam_community_184.jpg` | Steam App Icon（184×184），已符合要求 |
| `tools/promo/` | `capture.py` / `shots.py` / `film/` —— 出截图和预告片的工具链 |

Steamworks SDK 需要 **1.58 或更高**版本。

### 上传命令

```sh
export STEAMWORKS_SDK=/path/to/steamworks_sdk          # 含 public/steam/steam_api.h 的那一层
export FROSTLINE_STEAM_APPID=<App ID>
export FROSTLINE_STEAM_DEPOT_WIN=<Windows depot ID>   # 通常是 App ID + 1
export FROSTLINE_STEAM_DEPOT_MAC=<macOS depot ID>     # 在后台建好 depot 后才有
export STEAM_BUILD_USER=<有上传权的 Steam 账号>

platform/pc/scripts/package_mac.sh --steam
platform/pc/scripts/package_windows.sh --steam
platform/pc/scripts/upload_steam.sh "Frostline Hold <版本号>"
```

Depot 必须在 Steamworks 后台先创建（内容 → Depot → 添加），ID 才会确定。

> **一个会绊住人的点**：`steamcmd +login` **不支持**手机 App 的"一键批准"推送，它只接受 5 位令牌码或邮箱验证码。如果你的 Steam 账号还卡在前面说的那套新账号保护问答里，**steamcmd 上传会一起卡住**。先把账号登录理顺，再谈发版。

---

## 第五步：发布

1. 商店页和构建都填完后提交审核，Valve 会实际跑一遍你的游戏，1–5 天出结果。
2. 商店页至少以公开可见的 "Coming Soon" 形态存在 **两周**——这段时间用来收集愿望单，愿望单数直接决定首发销量和 Steam 的推荐权重。
3. 付款满 21–30 天后，把构建指定到默认分支（`FROSTLINE_STEAM_BRANCH`），商店页点 "Release"。

---

## Frostline 上架前待补清单

- [ ] Steamworks 账号（Steam Direct onboarding 通过，拿到 App ID）
- [ ] 跨境美元收款账号已开通，账户名与 Steam 主体一致
- [ ] W-8BEN 填完并等第三方核验通过
- [ ] Steam 商店页 capsule 全套（920×430 / 462×174 / 1232×706 / 748×896 / 600×900 / 3840×1240 / 1280×720）—— `generate_icon.py` 目前只产出 OS 图标和 itch 图，需要扩展
- [ ] Shortcut Icon 256×256
- [ ] 截图 ≥5 张，1920×1080 起，16:9
- [ ] 预告片（`tools/promo/` 出片）
- [ ] 中英文简短描述 + 详细描述 + 系统要求
- [ ] 隐私政策 URL（若面向国区，放在已备案域名下）
- [ ] Content Survey 三部分填完
- [ ] 定价与销售区域
- [ ] Steamworks 后台创建两个 Depot，拿到 Depot ID
- [ ] `steamcmd` 登录验证通过（用上传账号实测一次）

---

## 来源

**Valve 官方**
- [Steam Direct（注册落地页）](https://partner.steamgames.com/steamdirect)
- [Onboarding（身份 / 收款 / 税务 / 等待期）](https://partner.steamgames.com/doc/gettingstarted/onboarding)
- [Steam Direct Fee（App fee 与抵扣规则）](https://partner.steamgames.com/doc/gettingstarted/appfee)
- [Graphical Assets — Overview（2024-08 后的胶囊尺寸）](https://partner.steamgames.com/doc/store/assets)
- [Content Survey（内容调查三部分）](https://partner.steamgames.com/doc/gettingstarted/contentsurvey)
- [Steam Guard Mobile Authenticator（令牌 / 迁移 / 备份码）](https://help.steampowered.com/en/faqs/view/7EFD-3CAE-64D3-1C31)

**中国合规（第三方法律评述，非 Valve 官方）**
- [邵诗巍律师：无游戏版号上架 Steam 发行游戏违法吗？](https://view.inews.qq.com/k/20251030A02J8X00)
- [China Game Legal：在 Steam 勾选中国区是否违法？](https://www.chinagamelegal.com/zh/blog/Steam-Region-Locking-and-Chinese-Law)

**收款**
- [万里汇：Steam 账户绑定指南](https://www.worldfirst.com.cn/content/articles/developer-guide/ky9akes9)
