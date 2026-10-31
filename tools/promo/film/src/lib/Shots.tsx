import React from 'react';
import {AbsoluteFill, useCurrentFrame} from 'remotion';
import {clipIndex, Focus, FULL, GAME_SIZE, GameView, toBox} from './GameView';
import {useFormat} from './format';
import {anim, COLORS, EXPO_IN, FONT_EN, headline} from './theme';
import {useLang} from './lang';
import {clipUse} from './timeline';

/* 场景里某个镜头此刻的状态：本地帧、源帧序号、是否在播 */
export const useClip = (id: string) => {
  const use = clipUse(id);
  const frame = useCurrentFrame();
  const local = frame - use.at;
  return {use, local, index: clipIndex(use, local), active: local >= 0 && local < use.duration};
};

/* 游戏画面的“舞台”。
 * 竖屏：中间一块正方形。1-bit 画面放大超过 ~5 倍就糊成噪点，竖屏铺满只看得到 140 像素宽，
 *       所以不铺满；上方留给标题，下方留给字幕。
 * 横屏：整张铺满。铺满时能看到游戏完整的 480 像素宽，只放大 4 倍，依然清楚 */
export const STAGE_TOP = 470;

export type Box = {left: number; top: number; width: number; height: number};

/* 游戏顶部 40 像素是 HUD（生命、金币、波次），特写镜头的中心压低到不拍到它 */
const HUD_HEIGHT = 40;

/* 镜头里的 focus.size 按“正方形舞台的边长”来写；放进长方形的框时，
 * 让短边框住同样多的游戏内容，长边顺势看得更宽（最多看到整张 480） */
export const fitFocus = (focus: Focus, box: Box): Focus => ({
  ...focus,
  size: Math.min(GAME_SIZE, (focus.size * Math.max(box.width, box.height)) / Math.min(box.width, box.height)),
});

export const useStage = () => {
  const {wide, width, height} = useFormat();
  const box: Box = wide ? {left: 0, top: 0, width, height} : {left: 0, top: STAGE_TOP, width, height: width};
  /* 框里实际看得到的游戏纵向像素 */
  const visible = (size: number) => {
    const f = fitFocus({cx: 0, cy: 0, size}, box);
    return (f.size * box.height) / Math.max(box.width, box.height);
  };
  return {
    box,
    wide,
    /* 让特写镜头的中心低到拍不到 HUD */
    belowHud: (cy: number, size: number) => Math.max(cy, HUD_HEIGHT + visible(size) / 2),
    /* 游戏坐标 -> 整个画面的坐标（用于把冲击波、点击波纹叠在游戏里的某个点上） */
    point: (focus: Focus, gx: number, gy: number) => {
      const p = toBox(fitFocus(focus, box), box.width, box.height, gx, gy);
      return {x: box.left + p.x, y: box.top + p.y};
    },
  };
};

/* 舞台镜头：清晰的游戏画面放在 box 里（默认是舞台）；box 没盖满画面时，
 * 空出来的地方用同一画面放大、模糊、压暗后垫底 */
export const SquareShot: React.FC<{
  clip: string;
  index: number;
  focus?: Focus;
  box?: Box;
  dx?: number;
  dy?: number;
  zoom?: number;
  filter?: string;
  transform?: string;
  grade?: 'frost' | 'none';
}> = ({clip, index, focus = FULL, box, dx = 0, dy = 0, zoom = 1, filter, transform, grade = 'frost'}) => {
  const stage = useStage();
  const {width, height} = useFormat();
  const b = box ?? stage.box;
  const covers = b.left <= 0 && b.top <= 0 && b.width >= width && b.height >= height;
  return (
    <AbsoluteFill>
      {!covers && (
        <AbsoluteFill style={{filter: 'blur(30px) brightness(0.32)', transform: 'scale(1.2)'}}>
          <GameView clip={clip} index={index} width={width} height={height} focus={FULL} grade={grade} />
        </AbsoluteFill>
      )}
      <div
        style={{
          position: 'absolute',
          left: b.left + dx,
          top: b.top + dy,
          width: b.width,
          height: b.height,
          overflow: 'hidden',
          transform: transform ?? `scale(${zoom})`,
          filter,
          boxShadow: covers ? undefined : '0 30px 90px rgba(0,0,0,0.8), 0 0 60px rgba(120,180,240,0.18)',
        }}
      >
        <GameView clip={clip} index={index} width={b.width} height={b.height} focus={fitFocus(focus, b)} grade={grade} />
        {/* 舞台四周压一圈暗角，让边缘融进夜色 */}
        <div style={{position: 'absolute', inset: 0, boxShadow: `inset 0 0 ${covers ? 220 : 120}px rgba(5,9,16,0.75)`}} />
      </div>
    </AbsoluteFill>
  );
};

/* 角色名牌：一道冰色细线，上面小字，下面大字。
 * 中文版：英文名小字 + 中文名大字；英文版：身份小字（role，比如 FINAL BOSS）+ 英文名大字 */
export const NameCard: React.FC<{zh: string; en: string; role: string; start: number; exit?: number}> = ({
  zh, en, role, start, exit,
}) => {
  const frame = useCurrentFrame();
  const {pick} = useFormat();
  const {en: english} = useLang();
  const t = anim(frame, [start, start + 14], [0, 1]);
  const leave = exit === undefined ? 0 : anim(frame, [exit, exit + 6], [0, 1], EXPO_IN);
  return (
    <div style={{position: 'absolute', isolation: 'isolate', left: pick(80, 110), top: pick(230, 110), opacity: t * (1 - leave)}}>
      <Scrim width={english ? 700 : 520} height={180} />

      <div style={{width: 180 * t, height: 3, background: COLORS.ice, boxShadow: `0 0 12px ${COLORS.ice}`}} />
      <div
        style={{
          fontFamily: FONT_EN,
          fontSize: 28,
          letterSpacing: `${0.5 - 0.2 * t}em`,
          color: COLORS.ice,
          marginTop: 14,
          textShadow: '0 2px 10px rgba(0,0,0,0.9)',
        }}
      >
        {english ? role : en}
      </div>
      <div
        style={{
          ...headline(english),
          fontSize: english ? 76 : 92,
          color: COLORS.paper,
          whiteSpace: 'nowrap',
          transform: `translateX(${(1 - t) * -30}px)`,
          textShadow: '0 4px 24px rgba(0,0,0,0.95), 0 0 30px rgba(150,205,255,0.35)',
        }}
      >
        {english ? en.toUpperCase() : zh}
      </div>
    </div>
  );
};

/* 横屏里文字直接压在发白的游戏画面上，在文字背后垫一团柔和的暗色；竖屏的文字在暗区里，不需要。
 * 父元素要带 isolation: isolate，衬底才会留在文字后面 */
export const Scrim: React.FC<{width?: number; height?: number; opacity?: number}> = ({width = 820, height = 300, opacity = 1}) => {
  const {wide} = useFormat();
  if (!wide) return null;
  // width × height 是文字大致占的范围，暗色从它的中心向外留出 PAD 的余量再淡掉
  const PAD = 200;
  return (
    <div
      style={{
        position: 'absolute',
        left: -PAD,
        top: -PAD,
        width: width + 2 * PAD,
        height: height + 2 * PAD,
        zIndex: -1,
        opacity,
        background: 'radial-gradient(ellipse closest-side, rgba(3,6,12,0.9) 0%, rgba(3,6,12,0.84) 60%, rgba(3,6,12,0.4) 82%, rgba(3,6,12,0) 100%)',
      }}
    />
  );
};

/* 手指点击的波纹，告诉观众“这里点了一下” */
export const TapRipple: React.FC<{at: number; x: number; y: number}> = ({at, x, y}) => {
  const frame = useCurrentFrame();
  const t = (frame - at) / 16;
  if (t < 0 || t > 1) return null;
  const r = 30 + 90 * (1 - (1 - t) ** 3);
  return (
    <div
      style={{
        position: 'absolute',
        left: x - r,
        top: y - r,
        width: r * 2,
        height: r * 2,
        borderRadius: '50%',
        border: `${6 * (1 - t)}px solid rgba(235,248,255,${1 - t})`,
        background: `rgba(235,248,255,${0.25 * (1 - t)})`,
        boxShadow: `0 0 30px rgba(170,220,255,${1 - t})`,
      }}
    />
  );
};
