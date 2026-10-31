import React from 'react';
import {AbsoluteFill, useCurrentFrame} from 'remotion';
import {CLIPS} from '../lib/GameView';
import {Flash, Shockwave} from '../lib/Fx';
import {Scrim, SquareShot, useClip, useStage} from '../lib/Shots';
import {useFormat} from '../lib/format';
import {anim, COLORS, EXPO_IN, EXPO_OUT, FONT_EN, headline, IN_OUT, shake} from '../lib/theme';
import {useLang} from '../lib/lang';
import {Callout} from '../lib/Type';

/* 释放瞬间在本场景的第 90 帧（timeline.json 里 nova 的变速段：前面慢放蓄力，90 帧定格后恢复） */
const IMPACT = 90;

/* 10–15 秒 英雄技能：镜头推向霜卫，时间放慢，冰霜新星炸开 */
export const Nova: React.FC = () => {
  const frame = useCurrentFrame();
  const {use, index} = useClip('nova');
  const [hx, hy] = (CLIPS.nova.marks as {hero: [number, number]}).hero;
  const stage = useStage();
  const {pick} = useFormat();
  const {en, t} = useLang();

  // 镜头：从战场全景推到英雄，爆发时猛地再推一下，然后缓缓拉开
  const push = anim(frame, [0, IMPACT], [0, 1], IN_OUT);
  const after = anim(frame, [IMPACT, 150], [0, 1], EXPO_OUT);
  const size = frame < IMPACT ? 460 - 230 * push : 210 + 130 * after;
  const focus = {cx: hx, cy: 300 + (hy - 30 - 300) * push, size};
  const hero = stage.point(focus, hx, hy - 14);

  const hit = shake(frame, IMPACT, 20, 30);
  const punch = frame >= IMPACT ? 1 + 0.12 * Math.max(0, 1 - (frame - IMPACT) / 8) : 1;

  // 慢放时四周结霜、画面变冷；爆发时做一下红蓝错位
  const slow = anim(frame, [40, 60], [0, 1]) * (1 - anim(frame, [IMPACT, IMPACT + 4], [0, 1]));
  const split = Math.max(0, 1 - Math.abs(frame - IMPACT - 2) / 6) * 10;
  const filter = split > 0
    ? `drop-shadow(${split}px 0 0 rgba(255,40,90,0.55)) drop-shadow(${-split}px 0 0 rgba(40,190,255,0.55))`
    : `saturate(${1 - 0.3 * slow}) brightness(${1 - 0.15 * slow})`;

  // 蓄力光晕：越接近爆发越大越亮
  const charge = anim(frame, [44, IMPACT], [0, 1]) * (frame < IMPACT ? 1 : 0);

  const slam = anim(frame, [IMPACT + 3, IMPACT + 13], [0, 1], EXPO_OUT);
  const slamExit = anim(frame, [142, 150], [0, 1], EXPO_IN);

  return (
    <AbsoluteFill style={{background: '#000'}}>
      <SquareShot clip={use.clip} index={index} focus={focus} dx={hit.x} dy={hit.y} zoom={punch} filter={filter} />
      {/* 时间放慢时四周结霜 */}
      <AbsoluteFill
        style={{
          opacity: slow,
          background: 'radial-gradient(ellipse at 50% 45%, rgba(0,0,0,0) 38%, rgba(170,215,255,0.28) 72%, rgba(230,245,255,0.55) 100%)',
        }}
      />
      {charge > 0 && (
        <div
          style={{
            position: 'absolute',
            left: hero.x - 260,
            top: hero.y - 260,
            width: 520,
            height: 520,
            borderRadius: '50%',
            transform: `scale(${0.3 + 0.7 * charge})`,
            background: `radial-gradient(circle, rgba(235,248,255,${0.8 * charge}) 0%, rgba(150,210,255,${0.45 * charge}) 30%, rgba(0,0,0,0) 70%)`,
            mixBlendMode: 'screen',
          }}
        />
      )}
      <Shockwave at={IMPACT} x={hero.x} y={hero.y} reach={1500} length={26} />
      <Flash at={IMPACT} length={14} peak={1} color="#eaf6ff" />
      <Callout overline={t('Hero · The Warden', 'Your Hero')} title={t('唤醒霜卫', 'THE WARDEN')} start={8} exit={84} />
      {/* 爆发后压上的大字 */}
      {frame >= IMPACT && (
        <div
          style={{
            position: 'absolute',
            left: 0,
            right: 0,
            top: pick(150, 70),
            textAlign: 'center',
            opacity: slam * (1 - slamExit),
            transform: `scale(${1.5 - 0.5 * slam})`,
          }}
        >
          {/* 收成文字宽度的块，暗底才能对准文字中心 */}
          <div style={{display: 'inline-block', position: 'relative', isolation: 'isolate'}}>
            {/* 英文大字笔画细，横屏全景是大片雪地，要压一层暗底才读得清 */}
            {en && <Scrim width={1000} height={230} opacity={0.85} />}
            <div
              style={{
                ...headline(en),
                fontSize: en ? pick(124, 140) : pick(170, 150),
                letterSpacing: '0.08em',
                whiteSpace: 'nowrap',
                color: '#f4fbff',
                textShadow: `0 0 50px ${COLORS.ice}, 0 8px 40px rgba(0,0,0,0.9)`,
              }}
            >
              {t('冰霜新星', 'FROST NOVA')}
            </div>
            <div style={{fontFamily: FONT_EN, fontSize: 46, letterSpacing: '0.45em', color: COLORS.ice, marginTop: 6}}>
              {t('FROST NOVA', 'HERO SKILL')}
            </div>
          </div>
        </div>
      )}
    </AbsoluteFill>
  );
};
