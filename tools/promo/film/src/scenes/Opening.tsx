import React from 'react';
import {AbsoluteFill, useCurrentFrame} from 'remotion';
import {bossAt, CLIPS, GameView} from '../lib/GameView';
import {Device} from '../lib/Device';
import {Flash, NightBackdrop, Shockwave, Snow} from '../lib/Fx';
import {NameCard, SquareShot, useClip, useStage} from '../lib/Shots';
import {useFormat} from '../lib/format';
import {anim, COLORS, EXPO_IN, headline, IN_OUT, shake} from '../lib/theme';
import {useLang} from '../lib/lang';
import {CharReveal, Tracked} from '../lib/Type';

/* 0–2 秒 钩子：黑暗里一声冰裂，冰封之王走进画面，法杖射出冰冻光束 */
export const Hook: React.FC = () => {
  const frame = useCurrentFrame();
  const {use, local, index} = useClip('king_hook');
  const [kx, ky] = bossAt(use.clip, index);
  // 落在这个镜头里的第一道冰冻光束（录像标记的源帧 -> 本场景帧）
  const [, src, rate] = use.segments[0];
  const cast = (CLIPS.king.marks as {freeze: number[]}).freeze.find((f) => f >= src) ?? src;
  const freeze = use.at + Math.round((cast - src) / rate);
  const stage = useStage();
  const push = anim(frame, [use.at, 60], [0, 1], IN_OUT);
  const size = 260 - 60 * push;
  const focus = {cx: kx, cy: stage.belowHud(ky - 30, size), size};
  const s1 = shake(frame, use.at, 10, 14);
  const s2 = shake(frame, freeze, 14, 22);
  const fadeIn = anim(frame, [use.at, use.at + 8], [0, 1]);
  const king = stage.point(focus, kx, ky - 30);
  return (
    <AbsoluteFill style={{background: '#000'}}>
      {local >= 0 && (
        <AbsoluteFill style={{opacity: fadeIn}}>
          <SquareShot
            clip={use.clip}
            index={index}
            focus={focus}
            dx={s1.x + s2.x}
            dy={s1.y + s2.y}
            zoom={1 + 0.06 * Math.max(0, 1 - (frame - freeze) / 10) * (frame >= freeze ? 1 : 0)}
          />
        </AbsoluteFill>
      )}
      <Shockwave at={freeze} x={king.x} y={king.y} reach={900} length={20} />
      <Flash at={use.at} length={6} peak={0.6} />
      <Flash at={freeze} length={9} peak={0.75} color="#bfe4ff" />
      <NameCard zh="冰封之王" en="The Frost King" role="Final Boss" start={freeze - 6} />
    </AbsoluteFill>
  );
};

/* 2–5 秒 标题：掌机从夜色里升起，屏幕上拉开游戏的标题页，旁边压上片名。
 * 竖屏：掌机在上、片名在下；横屏：掌机在左半边、片名在右半边 */
export const Title: React.FC = () => {
  const frame = useCurrentFrame();
  const {wide, width: frameWidth, pick} = useFormat();
  const {en} = useLang();
  const {use, index} = useClip('title');
  const rise = anim(frame, [0, 26], [0, 1]);
  const drift = anim(frame, [0, 90], [0, 1], IN_OUT);
  const width = pick(820, 700) + 30 * drift;
  const exit = anim(frame, [82, 90], [0, 1], EXPO_IN);
  const logoPlace: React.CSSProperties = wide
    ? {top: 330, left: frameWidth / 2, right: 40}
    : {top: 1150, left: 0, right: 0};
  return (
    <AbsoluteFill>
      <NightBackdrop glow={0.6 + 0.6 * rise} />
      <Snow count={90} opacity={0.7} />
      <AbsoluteFill
        style={{
          alignItems: 'center',
          top: pick(170, 150) + (1 - rise) * 260 - 20 * drift,
          // AbsoluteFill 自带 width: 100%，横屏要直接改宽度，掌机才会居中在左半边
          width: wide ? frameWidth / 2 + 60 : '100%',
          opacity: rise * (1 - exit),
          filter: `blur(${exit * 12}px)`,
        }}
      >
        <Device
          width={width}
          tiltX={(1 - rise) * 28 + 3}
          tiltY={-2 + 4 * drift}
          glow={rise}
          screen={(w) => <GameView clip={use.clip} index={index} width={w} height={w} grade="none" />}
        />
      </AbsoluteFill>
      <div style={{position: 'absolute', ...logoPlace, textAlign: 'center', opacity: 1 - exit}}>
        {/* 片名：中文版“霜线要塞”一行；英文版 FROSTLINE / HOLD 两行，字母宽，一行放不下 */}
        <CharReveal
          text={en ? 'FROSTLINE\nHOLD' : '霜线要塞'}
          start={10}
          stagger={en ? 2 : 4}
          length={18}
          style={{
            ...headline(en),
            fontSize: en ? pick(128, 118) : pick(168, 156),
            letterSpacing: en ? '0.08em' : '0.1em',
            lineHeight: en ? 1.05 : undefined,
            color: COLORS.paper,
            textShadow: '0 6px 40px rgba(0,0,0,0.9), 0 0 60px rgba(150,205,255,0.45)',
          }}
        />
        {!en && <Tracked text="Frostline Hold" start={22} size={46} style={{marginTop: 4}} />}
        <Tracked
          text={en ? 'A Winter Tower Defense' : '凛冬塔防 · A Winter Tower Defense'}
          start={en ? 24 : 30}
          size={en ? 34 : 26}
          color={en ? COLORS.ice : 'rgba(220,235,250,0.75)'}
          style={{marginTop: en ? 24 : 18}}
        />
      </div>
      <Flash at={0} length={12} peak={0.85} />
    </AbsoluteFill>
  );
};
