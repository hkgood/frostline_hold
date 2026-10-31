import React from 'react';
import {AbsoluteFill, useCurrentFrame} from 'remotion';
import {GameView} from '../lib/GameView';
import {Device} from '../lib/Device';
import {Flash, NightBackdrop, Shockwave, Snow} from '../lib/Fx';
import {SquareShot, useClip} from '../lib/Shots';
import {useFormat} from '../lib/format';
import {anim, COLORS, EXPO_IN, EXPO_OUT, FONT_UI, headline, IN_OUT} from '../lib/theme';
import {useLang} from '../lib/lang';
import {Callout, CharReveal, Tracked} from '../lib/Type';

/* 24–27 秒 战役：章节地图像一张铺在桌上的长卷，向远处滚动。
 * 竖屏：地图在标题下方；横屏：地图斜放在右边，标题在左上 */
export const Campaign: React.FC = () => {
  const frame = useCurrentFrame();
  const {wide} = useFormat();
  const {t: tr} = useLang();
  const {use, index} = useClip('campaign');
  const t = anim(frame, [0, 90], [0, 1], IN_OUT);
  const enter = anim(frame, [0, 14], [0, 1], EXPO_OUT);
  const exit = anim(frame, [84, 90], [0, 1], EXPO_IN);
  const scale = 1.08 - 0.08 * enter + 0.04 * t;
  const box = wide ? {left: 1010, top: 70, width: 800, height: 800} : {left: 50, top: 560, width: 980, height: 980};
  const transform = wide
    ? `perspective(1600px) rotateY(${-16 + 6 * t}deg) rotateX(${12 - 4 * t}deg) scale(${scale})`
    : `perspective(1600px) rotateX(${22 - 8 * t}deg) scale(${scale})`;
  return (
    <AbsoluteFill style={{background: COLORS.night}}>
      <AbsoluteFill style={{opacity: enter * (1 - exit)}}>
        <SquareShot clip={use.clip} index={index} box={box} transform={transform} />
      </AbsoluteFill>
      <Snow count={60} opacity={0.5} />
      <Callout
        overline={tr('8 Chapters · 22 Stages', 'The Campaign')}
        title={tr('八章远征', '8 CHAPTERS')}
        sub={tr('8 个章节 · 22 场死守', '22 last stands')}
        start={4}
        exit={84}
      />
      <Flash at={0} length={8} peak={0.5} />
    </AbsoluteFill>
  );
};

/* 27–30 秒 收尾：三星胜利，掌机落定，片名定格。
 * 竖屏：掌机在上、片名在下；横屏：掌机在左半边、片名在右半边 */
export const Finale: React.FC = () => {
  const frame = useCurrentFrame();
  const {wide, width, pick} = useFormat();
  const {en, t} = useLang();
  const victory = useClip('victory');
  const logo = useClip('logo');
  const settle = anim(frame, [logo.use.at, logo.use.at + 20], [0, 1], EXPO_OUT);
  const deviceWidth = pick(640, 600);
  const deviceTop = pick(180, 220);
  // 冲击波从掌机屏幕中心扩散
  const center = wide ? {x: (width / 2 + 60) / 2, y: deviceTop + 300} : {x: width / 2, y: 500};
  const logoPlace: React.CSSProperties = wide ? {top: 290, left: width / 2, right: 40} : {top: 1000, left: 0, right: 0};
  return (
    <AbsoluteFill style={{background: '#000'}}>
      {victory.active && (
        <SquareShot
          clip={victory.use.clip}
          index={victory.index}
          focus={{cx: 240, cy: 215, size: 400 - 50 * anim(victory.local, [0, 30], [0, 1], IN_OUT)}}
        />
      )}
      {logo.active && (
        <AbsoluteFill>
          <NightBackdrop glow={1.2} />
          <Snow count={110} opacity={0.8} />
          <AbsoluteFill
            style={{alignItems: 'center', top: deviceTop + 120 * (1 - settle), width: wide ? width / 2 + 60 : '100%', opacity: settle}}
          >
            <Device
              width={deviceWidth}
              tiltX={4 + 20 * (1 - settle)}
              tiltY={wide ? 6 * (1 - settle) - 4 : 0}
              glow={1.3}
              screen={(w) => <GameView clip={logo.use.clip} index={logo.index} width={w} height={w} grade="none" />}
            />
          </AbsoluteFill>
          <div style={{position: 'absolute', ...logoPlace, textAlign: 'center'}}>
            {/* 片名：中文版“霜线要塞”+ 英文小字；英文版 FROSTLINE / HOLD 两行大字 */}
            <CharReveal
              text={t('霜线要塞', 'FROSTLINE\nHOLD')}
              start={logo.use.at + 4}
              stagger={en ? 1.5 : 3}
              length={14}
              style={{
                ...headline(en),
                fontSize: en ? pick(140, 124) : pick(190, 170),
                letterSpacing: en ? '0.08em' : '0.1em',
                lineHeight: en ? 1.05 : undefined,
                color: COLORS.paper,
                textShadow: '0 6px 40px rgba(0,0,0,0.9), 0 0 70px rgba(150,205,255,0.55)',
              }}
            />
            {!en && <Tracked text="Frostline Hold" start={logo.use.at + 10} size={pick(52, 48)} style={{marginTop: 4}} />}
            <div style={{height: 2, width: 420 * anim(frame, [logo.use.at + 14, logo.use.at + 30], [0, 1], EXPO_OUT), background: COLORS.ice, margin: pick('40px auto', '32px auto'), boxShadow: `0 0 14px ${COLORS.ice}`}} />
            <Tracked text={t('守住它 · Hold the Line', 'Hold the Line')} start={logo.use.at + 18} size={en ? pick(46, 42) : pick(40, 36)} color={COLORS.paper} />
          </div>
          <div
            style={{
              position: 'absolute',
              bottom: pick(150, 130),
              left: wide ? width / 2 : 0,
              right: wide ? 40 : 0,
              textAlign: 'center',
              fontFamily: FONT_UI,
              fontSize: pick(30, 26),
              letterSpacing: '0.35em',
              color: 'rgba(215,232,248,0.7)',
              opacity: anim(frame, [logo.use.at + 26, logo.use.at + 40], [0, 1]),
            }}
          >
            {t('ESP-MOSAICO 掌机游戏', 'A GAME FOR THE ESP-MOSAICO HANDHELD')}
          </div>
          <Shockwave at={logo.use.at} x={center.x} y={center.y} reach={1300} length={24} />
        </AbsoluteFill>
      )}
      <Flash at={0} length={8} peak={0.6} />
      <Flash at={logo.use.at} length={14} peak={1} color="#eaf6ff" />
    </AbsoluteFill>
  );
};
