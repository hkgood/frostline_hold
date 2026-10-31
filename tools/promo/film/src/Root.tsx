import React from 'react';
import {Composition, Still} from 'remotion';
import './lib/fonts';
import {Promo} from './Promo';
import {Thumbnail} from './Thumbnail';
import {DURATION, FPS, HEIGHT, WIDE, WIDTH} from './lib/timeline';

/* 同一份时间轴、录像和配乐，出两种画幅、两种语言；排版由各组件按画幅和语言自己决定 */
export const Root: React.FC = () => (
  <>
    {/* 竖屏 9:16：抖音、视频号、Shorts */}
    <Composition
      id="FrostlinePromo"
      component={Promo}
      durationInFrames={DURATION}
      fps={FPS}
      width={WIDTH}
      height={HEIGHT}
      defaultProps={{audio: true, lang: 'zh' as const}}
    />
    {/* 横屏 16:9：YouTube，渲染时 --scale=2 输出 4K */}
    <Composition
      id="FrostlinePromoWide"
      component={Promo}
      durationInFrames={DURATION}
      fps={FPS}
      width={WIDE.width}
      height={WIDE.height}
      defaultProps={{audio: true, lang: 'zh' as const}}
    />
    {/* YouTube 封面 */}
    <Still id="FrostlineThumbnail" component={Thumbnail} width={1280} height={720} defaultProps={{lang: 'zh' as const}} />

    {/* 纯英文版：英文界面的录像、英文旁白与字幕、英文画面文字 */}
    <Composition
      id="FrostlinePromoEn"
      component={Promo}
      durationInFrames={DURATION}
      fps={FPS}
      width={WIDTH}
      height={HEIGHT}
      defaultProps={{audio: true, lang: 'en' as const}}
    />
    <Composition
      id="FrostlinePromoWideEn"
      component={Promo}
      durationInFrames={DURATION}
      fps={FPS}
      width={WIDE.width}
      height={WIDE.height}
      defaultProps={{audio: true, lang: 'en' as const}}
    />
    <Still id="FrostlineThumbnailEn" component={Thumbnail} width={1280} height={720} defaultProps={{lang: 'en' as const}} />
  </>
);
