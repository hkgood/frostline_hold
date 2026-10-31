import React from 'react';
import {AbsoluteFill} from 'remotion';
import {CLIPS, FrostGradeFilter, FULL, GameView} from './lib/GameView';
import {Grain} from './lib/Fx';
import {Sprite} from './scenes/Roster';
import {COLORS, FONT_EN, FONT_UI, headline} from './lib/theme';
import {Lang, LangContext, useLang} from './lib/lang';

/* YouTube 封面（1280×720）：右边一个放大的冰封之王，左边压大字片名。
 * 封面常被缩成很小的图，游戏截图里的国王会淹没在 1-bit 细节里，
 * 所以用干净的精灵图放大做主体，游戏画面模糊压暗当背景 */
export const Thumbnail: React.FC<{lang: Lang}> = ({lang}) => (
  <LangContext.Provider value={lang}>
    <ThumbnailBody />
  </LangContext.Provider>
);

const ThumbnailBody: React.FC = () => {
  const background = (CLIPS.king.marks as {freeze: number[]}).freeze[1];
  const {en} = useLang();
  return (
    <AbsoluteFill style={{background: COLORS.night}}>
      <FrostGradeFilter />
      <AbsoluteFill style={{filter: 'blur(5px) brightness(0.42)', transform: 'scale(1.08)'}}>
        <GameView clip="king" index={background} width={1280} height={720} focus={{...FULL, cy: 150}} />
      </AbsoluteFill>
      {/* 国王身后的冰光 */}
      <AbsoluteFill
        style={{
          background: 'radial-gradient(circle at 76% 50%, rgba(170,225,255,0.6) 0%, rgba(110,180,255,0.22) 24%, rgba(0,0,0,0) 48%)',
          mixBlendMode: 'screen',
        }}
      />
      <div
        style={{
          position: 'absolute',
          left: 720,
          top: 70,
          filter: 'drop-shadow(0 0 2px #eaf6ff) drop-shadow(0 0 2px #eaf6ff) drop-shadow(0 0 40px rgba(140,200,255,0.9))',
        }}
      >
        <Sprite name="enemy_king" scale={7} frame={0} />
      </div>
      {/* 左侧压暗，给片名一块干净的底 */}
      <AbsoluteFill
        style={{
          background: 'linear-gradient(90deg, rgba(4,7,12,0.95) 0%, rgba(4,7,12,0.8) 36%, rgba(4,7,12,0) 62%)',
        }}
      />
      <AbsoluteFill style={{boxShadow: 'inset 0 0 160px rgba(2,4,8,0.85)'}} />
      <div style={{position: 'absolute', left: 70, top: en ? 190 : 160}}>
        <div
          style={{
            display: 'inline-block',
            fontFamily: FONT_UI,
            fontWeight: 700,
            fontSize: 24,
            letterSpacing: '0.3em',
            color: COLORS.night,
            background: COLORS.ice,
            padding: '6px 16px',
          }}
        >
          {en ? 'A WINTER TOWER DEFENSE' : '凛冬塔防 · TOWER DEFENSE'}
        </div>
        {/* 中文版：“霜线要塞”大字 + 英文小字；英文版：FROSTLINE / HOLD 两行大字，宽度要让开右边的国王 */}
        <div
          style={{
            ...headline(en),
            fontSize: en ? 104 : 168,
            lineHeight: en ? 1.08 : 1.15,
            marginTop: 18,
            letterSpacing: '0.04em',
            whiteSpace: 'pre',
            color: '#f4f8fb',
            textShadow: '0 6px 30px rgba(0,0,0,0.9), 0 0 50px rgba(150,210,255,0.6)',
          }}
        >
          {en ? 'FROSTLINE\nHOLD' : '霜线要塞'}
        </div>
        {!en && (
          <div
            style={{
              fontFamily: FONT_EN,
              fontSize: 50,
              letterSpacing: '0.24em',
              color: COLORS.ice,
              marginTop: 6,
              textShadow: '0 3px 16px rgba(0,0,0,0.9)',
            }}
          >
            FROSTLINE HOLD
          </div>
        )}
      </div>
      <Grain amount={0.06} />
    </AbsoluteFill>
  );
};
