import React from 'react';
import {AbsoluteFill, staticFile, useCurrentFrame} from 'remotion';
import sprites from '../../public/sprites/sprites.json';
import {Flash} from '../lib/Fx';
import {useClip} from '../lib/Shots';
import {FULL, GameView} from '../lib/GameView';
import {anim, COLORS, EXPO_IN, EXPO_OUT, FONT_CN, FONT_EN, FONT_UI, headline} from '../lib/theme';
import {Callout} from '../lib/Type';
import {useFormat} from '../lib/format';
import {useLang} from '../lib/lang';

type SpriteInfo = {w: number; h: number; frames: number};
const SHEETS = sprites as Record<string, SpriteInfo>;

/* 播放导出的横向精灵条：按帧切换 background-position，最近邻放大 */
export const Sprite: React.FC<{name: string; scale: number; frame: number; fps?: number}> = ({name, scale, frame, fps = 5}) => {
  const {w, h, frames} = SHEETS[name];
  const cell = Math.floor(frame / fps) % frames;
  return (
    <div
      style={{
        width: w * scale,
        height: h * scale,
        backgroundImage: `url(${staticFile(`sprites/${name}.png`)})`,
        backgroundSize: `${w * frames * scale}px ${h * scale}px`,
        backgroundPosition: `${-cell * w * scale}px 0`,
        imageRendering: 'pixelated',
        filter: 'drop-shadow(0 0 1px #f2eee6) drop-shadow(0 0 1px #f2eee6) drop-shadow(0 6px 10px rgba(0,0,0,0.6))',
      }}
    />
  );
};

/* 十二种普通敌人 + 三个大块头：[精灵, 中文名, 中文版的英文小字, 英文版用的游戏英文名（strings.py）] */
const COMMON = [
  ['wolf', '雪狼', 'Wolf', 'Grey Wolf'],
  ['raider', '劫掠者', 'Raider', 'Raider'],
  ['skitter', '冰甲虫', 'Skitter', 'Ice Beetle'],
  ['husk', '铁甲蛮兵', 'Husk', 'Husk'],
  ['shaman', '霜萨满', 'Shaman', 'Frost Shaman'],
  ['banner', '旗手', 'Bannerman', 'Standard Bearer'],
  ['sap', '冰泥怪', 'Sap', 'Frost Slime'],
  ['crow', '冰鸦', 'Crow', 'Frost Crow'],
  ['golem', '石傀儡', 'Golem', 'Stone Golem'],
  ['slime', '冰史莱姆', 'Slime', 'Great Slime'],
  ['slimelet', '小史莱姆', 'Slimelet', 'Slimelet'],
  ['burrower', '掘地虫', 'Burrower', 'Burrower'],
];
/* 大块头额外带一个角标：[..., 中文角标, 英文角标] */
const GIANTS = [
  ['titan', '冰霜泰坦', 'Titan', 'Ice Titan', '巨像', 'Giant'],
  ['wyrm', '矿洞巨蠕', 'Mine Wyrm', 'Mine Wyrm', '首领', 'Boss'],
  ['king', '冰封之王', 'Frost King', 'Frost King', '首领', 'Boss'],
];

/* 每张牌的位置、尺寸和精灵放大倍数 */
type Slot = {x: number; y: number; w: number; h: number; scale: number};

/* 普通敌人的精灵有 40 和 48 两种格子，放大倍数各取一档，让画面里的个头接近 */
const commonScale = (key: string, small: number, large: number) => (SHEETS[`enemy_${key}`].w > 40 ? large : small);

/* 竖屏：标题下面 3 列 × 4 行普通敌人，最底下一排三张大牌 */
const tallLayout = () => {
  const cols = 3, w = 304, h = 236, gap = 22, top = 480;
  const left = (1080 - cols * w - (cols - 1) * gap) / 2;
  const giantTop = top + 4 * (h + gap) + 16;
  return {
    common: (i: number, key: string): Slot => ({
      x: left + (i % cols) * (w + gap), y: top + Math.floor(i / cols) * (h + gap), w, h, scale: commonScale(key, 3.4, 2.9),
    }),
    giant: (i: number): Slot => ({x: left + i * (w + gap), y: giantTop, w, h: 330, scale: 2.9}),
  };
};

/* 横屏：左边 4 列 × 3 行普通敌人，右边三张竖长的大牌，标题在左上 */
const wideLayout = () => {
  const cols = 4, w = 230, h = 190, gap = 16, top = 340, left = 110;
  const giantLeft = 1140, giantW = 210, giantGap = 20;
  return {
    common: (i: number, key: string): Slot => ({
      x: left + (i % cols) * (w + gap), y: top + Math.floor(i / cols) * (h + gap), w, h, scale: commonScale(key, 2.6, 2.2),
    }),
    giant: (i: number): Slot => ({x: giantLeft + i * (giantW + giantGap), y: top, w: giantW, h: 3 * h + 2 * gap, scale: 2.5}),
  };
};

const Card: React.FC<{
  x: number;
  y: number;
  w: number;
  h: number;
  appear: number;
  sprite: string;
  scale: number;
  zh: string;
  en: string;
  /* 英文版显示的游戏英文名 */
  name: string;
  tag?: string;
  /* 首领的角标用实心底色 */
  boss?: boolean;
  big?: boolean;
}> = ({x, y, w, h, appear, sprite, scale, zh, en, name, tag, boss, big}) => {
  const frame = useCurrentFrame();
  const {en: english} = useLang();
  // 英文名长短差很多（HUSK / STANDARD BEARER），按卡片宽度收字号，Cinzel 大写字母约 0.8em 宽
  const nameSize = Math.min(big ? 30 : 27, (w - 24) / (name.length * 0.8));
  const t = anim(frame, [appear, appear + (big ? 12 : 9)], [0, 1], EXPO_OUT);
  const glint = Math.max(0, 1 - Math.abs(frame - appear - 6) / 6);
  return (
    <div
      style={{
        position: 'absolute',
        left: x,
        top: y,
        width: w,
        height: h,
        opacity: t,
        transform: `perspective(900px) rotateY(${(1 - t) * 80}deg) scale(${(big ? 1.25 : 1.1) - (big ? 0.25 : 0.1) * t})`,
        background: 'linear-gradient(180deg, rgba(14,24,38,0.86), rgba(6,10,18,0.92))',
        border: `2px solid rgba(190,225,255,${0.25 + 0.6 * glint})`,
        boxShadow: `0 10px 30px rgba(0,0,0,0.6), 0 0 ${30 * glint}px rgba(170,220,255,${glint})`,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'flex-end',
        paddingBottom: 14,
        overflow: 'hidden',
      }}
    >
      {tag && (
        <div
          style={{
            position: 'absolute',
            top: 14,
            left: 14,
            ...(english
              ? {fontFamily: FONT_UI, fontWeight: 700, fontSize: 22, letterSpacing: '0.2em', textTransform: 'uppercase' as const}
              : {fontFamily: FONT_CN, fontWeight: 900, fontSize: 26}),
            color: boss ? COLORS.night : COLORS.ice,
            background: boss ? COLORS.ice : 'transparent',
            border: `2px solid ${COLORS.ice}`,
            padding: '2px 10px',
          }}
        >
          {tag}
        </div>
      )}
      <div style={{flex: 1, display: 'flex', alignItems: 'center', justifyContent: 'center'}}>
        <Sprite name={`enemy_${sprite}`} scale={scale} frame={frame + appear} />
      </div>
      {english ? (
        <div
          style={{
            ...headline(true),
            fontSize: nameSize,
            color: COLORS.paper,
            lineHeight: 1.3,
            whiteSpace: 'nowrap',
            marginBottom: 6,
          }}
        >
          {name.toUpperCase()}
        </div>
      ) : (
        <>
          <div style={{fontFamily: FONT_CN, fontWeight: 900, fontSize: big ? 44 : 38, color: COLORS.paper, lineHeight: 1.1}}>{zh}</div>
          <div style={{fontFamily: FONT_EN, fontSize: big ? 22 : 19, letterSpacing: '0.22em', color: COLORS.iceDeep, textTransform: 'uppercase'}}>
            {en}
          </div>
        </>
      )}
    </div>
  );
};

/* 15–19 秒 敌人图鉴：十五种敌人翻牌出场，最后三张大牌落下 */
export const Roster: React.FC = () => {
  const frame = useCurrentFrame();
  const {use, index} = useClip('roster_bg');
  const exit = anim(frame, [110, 120], [0, 1], EXPO_IN);
  // 三张大牌各落在一个重拍上（每拍 15 帧），audio.py 在同样的拍子上放了低频冲击
  const giantAt = (i: number) => 60 + i * 15;
  const {wide, width, height} = useFormat();
  const {t} = useLang();
  const slots = wide ? wideLayout() : tallLayout();
  return (
    <AbsoluteFill style={{background: COLORS.night}}>
      <AbsoluteFill style={{filter: 'blur(16px) brightness(0.4)', transform: 'scale(1.2)'}}>
        <GameView clip={use.clip} index={index} width={width} height={height} focus={{...FULL, size: 300}} />
      </AbsoluteFill>
      <AbsoluteFill
        style={{
          transform: `scale(${1 + 0.03 * anim(frame, [0, 120], [0, 1]) + 0.25 * exit})`,
          opacity: 1 - exit,
          filter: `blur(${exit * 14}px)`,
        }}
      >
        {COMMON.map(([key, zh, en, name], i) => (
          <Card key={key} {...slots.common(i, key)} appear={4 + i * 4} sprite={key} zh={zh} en={en} name={name} />
        ))}
        {GIANTS.map(([key, zh, en, name, tagZh, tagEn], i) => (
          <Card
            key={key}
            {...slots.giant(i)}
            appear={giantAt(i)}
            sprite={key}
            zh={zh}
            en={en}
            name={name}
            tag={t(tagZh, tagEn)}
            boss={tagEn === 'Boss'}
            big
          />
        ))}
        <Callout
          overline={t('15 Enemies · 2 Bosses', 'Know Your Enemy')}
          title={t('十五种敌人', '15 ENEMIES')}
          sub={t(undefined, 'including 2 bosses')}
          start={2}
        />
      </AbsoluteFill>
      {GIANTS.map((_, i) => (
        <Flash key={i} at={giantAt(i)} length={6} peak={0.3} />
      ))}
    </AbsoluteFill>
  );
};
