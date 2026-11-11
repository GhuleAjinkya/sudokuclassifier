import React from 'react';
import { SEARCH_NAME, TIER_STYLE, reasonOf, tierOf } from './techniqueInfo.js';
import { fmt } from './Report.jsx';

// "What it took to solve": where every digit came from, and every technique used with its weight and
// its share of the difficulty score.
export default function Took({ analysis, config }) {
  if (!analysis || analysis.outcome === 'invalid') {
    return (
      <div className="report-empty">
        <h2>What it took to solve</h2>
        <p className="muted">The techniques used, their weights and the guessing involved will appear here after an analysis.</p>
      </div>
    );
  }

  const r = analysis.rating;
  const logicSteps = analysis.steps.filter((s) => s.method !== SEARCH_NAME);
  const logicPlacements = logicSteps.filter((s) => s.kind === 'placement');
  const eliminations = logicSteps.length - logicPlacements.length;
  const search = analysis.search;

  // Where each digit really came from: the technique that made it possible, then the single that wrote it.
  const origin = new Map();
  for (const s of logicPlacements) {
    const label = s.root ? `${s.root} → ${s.method}` : s.method;
    const entry = origin.get(label) || { label, tier: tierOf(reasonOf(s)), count: 0 };
    entry.count += 1;
    origin.set(label, entry);
  }
  if (search) origin.set(SEARCH_NAME, { label: SEARCH_NAME, tier: 4, count: search.cells });
  const origins = [...origin.values()].sort((a, b) => b.tier - a.tier || b.count - a.count);

  const rows = analysis.techniques.map((x) => {
    const weight = config.weights[x.name] != null ? config.weights[x.name] : 1;
    return { name: x.name, tier: tierOf(x.name), steps: `${x.count}`, weightText: fmt(weight), points: weight * x.count };
  });
  if (search) {
    rows.push({
      name: SEARCH_NAME,
      tier: 4,
      steps: `${search.cells} cells`,
      weightText: `${fmt(config.searchBase)} + ${fmt(config.searchPerDepth)}/level`,
      points: config.searchBase + config.searchPerDepth * search.depth,
    });
  }
  rows.sort((a, b) => b.tier - a.tier || b.points - a.points);
  const maxPoints = Math.max(...rows.map((x) => x.points), 1);

  return (
    <div>
      <h2>What it took to solve</h2>

      <div className="stat-row">
        <div className="stat"><strong>{logicSteps.length}</strong><span className="muted small">logic steps</span></div>
        <div className="stat"><strong>{logicPlacements.length}</strong><span className="muted small">placements</span></div>
        <div className="stat"><strong>{eliminations}</strong><span className="muted small">eliminations</span></div>
        {search && <div className="stat"><strong>{search.guesses}</strong><span className="muted small">guesses</span></div>}
        {search && <div className="stat"><strong>{search.deadEnds}</strong><span className="muted small">wrong turns</span></div>}
        {search && <div className="stat"><strong>{search.depth}</strong><span className="muted small">max depth</span></div>}
      </div>

      {origins.length > 0 && (
        <>
          <h3>Where the digits came from</h3>
          <p className="muted small">Each placed digit is credited to the technique that made it possible, followed by the single that wrote it down.</p>
          <ul className="origins">
            {origins.map((o) => (
              <li key={o.label}>
                <span className="dot" style={{ backgroundColor: TIER_STYLE[o.tier].solid }} aria-hidden="true" />
                <span className="origin-label">{o.label}</span>
                <span className="origin-count">{o.count}</span>
              </li>
            ))}
          </ul>
        </>
      )}

      <h3>Techniques and their weight</h3>
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
                    <span className="dot" style={{ backgroundColor: TIER_STYLE[row.tier].solid }} aria-hidden="true" />
                    {row.name}
                    <div className="muted small">{TIER_STYLE[row.tier].label}</div>
                  </td>
                  <td className="num">{row.steps}</td>
                  <td className="num">{row.weightText}</td>
                  <td>
                    <div className="points">
                      <div className="bar" style={{ width: `${(row.points / maxPoints) * 100}%`, backgroundColor: TIER_STYLE[row.tier].solid }} />
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
