import React from 'react';
import { SEARCH_NAME, TIER_STYLE, tierOf } from './techniqueInfo.js';

// ---- band colours: green / amber / red ----
const BAND = {
  Low: { color: '#15803d', bg: '#dcfce7' },
  Short: { color: '#15803d', bg: '#dcfce7' },
  Medium: { color: '#b45309', bg: '#fef3c7' },
  High: { color: '#b91c1c', bg: '#fee2e2' },
  Long: { color: '#b91c1c', bg: '#fee2e2' },
};
const ZONE_COLORS = ['#bbf7d0', '#fde68a', '#fecaca'];

export const fmt = (n) => (Number.isInteger(n) ? String(n) : n.toFixed(1));

export function hardestTechnique(analysis) {
  if (analysis.search) return { name: SEARCH_NAME, tier: 4 };
  let best = null;
  for (const t of analysis.techniques) {
    const tier = tierOf(t.name);
    if (!best || tier > best.tier) best = { name: t.name, tier };
  }
  return best;
}

// Where to draw the marker. The three zones get equal width so that every band is readable
// no matter how narrow it is numerically.
function markerPosition(value, mid, high) {
  if (value < mid) return (mid > 0 ? value / mid : 0) / 3;
  if (value < high) return (1 + (value - mid) / (high - mid || 1)) / 3;
  const span = Math.max(high - mid, high * 0.25, 1);
  return (2 + Math.min((value - high) / span, 1)) / 3;
}

function Meter({ title, value, valueText, band, mid, high, labels, integer }) {
  const pos = Math.max(0, Math.min(1, markerPosition(value, mid, high))) * 100;
  const midText = fmt(mid);
  const mediumText = integer ? (high - 1 > mid ? `${midText}–${fmt(high - 1)}` : midText) : `${midText}–${fmt(high)}`;
  return (
    <div className="meter">
      <div className="meter-head">
        <span className="meter-title">{title}</span>
        <span className="band-pill" style={{ color: BAND[band].color, backgroundColor: BAND[band].bg }}>
          {band} · {valueText}
        </span>
      </div>
      <div className="meter-track" role="img" aria-label={`${title}: ${band} (${valueText})`}>
        {ZONE_COLORS.map((c) => (
          <div key={c} className="zone" style={{ backgroundColor: c }} />
        ))}
        <div className="marker" style={{ left: `${pos}%` }} />
      </div>
      <div className="meter-zones muted small">
        <span>{labels[0]} &lt; {midText}</span>
        <span>{labels[1]} {mediumText}</span>
        <span>{labels[2]} ≥ {fmt(high)}</span>
      </div>
    </div>
  );
}

// The compact difficulty report: three ratings, a few facts, and where each sits against its thresholds.
export default function Report({ analysis, config, analyzing }) {
  if (!analysis) {
    return (
      <div className="report-empty">
        <h2>Difficulty report</h2>
        <p className="muted">
          {analyzing
            ? 'Analyzing the puzzle…'
            : 'Generate a puzzle or build your own, then press Analyze to see how hard it is.'}
        </p>
        {analyzing && <span className="spinner big" aria-hidden="true" />}
      </div>
    );
  }

  if (analysis.outcome === 'invalid') {
    return (
      <div>
        <h2>Difficulty report</h2>
        <div className="notice error">
          <strong>No solution.</strong> This puzzle is contradictory or cannot be completed, so it can't be rated. Check for a mistake in the clues.
        </div>
      </div>
    );
  }

  const r = analysis.rating;
  const t = config.thresholds;
  const hardest = hardestTechnique(analysis);

  return (
    <div>
      <h2>Difficulty report</h2>

      <div className="tiles">
        <div className="tile" style={{ backgroundColor: BAND[r.difficulty].bg, color: BAND[r.difficulty].color }}>
          <div className="tile-label">Difficulty</div>
          <div className="tile-value">{r.difficulty}</div>
          <div className="tile-sub">score {fmt(r.score)}</div>
        </div>
        <div className="tile" style={{ backgroundColor: BAND[r.length].bg, color: BAND[r.length].color }}>
          <div className="tile-label">Length</div>
          <div className="tile-value">{r.length}</div>
          <div className="tile-sub">{r.steps} steps</div>
        </div>
        <div className="tile" style={{ backgroundColor: BAND[r.complexity].bg, color: BAND[r.complexity].color }}>
          <div className="tile-label">Complexity</div>
          <div className="tile-value">{r.complexity}</div>
          <div className="tile-sub">{r.iterations} iter.</div>
        </div>
      </div>

      <div className="facts">
        <span className={`fact ${analysis.outcome === 'search' ? 'warn' : 'ok'}`}>
          {analysis.outcome === 'search' ? 'Needed guessing' : analysis.outcome === 'logic' ? 'Solved by logic' : 'Not fully solved'}
        </span>
        <span className={`fact ${analysis.uniqueness === 'unique' ? 'ok' : 'warn'}`}>
          {analysis.uniqueness === 'unique' ? 'One solution' : 'Multiple solutions'}
        </span>
        {hardest && (
          <span className="fact" style={{ color: TIER_STYLE[hardest.tier].color, backgroundColor: TIER_STYLE[hardest.tier].bg }}>
            Hardest: {hardest.name}
          </span>
        )}
      </div>

      {analysis.uniqueness !== 'unique' && (
        <div className="notice warn">
          More than one solution exists. The rating describes one route to a solution; a well-formed puzzle has exactly one.
        </div>
      )}

      <Meter
        title="Difficulty score"
        value={r.score}
        valueText={fmt(r.score)}
        band={r.difficulty}
        mid={t.difficulty.medium}
        high={t.difficulty.high}
        labels={['Low', 'Medium', 'High']}
      />
      <Meter
        title="Length (steps)"
        value={r.steps}
        valueText={`${r.steps}`}
        band={r.length}
        mid={t.length.medium}
        high={t.length.high}
        labels={['Short', 'Medium', 'Long']}
        integer
      />
      <Meter
        title="Complexity (iterations)"
        value={r.iterations}
        valueText={`${r.iterations}`}
        band={r.complexity}
        mid={t.complexity.medium}
        high={t.complexity.high}
        labels={['Low', 'Medium', 'High']}
        integer
      />
    </div>
  );
}
