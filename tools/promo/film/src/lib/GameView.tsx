import React from 'react';
import {Img, staticFile} from 'remotion';
import manifest from '../../public/clips/manifest.json';
import {ClipUse, srcAt} from './timeline';
import {ASSETS, Lang, useLang} from './lang';

/* 每一帧素材都是真实游戏的 480x480 画面（tools/promo/capture.py 录制） */
export const GAME_SIZE = 480;

type ClipInfo = {
  frames: number;
  sounds: {frame: number; name: string}[];
  marks: Record<string, unknown>;
  track: ([number, number] | null)[];
};
export const CLIPS = manifest as unknown as Record<string, ClipInfo>;

export const frameCount = (clip: string) => {
  const info = CLIPS[clip];
  if (!info) throw new Error(`clip ${clip} is missing; run npm run capture`);
  return info.frames;
};

/* 两种语言的录像只差界面文字，帧数、音效、首领轨迹完全一致，所以共用中文版的 manifest */
export const frameUrl = (clip: string, index: number, lang: Lang = 'zh') =>
  staticFile(`${ASSETS[lang].clips}/${clip}/${String(index).padStart(4, '0')}.png`);

/* 镜头在 local 帧显示的源帧序号（钳在素材范围内） */
export const clipIndex = (use: ClipUse, local: number) =>
  Math.min(frameCount(use.clip) - 1, Math.max(0, Math.floor(srcAt(use, local))));

/* 首领在某个源帧的位置（录制时记下的轨迹），没有首领时取最近的一帧 */
export const bossAt = (clip: string, index: number): [number, number] => {
  const track = CLIPS[clip].track;
  for (let d = 0; d < track.length; d++) {
    const a = track[index - d] ?? null;
    if (a) return a;
    const b = track[index + d] ?? null;
    if (b) return b;
  }
  return [240, 240];
};

/* 取景：中心点与长边覆盖多少游戏像素 */
export type Focus = {cx: number; cy: number; size: number};
export const FULL: Focus = {cx: 240, cy: 240, size: GAME_SIZE};

export const lerpFocus = (a: Focus, b: Focus, t: number): Focus => ({
  cx: a.cx + (b.cx - a.cx) * t,
  cy: a.cy + (b.cy - a.cy) * t,
  size: a.size + (b.size - a.size) * t,
});

/* 把 focus 放进 width x height 的框里：缩放比例和左上角偏移，始终铺满不留边 */
export const layout = (focus: Focus, width: number, height: number) => {
  const scale = Math.max(width, height) / focus.size;
  const span = GAME_SIZE * scale;
  const clamp = (v: number, lo: number) => Math.min(0, Math.max(lo, v));
  return {
    scale,
    span,
    left: clamp(width / 2 - focus.cx * scale, width - span),
    top: clamp(height / 2 - focus.cy * scale, height - span),
  };
};

/* 游戏坐标 -> 框内坐标，用来把特效叠在游戏里的某个点上 */
export const toBox = (focus: Focus, width: number, height: number, gx: number, gy: number) => {
  const l = layout(focus, width, height);
  return {x: l.left + gx * l.scale, y: l.top + gy * l.scale};
};

type Props = {
  clip: string;
  index: number;
  width: number;
  height: number;
  focus?: Focus;
  /* 全屏镜头用冷色调；掌机里的画面保持游戏原色 */
  grade?: 'frost' | 'none';
  style?: React.CSSProperties;
};

/* 一帧游戏画面，按 focus 裁切；最近邻放大，版画的颗粒保持锐利 */
export const GameView: React.FC<Props> = ({clip, index, width, height, focus = FULL, grade = 'frost', style}) => {
  const l = layout(focus, width, height);
  const {lang} = useLang();
  return (
    <div style={{position: 'relative', width, height, overflow: 'hidden', ...style}}>
      <Img
        src={frameUrl(clip, index, lang)}
        style={{
          position: 'absolute',
          left: l.left,
          top: l.top,
          width: l.span,
          height: l.span,
          imageRendering: 'pixelated',
          filter: grade === 'frost' ? 'url(#frost-grade)' : undefined,
        }}
      />
    </div>
  );
};

/* 游戏墨色/纸色 -> 夜蓝/冰白 的双色调，在 Promo 里挂一次 */
export const FrostGradeFilter: React.FC = () => {
  const ink = [49, 47, 40];
  const paper = [177, 175, 168];
  const toInk = [16, 21, 31];
  const toPaper = [214, 223, 229];
  const span = (paper[0] - ink[0]) / 255;
  const rows = [0, 1, 2].map((c) => {
    const m = (toPaper[c] - toInk[c]) / 255 / span;
    const b = toInk[c] / 255 - m * (ink[0] / 255);
    return `${m} 0 0 0 ${b}`;
  });
  return (
    <svg width={0} height={0} style={{position: 'absolute'}}>
      <filter id="frost-grade" colorInterpolationFilters="sRGB">
        <feColorMatrix type="matrix" values={`${rows.join(' ')} 0 0 0 1 0`} />
      </filter>
    </svg>
  );
};
