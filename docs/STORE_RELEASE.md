# Mac App Store 与 Google Play 上架指南（Frostline Hold）

两家商店的路径相同：开发者账号与收款 → 用商店要求的方式签名打包 → 商店页素材与问卷 → 测试 → 审核。
定价 **0.99 美元**（其他地区按商店自动换算），首发**不含中国大陆**（App Store 中国区上架游戏需要版号；Google Play 在大陆不可用）。

仓库里已经就绪的部分：

| 项目 | 位置 |
| --- | --- |
| Mac App Store 构建（沙盒、签名 `.pkg`） | `platform/pc/scripts/package_mac.sh --mas` |
| Android 构建（签名 `.aab` / `.apk`） | `platform/pc/scripts/package_android.sh` |
| 商店截图、Play 宣传图 | `cd assets_src && python3 artgen/generate_store_art.py` → `dist/store/` |
| Play 512×512 图标 | `platform/pc/icon/itch_square_512.png` |
| 主页 / 支持页 | <https://hkgood.github.io/frostline_hold/> |
| 隐私政策 | <https://hkgood.github.io/frostline_hold/privacy.html> |

截图生成脚本读取 `tools/promo/capture.py` 录下的画面，先运行 `python3 tools/promo/capture.py` 和 `python3 tools/promo/capture.py --lang en`。

---

## Mac App Store

### 第一步：证书、App ID 与描述文件（一次性）

1. Xcode → Settings → Apple Accounts → rocky liu → Manage Certificates → 左下角 "+"，加两张证书：
   **Apple Distribution** 和 **Mac Installer Distribution**。
2. [Identifiers](https://developer.apple.com/account/resources/identifiers/list) → "+" → App IDs → App →
   Explicit Bundle ID：`com.hkgood.frostlinehold`，描述 `Frostline Hold`，不勾任何 Capability。
3. [Profiles](https://developer.apple.com/account/resources/profiles/list) → "+" → Distribution 下的
   **Mac App Store Connect** → 选上面的 App ID → 选 Apple Distribution 证书 → 命名 `Frostline Hold MAS` → 下载。
4. 把下载的文件放到 `~/.frostline/FrostlineHold_MAS.provisionprofile`（或用 `FROSTLINE_MAS_PROFILE` 指向它）。
   描述文件一年后过期，到时重新生成下载即可。

### 第二步：协议、税务和银行业务（一次性，收费必需）

在 [App Store Connect → 商务](https://appstoreconnect.apple.com/business)：

1. 签署 **付费 App 协议**（Paid Applications Agreement）。未生效前不能设价格、不能上架付费 App。
2. 税务：非美国个人填 **W-8BEN**。
3. 银行账户：填收款账户（个人名下的境内银行卡可以收美元电汇，以 Apple 页面支持的银行为准）。
4. 申请 [App Store 小型企业计划](https://developer.apple.com/app-store/small-business-program/)：年收入低于 100 万美元，佣金从 30% 降为 15%。
5. 欧盟《数字服务法》的**商家身份**声明：在欧盟销售付费 App 一般要申报为商家，地址、电话、邮箱会在欧盟商店页公开。

### 第三步：新建 App 记录

App Store Connect → App → "+" → 新建 App：

| 字段 | 填写 |
| --- | --- |
| 平台 | macOS |
| 名称 | `Frostline Hold: Winter TD`（`Frostline Hold` 已被占用；简体中文本地化：`霜线要塞`） |
| 主要语言 | 英语（美国），再添加"简体中文"本地化 |
| 套装 ID | `com.hkgood.frostlinehold` |
| SKU | `frostlinehold-mac` |
| 用户访问权限 | 完全访问权限 |

App 信息：

| 字段 | 填写 |
| --- | --- |
| 类别 | 游戏 → 策略；次要类别：游戏 → 休闲（可选） |
| 内容版权 | 不包含第三方内容 |
| 年龄分级 | 问卷里"卡通或幻想暴力"选"不常见/轻微"，其余全选"无"；结果以问卷给出的为准 |
| 隐私政策网址 | `https://hkgood.github.io/frostline_hold/privacy.html` |
| App 隐私 | "我们不从此 App 中收集数据"（Data Not Collected） |
| 价格 | 0.99 美元；销售范围：所有国家或地区，**取消中国大陆** |
| 出口合规 | 已在 `Info.plist` 声明 `ITSAppUsesNonExemptEncryption = NO`，上传后不再询问 |

版本信息（每个语言一份，文案见文末）：截图、宣传文本、描述、关键词、技术支持网址
`https://hkgood.github.io/frostline_hold/#support`、营销网址 `https://hkgood.github.io/frostline_hold/`、
版权 `2026 hkgood`。

截图：`dist/store/mac/en/` 与 `dist/store/mac/zh/`，2880×1800，每种语言最多 10 张，按文件名顺序上传。

审核备注（App Review Information → Notes）：

```text
No account or login is needed; the game runs fully offline. START on the
title screen opens the campaign map, and FIGHT starts the selected level.
Settings (music, sound, language, day/night theme) are under SETTINGS.
```

### 第四步：构建与上传

```sh
platform/pc/scripts/package_mac.sh --mas
# → dist/mac-mas/frostline-hold-<版本>-<构建号>-mas.pkg
```

用 Mac App Store 上的免费 App **Transporter** 登录开发者 Apple ID，把 `.pkg` 拖进去上传。
几分钟后构建出现在 App Store Connect → TestFlight，可以先在本机用 TestFlight 装一遍，
确认沙盒下游戏、存档正常，再在版本页选中这个构建，提交审核（一般 1–3 天）。

每次上传的构建号（`CFBundleVersion`）必须比上次大；脚本默认用 UTC 时间 `YYDDDHHMM`，不用手动管理。
沙盒版的存档在 `~/Library/Containers/com.hkgood.frostlinehold/` 下，和 itch / GitHub 版的存档互不相通。

---

## Google Play

### 第一步：账号与收款（一次性）

1. Play Console → 设置 → **付款资料**：建立商家账户（Merchant），填税务信息与收款银行账户。未设置之前不能定价。
2. 开发者账号页面的联系信息：有收入的账号，Google 会在商店页公开显示开发者地址。

### 第二步：创建应用

Play Console → 创建应用：

| 字段 | 填写 |
| --- | --- |
| 应用名称 | `Frostline Hold`（zh-CN 本地化：`霜线要塞`） |
| 默认语言 | English (United States) – en-US，再添加 中文（简体）– zh-CN |
| 应用或游戏 | 游戏 |
| 免费或付费 | **付费**（发布后付费可以改免费，免费**不能**再改付费） |

### 第三步：商店设置与问卷（"设置应用"清单）

| 项目 | 填写 |
| --- | --- |
| 隐私权政策 | `https://hkgood.github.io/frostline_hold/privacy.html` |
| 应用访问权限 | 所有功能均无需特殊访问权限 |
| 广告 | 不含广告 |
| 内容分级（IARC） | 类别选"游戏"；暴力相关问题按"幻想角色之间的暴力、无血腥"如实作答，其余选否 |
| 目标受众 | 建议选 13 岁及以上；若勾选 13 岁以下，要额外遵守"家庭"政策 |
| 数据安全 | 不收集、不分享任何用户数据；没有账号，无需账号删除网址 |
| 政府应用 / 金融功能 / 健康 | 否 |
| 类别 | 游戏 → 策略；标签：塔防、像素、离线、单人、奇幻 |
| 联系方式 | 邮箱 `rocky.hk@gmail.com`，网站 `https://hkgood.github.io/frostline_hold/` |
| 价格 | 0.99 美元（"价格"页设置默认价格后自动换算各国） |

商店页素材：

| 素材 | 文件 |
| --- | --- |
| 应用图标 512×512 | `platform/pc/icon/itch_square_512.png` |
| 置顶大图 1024×500 | `dist/store/android/feature_graphic.png` |
| 手机截图 1080×1920 | `dist/store/android/en/`、`dist/store/android/zh/`（2–8 张） |

### 第四步：构建、封闭测试、正式发布

```sh
platform/pc/scripts/package_android.sh --new-key   # 仅第一次：生成上传密钥，务必备份
platform/pc/scripts/package_android.sh             # → dist/android/*.aab
```

1. 第一次上传时选择加入 **Play 应用签名**（Play App Signing），上传 `.aab`。
2. 测试 → **封闭测试**：建一个测试轨道，用邮件列表或 Google 群组加入至少 **12 名**测试者，
   把"选择加入"链接发给他们。按当前政策，2023 年 11 月 13 日之后注册的个人账号，
   要有至少 12 名测试者**连续 14 天**参与封闭测试，才能申请正式版访问权限（以 Play Console 显示为准）。
3. 付费应用的测试者通常也要在 Play 上购买才能安装（可事后退款），提前告知测试者。
4. 14 天满后，在信息中心申请正式版访问权限，回答测试情况问卷；通过后把构建推到"正式版"轨道，提交审核。

每次上传的 `versionCode` 必须更大：它由 `platform/pc/CMakeLists.txt` 的
`project(frostline_hold_pc VERSION x.y.z)` 算出（0.1.1 → 101），发版前先升版本号。

---

## 商店文案

### 名称

| 商店 | English | 简体中文 |
| --- | --- | --- |
| App Store 名称（≤30） | Frostline Hold: Winter TD | 霜线要塞 |
| App Store 副标题（≤30） | Hold the Frozen Fortress | 双色版画风格的凛冬塔防 |
| Play 应用名称（≤30） | Frostline Hold | 霜线要塞 |

### App Store 宣传文本（≤170）

```text
22 levels, 7 towers with two upgrade paths each, a hero to summon and two bosses: a winter tower defense drawn like an old engraving. No ads, no in-app purchases.
```

```text
22 个关卡、7 种各有两条升级路线的防御塔、可召唤的英雄和两个首领：一款画得像老版画的凛冬塔防。没有广告，没有内购。
```

### App Store 关键词（≤100，逗号分隔）

```text
tower defense,td,strategy,pixel,retro,winter,castle,fantasy,offline,engraving,hero,boss,ice
```

```text
塔防,策略,像素,复古,冬天,冰雪,城堡,奇幻,单机,离线,版画,英雄,首领,守城
```

### Play 简短说明（≤80）

```text
Build towers, call in a hero, hold a frozen fortress. A two-tone winter TD.
```

```text
建塔、升级、召唤英雄，守住冰原要塞。双色版画风格的凛冬塔防。
```

### 完整描述（两家商店通用，≤4000）

```text
Hold a frozen fortress in a two-tone engraving style. Build towers, upgrade them, call in a hero, and stop wave after wave of monsters before they reach the gate.

THE CAMPAIGN
• 22 levels across 8 chapters, from Frostline Pass to the Rime Citadel
• A campaign map you drag like a long scroll; every map has its own living scenery
• Champions: tough, armoured elites that shrug off crowd control
• Two bosses: the Mine Wyrm and the Frost King

THE DEFENCES
• 7 towers: Guard, Frost, Siege, Storm, Rune, Barracks and Ember, each with two upgrade paths
• The Warden, a hero you summon onto the field, with a frost nova
• 15 kinds of enemies to learn

THE PACKAGE
• Hand-drawn two-tone engravings, with a day and a night theme
• An animated title screen, an ending, and separate title and boss themes
• English and Simplified Chinese
• Played with a single tap or click
• No ads, no in-app purchases, no account, fully offline
```

```text
用双色版画风格守住冰原上的要塞：建塔、升级、召唤英雄，在怪物冲到城门前挡住一波又一波的进攻。

战役
• 22 个关卡、8 个章节，从霜线山口一路打到冰封要塞
• 战役地图是一幅可以左右拖动的长卷，每张地图都有自己的场景动画
• 精英小首领：血厚甲硬，不受控制
• 两个首领：矿洞巨蠕和冰封之王

防御
• 7 种防御塔：箭塔、冰霜塔、投石塔、雷暴塔、符文塔、盾卫营、余烬塔，每种都有两条升级路线
• 英雄霜卫：召唤上场，释放冰霜新星
• 15 种敌人等你认识

完整体验
• 手绘双色版画风格，有白天和夜间两种主题
• 动态标题页、结局动画，独立的标题和首领配乐
• 支持简体中文和英文
• 单指点击或鼠标即可游玩
• 没有广告、没有内购、不需要账号，完全离线
```
