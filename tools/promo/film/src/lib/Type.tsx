import React from 'react';
import {useCurrentFrame} from 'remotion';
import {anim, COLORS, EXPO_IN, FONT_CN, FONT_EN, headline} from './theme';
import {voiceLines} from './timeline';
import {useLang} from './lang';
import {useFormat} from './format';
import {Scrim} from './Shots';

/* 逐字显现：每个字带着模糊从下方浮起；exit 后整行模糊着散去。text 里的 \n 换行 */
export const CharReveal: React.FC<{
  text: string;
  start: number;
  stagger?: number;
  length?: number;
  exit?: number;
  style?: React.CSSProperties;
}> = ({text, start, stagger = 2, length = 14, exit, style}) => {
  const frame = useCurrentFrame();
  const leave = exit === undefined ? 0 : anim(frame, [exit, exit + 8], [0, 1], EXPO_IN);
  return (
    <div style={{whiteSpace: 'pre', ...style}}>
      {Array.from(text).map((ch, i) => {
        if (ch === '\n') return <br key={i} />;
        const t = anim(frame, [start + i * stagger, start + i * stagger + length], [0, 1]);
        return (
          <span
            key={i}
            style={{
              display: 'inline-block',
              opacity: t * (1 - leave),
              filter: `blur(${(1 - t) * 14 + leave * 16}px)`,
              transform: `translateY(${(1 - t) * 0.35}em) scale(${1.12 - 0.12 * t})`,
            }}
          >
            {ch}
          </span>
        );
      })}
    </div>
  );
};

/* 字距从宽收紧的英文小字，放在中文大标题的上方或下方 */
export const Tracked: React.FC<{
  text: string;
  start: number;
  exit?: number;
  size?: number;
  color?: string;
  style?: React.CSSProperties;
}> = ({text, start, exit, size = 30, color = COLORS.ice, style}) => {
  const frame = useCurrentFrame();
  const t = anim(frame, [start, start + 22], [0, 1]);
  const leave = exit === undefined ? 0 : anim(frame, [exit, exit + 8], [0, 1], EXPO_IN);
  return (
    <div
      style={{
        fontFamily: FONT_EN,
        fontSize: size,
        letterSpacing: `${0.55 - 0.25 * t}em`,
        color,
        opacity: t * (1 - leave),
        textTransform: 'uppercase',
        textShadow: '0 2px 12px rgba(0,0,0,0.8)',
        ...style,
      }}
    >
      {text}
    </div>
  );
};

/* 功能标题：上面一行英文小字，下面一行大字，可选一行副标题。竖屏靠上居中，横屏靠左上。
 * 中文版大字是宋体中文；英文版大字是 Cinzel 英文，字母比汉字宽，字号相应收小 */
export const Callout: React.FC<{
  overline: string;
  title: string;
  sub?: string;
  start: number;
  exit?: number;
}> = ({overline, title, sub, start, exit}) => {
  const frame = useCurrentFrame();
  const {wide, pick} = useFormat();
  const {en} = useLang();
  const size = en ? pick(96, 88) : pick(118, 104);
  const subStyle: React.CSSProperties = en
    ? {fontFamily: FONT_EN, fontStyle: 'italic', fontWeight: 400, fontSize: pick(44, 40), letterSpacing: '0.06em'}
    : {fontFamily: FONT_CN, fontWeight: 600, fontSize: 44, letterSpacing: '0.12em'};
  // 横屏的暗色衬底跟着标题一起出现、一起散去
  const shown = anim(frame, [start, start + 10], [0, 1]) * (exit === undefined ? 1 : 1 - anim(frame, [exit, exit + 8], [0, 1]));
  const place: React.CSSProperties = wide
    ? {top: 100, left: 110, textAlign: 'left'}
    : {top: 210, left: 0, right: 0, textAlign: 'center'};
  return (
  <div style={{position: 'absolute', isolation: 'isolate', ...place}}>
    <Scrim opacity={shown} />
    <Tracked text={overline} start={start} exit={exit} />
    <CharReveal
      text={title}
      start={start + 4}
      exit={exit}
      stagger={en ? 1.5 : 3}
      style={{
        ...headline(en),
        fontSize: size,
        color: COLORS.paper,
        marginTop: 14,
        textShadow: '0 4px 30px rgba(0,0,0,0.9), 0 0 40px rgba(150,205,255,0.35)',
      }}
    />
    {sub && (
      <CharReveal
        text={sub}
        start={start + 14}
        exit={exit}
        stagger={1}
        style={{
          ...subStyle,
          color: COLORS.ice,
          marginTop: 10,
          textShadow: '0 2px 16px rgba(0,0,0,0.9)',
        }}
      />
    )}
  </div>
  );
};

/* 英文版字幕：一行清晰的无衬线字，压在亮的游戏画面上也读得清 */
const EnglishLine: React.FC<{text: string}> = ({text}) => {
  const {pick} = useFormat();
  return (
    <div
      style={{
        fontFamily: '"Avenir Next", "Helvetica Neue", sans-serif',
        fontWeight: 600,
        fontSize: pick(52, 46),
        color: '#ffffff',
        letterSpacing: '0.02em',
        whiteSpace: 'nowrap',
        textShadow: '0 3px 18px rgba(0,0,0,0.95), 0 0 2px rgba(0,0,0,1)',
      }}
    >
      {text}
    </div>
  );
};

/* 跟着旁白走的字幕，固定在画面下方；中文版是中文大字 + 英文小字，英文版只有一行英文。
 * skip 里的台词由画面大字代替，不出字幕 */
export const Subtitles: React.FC<{skip?: string[]}> = ({skip = []}) => {
  const frame = useCurrentFrame();
  const {pick} = useFormat();
  const {lang, en} = useLang();
  const line = voiceLines(lang).find((v) => !skip.includes(v.id) && frame >= v.from - 4 && frame < v.from + v.duration + 10);
  if (!line) return null;
  const t = anim(frame, [line.from - 4, line.from + 6], [0, 1]);
  const out = anim(frame, [line.from + line.duration + 2, line.from + line.duration + 10], [0, 1], EXPO_IN);
  return (
    <div
      style={{
        position: 'absolute',
        left: 60,
        right: 60,
        bottom: pick(190, 64),
        textAlign: 'center',
        opacity: t * (1 - out),
        transform: `translateY(${(1 - t) * 18}px)`,
      }}
    >
      {en ? <EnglishLine text={line.en} /> : (<>
      <div
        style={{
          fontFamily: FONT_CN,
          fontWeight: 700,
          fontSize: pick(56, 50),
          color: '#ffffff',
          letterSpacing: '0.04em',
          whiteSpace: 'nowrap',
          textShadow: '0 3px 18px rgba(0,0,0,0.95), 0 0 2px rgba(0,0,0,1)',
        }}
      >
        {line.zh}
      </div>
      <div
        style={{
          fontFamily: FONT_EN,
          fontStyle: 'italic',
          fontSize: pick(34, 30),
          color: COLORS.ice,
          marginTop: pick(12, 8),
          letterSpacing: '0.04em',
          textShadow: '0 2px 12px rgba(0,0,0,0.95)',
        }}
      >
        {line.en}
      </div>
      </>)}
    </div>
  );
};
