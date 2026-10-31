import {continueRender, delayRender, staticFile} from 'remotion';

/* 英文版的大标题字体：Cinzel（罗马碑刻风格，SIL OFL 开源，见 public/fonts/Cinzel-OFL.txt）。
 * 字体文件随项目放在 public/fonts，渲染时不依赖网络；加载完成前 Remotion 不截帧 */
const handle = delayRender('Loading Cinzel');
const cinzel = new FontFace('Cinzel', `url(${staticFile('fonts/Cinzel.ttf')})`, {weight: '400 900'});
cinzel
  .load()
  .then(() => {
    document.fonts.add(cinzel);
    continueRender(handle);
  })
  .catch((err) => {
    console.error('Cinzel failed to load, falling back to Didot', err);
    continueRender(handle);
  });
