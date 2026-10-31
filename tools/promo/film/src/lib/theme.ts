import type React from 'react';
import {Easing, interpolate, random} from 'remotion';

/* 中文标题用宋体粗体，英文用 Didot：和游戏的版画气质一致 */
export const FONT_CN = '"Songti SC", "STSong", serif';
export const FONT_EN = '"Didot", "Bodoni 72", "Baskerville", serif';
export const FONT_UI = '"Avenir Next Condensed", "Avenir Next", "PingFang SC", sans-serif';
/* 英文版的大标题（lib/fonts.ts 加载） */
export const FONT_TITLE = '"Cinzel", "Didot", serif';

/* 大标题的字形：中文版宋体特粗，英文版 Cinzel 粗体、略加字距 */
export const headline = (en: boolean): React.CSSProperties =>
  en
    ? {fontFamily: FONT_TITLE, fontWeight: 700, letterSpacing: '0.05em'}
    : {fontFamily: FONT_CN, fontWeight: 900};

export const COLORS = {
  night: '#05070b',
  nightBlue: '#0c1320',
  ice: '#cfe9ff',
  iceDeep: '#7fb8e6',
  paper: '#e9e5dc',
  /* 游戏面板的两种颜色：墨色与纸色 */
  gameInk: '#312f28',
  gamePaper: '#b1afa8',
  ember: '#ff8a3d',
};

export const EXPO_OUT = Easing.bezier(0.16, 1, 0.3, 1);
export const EXPO_IN = Easing.bezier(0.7, 0, 0.84, 0);
export const IN_OUT = Easing.bezier(0.65, 0, 0.35, 1);

/* 带缓动、两端钳住的插值：所有动画的基础 */
export const anim = (
  frame: number,
  input: [number, number],
  output: [number, number],
  easing: (t: number) => number = EXPO_OUT,
) =>
  interpolate(frame, input, output, {
    easing,
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
  });

/* 在 frame 处的冲击衰减：从 at 开始，length 帧内从 1 落到 0 */
export const pulse = (frame: number, at: number, length: number) =>
  frame < at ? 0 : Math.max(0, 1 - (frame - at) / length) ** 2;

/* 衰减的随机震屏（像素） */
export const shake = (frame: number, at: number, length: number, amount: number) => {
  const k = pulse(frame, at, length) * amount;
  return {x: (random(`sx${frame}`) - 0.5) * 2 * k, y: (random(`sy${frame}`) - 0.5) * 2 * k};
};
