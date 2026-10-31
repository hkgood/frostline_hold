import React from 'react';
import {AbsoluteFill, Img, staticFile, useCurrentFrame} from 'remotion';
import {CLIPS} from '../lib/GameView';
import {Flash} from '../lib/Fx';
import {Scrim, SquareShot, TapRipple, useClip, useStage} from '../lib/Shots';
import {useFormat} from '../lib/format';
import {anim, COLORS, EXPO_IN, FONT_CN, FONT_EN, FONT_UI, headline, IN_OUT, pulse} from '../lib/theme';
import {useLang} from '../lib/lang';
import {Callout} from '../lib/Type';

/* 七座塔的出场顺序、名字和这次录像里选的两级升级分支（见 shots.py 的 FALLS_TOWERS）。
 * en 是中文版里的英文小字；name / upgrades 是英文版用的、与游戏英文界面一致的名字（strings.py） */
const TOWERS = [
  {id: 't_storm', kind: 4, card: 'storm', zh: '雷暴塔', en: 'Storm Spire', path: '过载 · 风暴',
   name: 'Storm Tower', upgrades: 'Overload · Tempest'},
  {id: 't_rune', kind: 5, card: 'rune', zh: '符文塔', en: 'Rune Obelisk', path: '咒印 · 棱镜',
   name: 'Rune Tower', upgrades: 'Hex · Prism'},
  {id: 't_siege', kind: 3, card: 'siege', zh: '投石塔', en: 'Siege Catapult', path: '碎石雨 · 粉碎',
   name: 'Siege Tower', upgrades: 'Hail · Shatter'},
  {id: 't_frost', kind: 2, card: 'frost', zh: '冰霜塔', en: 'Frost Tower', path: '冰川 · 霜爆',
   name: 'Frost Tower', upgrades: 'Glacier · Nova'},
  {id: 't_ember', kind: 7, card: 'ember', zh: '余烬塔', en: 'Ember Kiln', path: '炼狱 · 野火',
   name: 'Ember Tower', upgrades: 'Inferno · Wildfire'},
  {id: 't_guard', kind: 1, card: 'guard', zh: '箭塔', en: 'Guard Tower', path: '巨弩 · 神射',
   name: 'Guard Tower', upgrades: 'Ballista · Marksman'},
  {id: 't_barracks', kind: 6, card: 'barracks', zh: '盾卫营', en: 'Barracks', path: '盾墙 · 老兵',
   name: 'Barracks', upgrades: 'Shieldwall · Veterans'},
];

const RingShot: React.FC = () => {
  const frame = useCurrentFrame();
  const {use, local, index, active} = useClip('ring');
  const stage = useStage();
  if (!active) return null;
  const marks = CLIPS.ring.marks as {pad: [number, number]; storm_medal: [number, number]; ring_open: number; pick: number; build: number};
  const [px, py] = marks.pad;
  const t = anim(local, [0, use.duration], [0, 1], IN_OUT);
  const focus = {cx: px, cy: py + 8, size: 300 - 50 * t};
  const toLocal = (src: number) => src - use.segments[0][1];
  const pad = stage.point(focus, px, py);
  const medal = stage.point(focus, ...marks.storm_medal);
  const built = toLocal(marks.build);
  return (
    <AbsoluteFill>
      <SquareShot clip={use.clip} index={index} focus={focus} zoom={1 + 0.04 * pulse(local, built, 10)} />
      <TapRipple at={toLocal(marks.ring_open) - 2} x={pad.x} y={pad.y} />
      <TapRipple at={toLocal(marks.pick)} x={medal.x} y={medal.y} />
      <TapRipple at={built} x={medal.x} y={medal.y} />
      <Flash at={built + 1} length={8} peak={0.45} />
    </AbsoluteFill>
  );
};

const TowerLabel: React.FC<{tower: (typeof TOWERS)[number]; n: number; local: number}> = ({tower, n, local}) => {
  const t = anim(local, [0, 8], [0, 1]);
  const {wide} = useFormat();
  const {en} = useLang();
  // 竖屏：顶部居中；横屏：左上角
  const place: React.CSSProperties = wide
    ? {left: 130, top: 110}
    : {left: 0, right: 0, top: 200, display: 'flex', justifyContent: 'center'};
  return (
    <div style={{position: 'absolute', isolation: 'isolate', ...place}}>
      <Scrim width={560} height={230} opacity={t} />
      <div style={{display: 'flex', alignItems: 'center', gap: 34, opacity: t, transform: `translateY(${(1 - t) * 30}px)`}}>
        <div
          style={{
            width: 132,
            height: 132,
            transform: 'rotate(45deg)',
            border: `3px solid ${COLORS.ice}`,
            background: 'rgba(8,14,24,0.85)',
            boxShadow: '0 0 30px rgba(150,205,255,0.45)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
          }}
        >
          <Img
            src={staticFile(`sprites/card_${tower.card}.png`)}
            style={{width: 112, height: 112, transform: 'rotate(-45deg)', imageRendering: 'pixelated'}}
          />
        </div>
        <div>
          <div style={{fontFamily: FONT_UI, fontSize: 30, letterSpacing: '0.3em', color: COLORS.iceDeep}}>
            {String(n).padStart(2, '0')} / 07
          </div>
          <div
            style={{
              ...headline(en),
              fontSize: en ? 72 : 96,
              color: COLORS.paper,
              lineHeight: en ? 1.15 : 1.05,
              whiteSpace: 'nowrap',
              textShadow: '0 4px 24px rgba(0,0,0,0.95)',
            }}
          >
            {en ? tower.name.toUpperCase() : tower.zh}
          </div>
          {en ? (
            <div style={{fontFamily: FONT_EN, fontStyle: 'italic', fontSize: 34, color: COLORS.ice, marginTop: 4, letterSpacing: '0.04em'}}>
              Upgrades: {tower.upgrades}
            </div>
          ) : (
            <>
              <div style={{fontFamily: FONT_EN, fontSize: 30, letterSpacing: '0.25em', color: COLORS.ice, textTransform: 'uppercase'}}>
                {tower.en}
              </div>
              <div style={{fontFamily: FONT_CN, fontSize: 32, color: 'rgba(225,235,245,0.8)', marginTop: 6, letterSpacing: '0.1em'}}>
                升级 {tower.path}
              </div>
            </>
          )}
        </div>
      </div>
    </div>
  );
};

const TowerCut: React.FC<{tower: (typeof TOWERS)[number]; n: number}> = ({tower, n}) => {
  const {use, local, index, active} = useClip(tower.id);
  if (!active) return null;
  const [x, y] = (CLIPS.towers.marks as {towers: Record<string, [number, number]>}).towers[String(tower.kind)];
  const t = local / use.duration;
  const focus = {cx: x, cy: y - 22, size: 215 - 30 * t};
  return (
    <AbsoluteFill>
      <SquareShot clip={use.clip} index={index} focus={focus} zoom={1 + 0.07 * pulse(local, 0, 7)} />
      <Flash at={use.at} length={5} peak={0.35} />
      <TowerLabel tower={tower} n={n} local={local} />
    </AbsoluteFill>
  );
};

/* 5–10 秒 建塔：真实的建造环交互，然后七座满级塔逐一亮相 */
export const Build: React.FC = () => {
  const frame = useCurrentFrame();
  const exit = anim(frame, [142, 150], [0, 1], EXPO_IN);
  const {t} = useLang();
  return (
    <AbsoluteFill style={{background: '#000'}}>
      <AbsoluteFill style={{filter: `blur(${exit * 10}px)`}}>
        <RingShot />
        {TOWERS.map((tower, i) => (
          <TowerCut key={tower.id} tower={tower} n={i + 1} />
        ))}
      </AbsoluteFill>
      <Callout
        overline={t('7 Towers · 14 Upgrade Paths', 'Build the Line')}
        title={t('七重防线', 'SEVEN TOWERS')}
        sub={t('7 种防御塔 · 14 条升级路线', '14 upgrade paths')}
        start={2}
        exit={38}
      />
      <Flash at={0} length={10} peak={0.7} />
    </AbsoluteFill>
  );
};
