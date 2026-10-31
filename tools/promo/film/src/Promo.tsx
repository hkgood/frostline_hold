import React from 'react';
import {AbsoluteFill, Audio, Sequence, staticFile} from 'remotion';
import {FrostGradeFilter} from './lib/GameView';
import {Grain, Snow, Vignette} from './lib/Fx';
import {ASSETS, Lang, LangContext} from './lib/lang';
import {Subtitles} from './lib/Type';
import {scene, SceneId} from './lib/timeline';
import {Hook, Title} from './scenes/Opening';
import {Build} from './scenes/Build';
import {Nova} from './scenes/Nova';
import {Roster} from './scenes/Roster';
import {Boss} from './scenes/Boss';
import {Campaign, Finale} from './scenes/Closing';

/* 场景 id 与组件的对应，时间段全部来自 timeline.json */
const SCENES: [SceneId, React.FC][] = [
  ['hook', Hook],
  ['title', Title],
  ['build', Build],
  ['nova', Nova],
  ['roster', Roster],
  ['boss', Boss],
  ['campaign', Campaign],
  ['finale', Finale],
];

export const Promo: React.FC<{audio: boolean; lang: Lang}> = ({audio, lang}) => (
  <LangContext.Provider value={lang}>
    <AbsoluteFill style={{background: '#000'}}>
      <FrostGradeFilter />
      {SCENES.map(([id, Scene]) => {
        const s = scene(id);
        return (
          <Sequence key={id} from={s.from} durationInFrames={s.duration} name={id}>
            <Scene />
          </Sequence>
        );
      })}
      {/* 全片共用的氛围层：飘雪、暗角、胶片颗粒 */}
      <Snow count={70} opacity={0.55} wind={0.6} />
      <Vignette strength={0.55} />
      <Grain amount={0.08} />
      {/* 最后一句旁白就是片名，画面上已有大字 */}
      <Subtitles skip={['v6']} />
      {audio && <Audio src={staticFile(ASSETS[lang].soundtrack)} />}
    </AbsoluteFill>
  </LangContext.Provider>
);
