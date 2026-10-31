import {useVideoConfig} from 'remotion';

/* 当前成片的画幅：竖屏（1080×1920）还是横屏（1920×1080）。
 * 组件据此决定排版，pick(竖屏取值, 横屏取值) 用来写两套数值 */
export const useFormat = () => {
  const {width, height} = useVideoConfig();
  const wide = width > height;
  const pick = <T>(tall: T, wideValue: T): T => (wide ? wideValue : tall);
  return {width, height, wide, pick};
};
