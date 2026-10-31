import timeline from '../timeline.json';
import voice from '../voice.json';
import voiceEn from '../voice_en.json';
import type {Lang} from './lang';

export type SceneId = keyof typeof timeline.scenes;

export const FPS = timeline.fps;
export const WIDTH = timeline.width;
export const HEIGHT = timeline.height;
export const DURATION = timeline.durationInFrames;
/* 横屏版（YouTube）的画布尺寸 */
export const WIDE = timeline.wide;
/* 每拍的帧数（120 BPM 下是 15）：剪辑点与重拍都落在它的整数倍上 */
export const BEAT = (60 / timeline.bpm) * timeline.fps;

export const scene = (id: SceneId) => timeline.scenes[id];

/* 一个镜头的取帧方式：从 segment 的 local 帧起，源帧 = src + (t - local) * rate */
export type Segment = [number, number, number];
export type ClipUse = {id: string; clip: string; at: number; duration: number; segments: Segment[]};

export const clipUse = (id: string): ClipUse => {
  const found = timeline.clips.find((c) => c.id === id);
  if (!found) throw new Error(`no clip use ${id}`);
  return found as unknown as ClipUse;
};

/* 场景内第 local 帧（相对镜头的 at）对应的源帧，可以是小数 */
export const srcAt = (use: ClipUse, local: number) => {
  const t = Math.max(0, local);
  let seg = use.segments[0];
  for (const s of use.segments) if (s[0] <= t) seg = s;
  return seg[1] + (t - seg[0]) * seg[2];
};

/* 旁白：时间轴给出文字，voice.json / voice_en.json（audio.py 生成）给出各语言真实的起点与时长 */
export type VoiceLine = {id: string; zh: string; en: string; from: number; duration: number};

const TIMINGS: Record<Lang, {id: string; from: number; duration: number}[]> = {zh: voice, en: voiceEn};

export const voiceLines = (lang: Lang): VoiceLine[] =>
  timeline.voice.map((line) => {
    const timing = TIMINGS[lang].find((v) => v.id === line.id);
    if (!timing) throw new Error(`voice ${line.id} missing; run npm run audio${lang === 'en' ? ':en' : ''}`);
    return {id: line.id, zh: line.zh, en: line.en, from: timing.from, duration: timing.duration};
  });

export const HITS: number[] = timeline.hits;
