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

const fmt = (n) => (Number.isInteger(n) ? String(n) : n.toFixed(1));

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

function hardestTechnique(analysis) {
  if (analysis.search) return { name: SEARCH_NAME, tier: 4 };
  let best = null;
  for (const t of analysis.techniques) {
    const tier = tierOf(t.name);
    if (!best || tier > best.tier) best = { name: t.name, tier };
  }
  return best;
}

function explain(analysis, config) {
  const r = analysis.rating;
  const hardest = hardestTechnique(analysis);
  const parts = [];
  if (!hardest) {
    parts.push('The puzzle was already complete, so no solving techniques were needed.');
  } else if (hardest.tier === 4) {
    parts.push(
      `Logic alone got stuck, so the solver had to guess. It filled ${analysis.search.cells} cells by search ` +
        `(${analysis.search.guesses} guesses, nesting depth ${analysis.search.depth}). Needing to guess adds a flat ` +
        `${fmt(config.searchBase)}-point penalty plus ${fmt(config.searchPerDepth)} points per level of nesting.`
    );
  } else if (hardest.tier === 3) {
    parts.push(`The hardest technique required was ${hardest.name}, an advanced (Tier 3) pattern. Every Tier 3 step is weighted heavily.`);
  } else if (hardest.tier === 2) {
    parts.push(`The hardest technique required was ${hardest.name}, an intermediate (Tier 2) technique. No advanced patterns or guessing were needed.`);
  } else {
    parts.push('Only single-candidate moves (naked and hidden singles) were needed, with no pairs, patterns or guessing.');
  }
  parts.push(`Overall the puzzle rates ${r.difficulty} difficulty, ${r.length} length and ${r.complexity} complexity.`);
  return parts.join(' ');
}

export default function Report({ analysis, config, analyzing }) {
  if (!analysis) {
    return (
      <div className="report-empty">
        <h2>Difficulty report</h2>
        <p className="muted">
          {analyzing
            ? 'Analyzing the puzzle…'
            : 'Generate a puzzle or build your own, then press Analyze to see how hard it is and which techniques solve it.'}
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

  const rows = analysis.techniques.map((x) => {
    const weight = config.weights[x.name] != null ? config.weights[x.name] : 1;
    return { name: x.name, tier: tierOf(x.name), steps: `${x.count}`, weightText: fmt(weight), points: weight * x.count };
  });
  if (analysis.search) {
    rows.push({
      name: SEARCH_NAME,
      tier: 4,
      steps: `${analysis.search.cells} cells`,
      weightText: `${fmt(config.searchBase)} + ${fmt(config.searchPerDepth)}/level`,
      points: config.searchBase + config.searchPerDepth * analysis.search.depth,
    });
  }
  rows.sort((a, b) => b.tier - a.tier || b.points - a.points);
  const maxPoints = Math.max(...rows.map((x) => x.points), 1);

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
          <div className="tile-sub">{r.iterations} iterations</div>
        </div>
      </div>

      <div className="facts">
        <span className={`fact ${analysis.outcome === 'search' ? 'warn' : 'ok'}`}>
          {analysis.outcome === 'search' ? 'Needed guessing (Tier 4)' : analysis.outcome === 'logic' ? 'Solved by logic alone' : 'Not fully solved'}
        </span>
        <span className={`fact ${analysis.uniqueness === 'unique' ? 'ok' : 'warn'}`}>
          {analysis.uniqueness === 'unique' ? 'Exactly one solution' : 'Multiple solutions'}
        </span>
        {hardest && (
          <span className="fact" style={{ color: TIER_STYLE[hardest.tier].color, backgroundColor: TIER_STYLE[hardest.tier].bg }}>
            Hardest: {hardest.name}
          </span>
        )}
      </div>

      {analysis.uniqueness !== 'unique' && (
        <div className="notice warn">
          This puzzle has more than one solution. The rating describes one route to a solution; a well-formed puzzle has exactly one.
        </div>
      )}

      <p className="explain">{explain(analysis, config)}</p>

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
        title="Length (steps to finish)"
        value={r.steps}
        valueText={`${r.steps} steps`}
        band={r.length}
        mid={t.length.medium}
        high={t.length.high}
        labels={['Short', 'Medium', 'Long']}
        integer
      />
      <Meter
        title="Complexity (solver iterations)"
        value={r.iterations}
        valueText={`${r.iterations}`}
        band={r.complexity}
        mid={t.complexity.medium}
        high={t.complexity.high}
        labels={['Low', 'Medium', 'High']}
        integer
      />

      <h3>What it took to solve</h3>
      {rows.length === 0 ? (
        <p className="muted">No techniques were needed.</p>
      ) : (
        <div className="table-wrap">
          <table className="techniques">
            <thead>
              <tr>
                <th>Technique</th>
                <th className="num">Steps</th>
                <th className="num">Weight</th>
                <th>Points</th>
              </tr>
            </thead>
            <tbody>
              {rows.map((row) => (
                <tr key={row.name}>
                  <td>
                    <span className="dot" style={{ backgroundColor: TIER_STYLE[row.tier].color }} aria-hidden="true" />
                    {row.name}
                    <div className="muted small">{TIER_STYLE[row.tier].label}</div>
                  </td>
                  <td className="num">{row.steps}</td>
                  <td className="num">{row.weightText}</td>
                  <td>
                    <div className="points">
                      <div className="bar" style={{ width: `${(row.points / maxPoints) * 100}%`, backgroundColor: TIER_STYLE[row.tier].color }} />
                      <span>{row.points.toFixed(2)}</span>
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
            <tfoot>
              <tr>
                <td colSpan={3}>Difficulty score</td>
                <td><strong>{fmt(r.score)}</strong></td>
              </tr>
            </tfoot>
          </table>
        </div>
      )}
      <p className="muted small">Weights and thresholds come from ratings.cfg. Edit that file to re-tune the ratings.</p>
    </div>
  );
}
