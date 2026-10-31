import {createContext, useContext} from 'react';

/* 成片语言：zh 是中文为主（中文大字 + 英文小字），en 是纯英文 */
export type Lang = 'zh' | 'en';

export const LangContext = createContext<Lang>('zh');

/* t(中文版取值, 英文版取值)：按当前语言挑一个 */
export const useLang = () => {
  const lang = useContext(LangContext);
  const en = lang === 'en';
  const t = <T>(zhValue: T, enValue: T): T => (en ? enValue : zhValue);
  return {lang, en, t};
};

/* 各语言的素材：游戏录像目录、配乐 */
export const ASSETS: Record<Lang, {clips: string; soundtrack: string}> = {
  zh: {clips: 'clips', soundtrack: 'audio/soundtrack.wav'},
  en: {clips: 'clips_en', soundtrack: 'audio/soundtrack_en.wav'},
};
