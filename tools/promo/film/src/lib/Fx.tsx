import React from 'react';
import {AbsoluteFill, random, useCurrentFrame, useVideoConfig} from 'remotion';
import {pulse} from './theme';

/* 三层景深的飘雪：远处细小清晰，近处大而虚；整片随风缓慢摆动 */
export const Snow: React.FC<{count?: number; opacity?: number; wind?: number}> = ({
  count = 140, opacity = 1, wind = 1,
}) => {
  const frame = useCurrentFrame();
  const {width: WIDTH, height: HEIGHT} = useVideoConfig();
  const flakes = [];
  for (let i = 0; i < count; i++) {
    const depth = random(`d${i}`);
    const size = 2 + depth * depth * 16;
    const speed = 1.2 + depth * 5.5;
    const x0 = random(`x${i}`) * (WIDTH + 200) - 100;
    const y0 = random(`y${i}`) * HEIGHT;
    const y = (y0 + frame * speed) % (HEIGHT + 40) - 20;
    const sway = Math.sin(frame / (40 + depth * 30) + i) * (10 + depth * 30);
    const x = x0 + sway + frame * wind * (0.6 + depth * 2.2);
    flakes.push(
      <div
        key={i}
        style={{
          position: 'absolute',
          left: ((x % (WIDTH + 200)) + WIDTH + 200) % (WIDTH + 200) - 100,
          top: y,
          width: size,
          height: size,
          borderRadius: '50%',
          background: 'rgba(235,245,255,0.9)',
          opacity: (0.25 + 0.6 * (1 - Math.abs(depth - 0.5))) * opacity,
          filter: depth > 0.75 ? `blur(${(depth - 0.75) * 14}px)` : undefined,
        }}
      />,
    );
  }
  return <AbsoluteFill style={{pointerEvents: 'none'}}>{flakes}</AbsoluteFill>;
};

/* 胶片颗粒：每帧换一张噪点，柔光叠加 */
export const Grain: React.FC<{amount?: number}> = ({amount = 0.08}) => {
  const frame = useCurrentFrame();
  const {width, height} = useVideoConfig();
  return (
    <AbsoluteFill style={{opacity: amount, mixBlendMode: 'overlay', pointerEvents: 'none'}}>
      <svg width={width} height={height}>
        <filter id={`grain${frame % 8}`}>
          <feTurbulence type="fractalNoise" baseFrequency="0.85" numOctaves="2" seed={frame % 8} />
          <feColorMatrix type="saturate" values="0" />
        </filter>
        <rect width="100%" height="100%" filter={`url(#grain${frame % 8})`} />
      </svg>
    </AbsoluteFill>
  );
};

/* 暗角，外加上下两道渐变压暗，给字幕留出底色 */
export const Vignette: React.FC<{strength?: number; top?: number; bottom?: number}> = ({
  strength = 0.75, top = 0.55, bottom = 0.7,
}) => (
  <AbsoluteFill style={{pointerEvents: 'none'}}>
    <AbsoluteFill
      style={{background: `radial-gradient(ellipse 75% 60% at 50% 48%, rgba(0,0,0,0) 45%, rgba(2,4,8,${strength}) 100%)`}}
    />
    <AbsoluteFill
      style={{
        background: `linear-gradient(180deg, rgba(3,5,9,${top}) 0%, rgba(3,5,9,0) 26%, rgba(3,5,9,0) 68%, rgba(3,5,9,${bottom}) 100%)`,
      }}
    />
  </AbsoluteFill>
);

/* 重拍闪白：at 帧满白，length 帧内褪去 */
export const Flash: React.FC<{at: number; length?: number; color?: string; peak?: number}> = ({
  at, length = 10, color = '#eaf6ff', peak = 0.9,
}) => {
  const frame = useCurrentFrame();
  const k = pulse(frame, at, length) * peak;
  if (k <= 0) return null;
  return <AbsoluteFill style={{background: color, opacity: k, mixBlendMode: 'screen', pointerEvents: 'none'}} />;
};

/* 冰霜冲击波：从 (x, y) 扩散的两道冰环，外加冰晶碎片 */
export const Shockwave: React.FC<{at: number; x: number; y: number; reach?: number; length?: number}> = ({
  at, x, y, reach = 900, length = 26,
}) => {
  const frame = useCurrentFrame();
  const t = (frame - at) / length;
  if (t < 0 || t > 1) return null;
  const ease = 1 - (1 - t) ** 3;
  const rings = [0, 0.18].map((lag, i) => {
    const e = Math.max(0, ease - lag);
    const r = reach * e;
    return (
      <div
        key={i}
        style={{
          position: 'absolute',
          left: x - r,
          top: y - r,
          width: r * 2,
          height: r * 2,
          borderRadius: '50%',
          border: `${(1 - t) * (i ? 10 : 26)}px solid rgba(220,242,255,${(1 - t) * (i ? 0.5 : 0.85)})`,
          boxShadow: `0 0 ${60 * (1 - t)}px rgba(150,210,255,${0.8 * (1 - t)}), inset 0 0 ${80 * (1 - t)}px rgba(150,210,255,${0.5 * (1 - t)})`,
        }}
      />
    );
  });
  const shards = Array.from({length: 28}, (_, i) => {
    const angle = random(`a${i}`) * Math.PI * 2;
    const dist = reach * (0.2 + random(`r${i}`) * 0.9) * ease;
    const size = 8 + random(`s${i}`) * 22;
    return (
      <div
        key={`s${i}`}
        style={{
          position: 'absolute',
          left: x + Math.cos(angle) * dist - size / 2,
          top: y + Math.sin(angle) * dist - size / 2,
          width: size,
          height: size * 0.5,
          background: 'rgba(235,248,255,0.95)',
          transform: `rotate(${(angle * 180) / Math.PI + frame * 9}deg)`,
          opacity: 1 - t,
          clipPath: 'polygon(50% 0, 100% 50%, 50% 100%, 0 50%)',
          boxShadow: '0 0 10px rgba(180,225,255,0.9)',
        }}
      />
    );
  });
  return (
    <AbsoluteFill style={{pointerEvents: 'none', mixBlendMode: 'screen'}}>
      {rings}
      {shards}
    </AbsoluteFill>
  );
};

/* 冷色的夜空底：用在掌机镜头背后 */
export const NightBackdrop: React.FC<{glow?: number}> = ({glow = 1}) => (
  <AbsoluteFill
    style={{
      background: `radial-gradient(ellipse 90% 55% at 50% 42%, rgba(60,100,150,${0.45 * glow}) 0%, rgba(12,19,32,1) 55%, #04060a 100%)`,
    }}
  />
);
