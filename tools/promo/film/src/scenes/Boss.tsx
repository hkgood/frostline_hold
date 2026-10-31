import React from 'react';
import {AbsoluteFill, useCurrentFrame} from 'remotion';
import {bossAt, CLIPS} from '../lib/GameView';
import {Flash, Shockwave} from '../lib/Fx';
import {NameCard, SquareShot, useClip, useStage} from '../lib/Shots';
import {useFormat} from '../lib/format';
import {useLang} from '../lib/lang';
import {anim, EXPO_IN, IN_OUT, shake} from '../lib/theme';
import {Callout} from '../lib/Type';

/* 录像里首领音效出现的源帧（横幅登场） */
const BANNER_SOUND = 10;

/* 首领来袭横幅：需要看到整行字，用正方形完整画面 */
const BannerShot: React.FC = () => {
  const frame = useCurrentFrame();
  const {use, local, index, active} = useClip('wyrm_banner');
  const {wide} = useFormat();
  if (!active) return null;
  const hit = use.at + BANNER_SOUND - use.segments[0][1];
  const s = shake(frame, hit, 16, 18);
  const push = anim(local, [0, use.duration], [0, 1], IN_OUT);
  // 横屏已经是整张宽度，取景没法再收，改用画面整体轻推
  return (
    <AbsoluteFill>
      <SquareShot
        clip={use.clip}
        index={index}
        focus={{cx: 240, cy: 230, size: 480 - 70 * push}}
        zoom={wide ? 1 + 0.06 * push : 1}
        dx={s.x}
        dy={s.y}
      />
      <Flash at={hit} length={8} peak={0.5} color="#ffd9c7" />
    </AbsoluteFill>
  );
};

/* 跟着首领移动的特写 */
const TrackShot: React.FC<{id: string; lift: number; size: number; cast?: number}> = ({id, lift, size, cast}) => {
  const frame = useCurrentFrame();
  const {use, local, index, active} = useClip(id);
  const stage = useStage();
  if (!active) return null;
  const [x, y] = bossAt(use.clip, index);
  const t = anim(local, [0, use.duration], [0, 1]);
  const span = size - 40 * t;
  const focus = {cx: x, cy: stage.belowHud(y - lift, span), size: span};
  const castAt = cast === undefined ? -999 : use.at + cast;
  const s1 = shake(frame, use.at, 10, 20);
  const s2 = shake(frame, castAt, 14, 24);
  const boss = stage.point(focus, x, y - lift);
  return (
    <AbsoluteFill>
      <SquareShot
        clip={use.clip}
        index={index}
        focus={focus}
        dx={s1.x + s2.x}
        dy={s1.y + s2.y}
        zoom={1 + 0.1 * Math.max(0, 1 - local / 8)}
      />
      <Flash at={use.at} length={6} peak={0.6} />
      {cast !== undefined && <Shockwave at={castAt} x={boss.x} y={boss.y} reach={900} length={20} />}
      {cast !== undefined && <Flash at={castAt} length={10} peak={0.7} color="#bfe4ff" />}
    </AbsoluteFill>
  );
};

/* 19–24 秒 首领战：横幅预警 → 巨蠕破土 → 冰封之王冻结防线 */
export const Boss: React.FC = () => {
  const frame = useCurrentFrame();
  const king = useClip('king_freeze');
  // 国王录像里落在这一段里的冰冻光束
  const freezes = (CLIPS.king.marks as {freeze: number[]}).freeze;
  const start = king.use.segments[0][1];
  const cast = freezes.find((f) => f >= start && f < start + king.use.duration);
  const exit = anim(frame, [144, 150], [0, 1], EXPO_IN);
  const {t} = useLang();
  return (
    <AbsoluteFill style={{background: '#000'}}>
      <AbsoluteFill style={{filter: `blur(${exit * 10}px)`}}>
        <BannerShot />
        <TrackShot id="wyrm_rise" lift={30} size={250} />
        <TrackShot id="king_freeze" lift={40} size={250} cast={cast === undefined ? undefined : cast - start} />
      </AbsoluteFill>
      <Callout overline={t('Boss Battles', 'Two Bosses')} title={t('两大首领', 'BOSS BATTLES')} start={6} exit={56} />
      <NameCard zh="矿洞巨蠕" en="Mine Wyrm" role="Boss" start={64} exit={100} />
      <NameCard zh="冰封之王" en="The Frost King" role="Final Boss" start={110} />
    </AbsoluteFill>
  );
};
