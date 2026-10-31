import React from 'react';

/* ESP-Mosaico 掌机：方形米白机身、贴边的方屏、屏幕外一圈橙色细边。
 * 屏幕四角按游戏的 FL_CORNER_RADIUS（480 里的 60）切圆。 */
const BEZEL = 0.045;       // 边框占机身宽度
const BODY_RADIUS = 0.11;
const SCREEN_RADIUS = 60 / 480;

export const screenSize = (width: number) => width * (1 - 2 * BEZEL);

type Props = {
  width: number;
  /* 渲染 w x w 的屏幕内容 */
  screen: (w: number) => React.ReactNode;
  /* 透视倾斜（度）与屏幕亮度 */
  tiltX?: number;
  tiltY?: number;
  glow?: number;
  style?: React.CSSProperties;
};

export const Device: React.FC<Props> = ({width, screen, tiltX = 0, tiltY = 0, glow = 1, style}) => {
  const s = screenSize(width);
  const inset = width * BEZEL;
  return (
    <div style={{perspective: 2400, ...style}}>
      <div
        style={{
          position: 'relative',
          width,
          height: width,
          borderRadius: width * BODY_RADIUS,
          transform: `rotateX(${tiltX}deg) rotateY(${tiltY}deg)`,
          background: 'linear-gradient(150deg, #f4f0e8 0%, #e2ddd2 45%, #cbc5b8 100%)',
          boxShadow: [
            `0 ${width * 0.06}px ${width * 0.12}px rgba(0,0,0,0.65)`,
            `0 0 ${width * 0.18 * glow}px rgba(140,200,255,${0.22 * glow})`,
            'inset 0 2px 0 rgba(255,255,255,0.9)',
            'inset 0 -3px 6px rgba(0,0,0,0.18)',
          ].join(', '),
        }}
      >
        {/* 屏幕外的橙色细边 */}
        <div
          style={{
            position: 'absolute',
            left: inset - 3,
            top: inset - 3,
            width: s + 6,
            height: s + 6,
            borderRadius: s * SCREEN_RADIUS + 3,
            background: '#ff5a1f',
            boxShadow: '0 0 8px rgba(255,90,31,0.5)',
          }}
        />
        <div
          style={{
            position: 'absolute',
            left: inset,
            top: inset,
            width: s,
            height: s,
            borderRadius: s * SCREEN_RADIUS,
            overflow: 'hidden',
            background: '#000',
          }}
        >
          {screen(s)}
          {/* 玻璃：一道斜向的微光 */}
          <div
            style={{
              position: 'absolute',
              inset: 0,
              background:
                'linear-gradient(125deg, rgba(255,255,255,0.20) 0%, rgba(255,255,255,0.04) 34%, rgba(255,255,255,0) 55%)',
              pointerEvents: 'none',
            }}
          />
        </div>
      </div>
    </div>
  );
};
