import {Config} from '@remotion/cli/config';

Config.setVideoImageFormat('jpeg');
Config.setJpegQuality(95);
Config.setCodec('h264');
Config.setCrf(16);
Config.setPixelFormat('yuv420p');
// 标准的 bt709 有限色域：默认输出是全色域 + 旧的 bt470bg 标记，上传 YouTube 等平台后颜色可能发灰或死黑
Config.setColorSpace('bt709');
Config.setOverwriteOutput(true);
// 用本机已装的 Chrome，不另外下载 Remotion 的无头浏览器
Config.setBrowserExecutable(process.env.REMOTION_CHROME ?? '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome');
