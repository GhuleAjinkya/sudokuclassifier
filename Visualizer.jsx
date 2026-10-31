import React, { useEffect, useState } from 'react';
import { SEARCH_NAME, TIER_STYLE, blurbOf, tierOf } from './techniqueInfo.js';

const SPEEDS = [
  { label: 'Slow', ms: 1200 },
  { label: 'Normal', ms: 650 },
  { label: 'Fast', ms: 300 },
  { label: 'Very fast', ms: 100 },
];

// Replays the solver's placements one at a time. The board itself is drawn by the parent;
// this component owns the playback controls and the explanation of the current step.
export default function Visualizer({ analysis, audit, step, setStep }) {
  const placements = analysis.placements;
  const total = placements.length;
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(1);

  // A new analysis starts paused.
  useEffect(() => {
    setPlaying(false);
  }, [analysis]);

  useEffect(() => {
    if (!playing) return undefined;
    const id = setInterval(() => setStep((s) => Math.min(s + 1, total)), SPEEDS[speed].ms);
    return () => clearInterval(id);
  }, [playing, speed, total, setStep]);

  useEffect(() => {
    if (playing && step >= total) setPlaying(false);
  }, [playing, step, total]);

  const togglePlay = () => {
    if (playing) {
      setPlaying(false);
      return;
    }
    if (step >= total) setStep(0);
    setPlaying(true);
  };

  const current = step > 0 ? placements[step - 1] : null;
  const tier = current ? tierOf(current.method) : null;
  const tiersPresent = [...new Set(placements.map((p) => tierOf(p.method)))].sort();
  const flagged = current ? audit[step - 1] : null;
  const flaggedCount = audit.filter(Boolean).length;

  return (
    <div className="visualizer">
      <div className="viz-head">
        <h3>Solve visualizer</h3>
        <span className="muted small">{step} / {total} placements</span>
      </div>

      <div className="timeline" role="group" aria-label="Timeline of placements">
        {placements.map((p, i) => {
          const t = TIER_STYLE[tierOf(p.method)];
          return (
            <button
              key={i}
              type="button"
              className={`tick${i < step ? ' done' : ''}${i === step - 1 ? ' now' : ''}${audit[i] ? ' flag' : ''}`}
              style={{ backgroundColor: t.color }}
              title={`Step ${i + 1}: ${p.method}${audit[i] ? ' (needed earlier eliminations)' : ''}`}
              aria-label={`Go to step ${i + 1}`}
              onClick={() => {
                setPlaying(false);
                setStep(i + 1);
              }}
            />
          );
        })}
      </div>

      <div className="controls">
        <button type="button" onClick={() => { setPlaying(false); setStep(0); }} disabled={step === 0}>Reset</button>
        <button type="button" onClick={() => { setPlaying(false); setStep(Math.max(step - 1, 0)); }} disabled={step === 0}>Back</button>
        <button type="button" className="primary" onClick={togglePlay}>
          {playing ? 'Pause' : step >= total ? 'Replay' : 'Play'}
        </button>
        <button type="button" onClick={() => { setPlaying(false); setStep(Math.min(step + 1, total)); }} disabled={step >= total}>Next</button>
        <button type="button" onClick={() => { setPlaying(false); setStep(total); }} disabled={step >= total}>Finish</button>
        <label className="speed">
          <span className="muted small">Speed</span>
          <select value={speed} onChange={(e) => setSpeed(Number(e.target.value))}>
            {SPEEDS.map((s, i) => (
              <option key={s.label} value={i}>{s.label}</option>
            ))}
          </select>
        </label>
      </div>

      <div className="caption" aria-live="polite">
        {current ? (
          <>
            <div>
              <strong>Step {step}:</strong> placed <strong>{current.digit}</strong> at row {current.row + 1}, column {current.col + 1}
            </div>
            <div className="chips">
              <span className="chip" style={{ color: TIER_STYLE[tier].color, backgroundColor: TIER_STYLE[tier].bg }}>
                {current.method}
              </span>
              <span className="muted small">{TIER_STYLE[tier].label}</span>
            </div>
            <p className="muted small">{blurbOf(current.method)}</p>
            {flagged && (
              <p className="notice info">
                Heads-up: on a plain grid this cell still has candidates {flagged.candidates.join(', ')}. It only became a{' '}
                {current.method.toLowerCase()} after earlier eliminations (pairs, pointing/claiming or wing patterns) that the
                solver's placement log does not print.
              </p>
            )}
            {current.method === SEARCH_NAME && (
              <p className="muted small">
                The solver does not record the order of its guesses, so cells filled by the search phase appear in reading order.
              </p>
            )}
          </>
        ) : (
          <p className="muted">Press <strong>Play</strong> to watch the solver fill in {total} cells, or step through with <strong>Next</strong>.</p>
        )}
      </div>

      <div className="legend">
        {tiersPresent.map((t) => (
          <span key={t} className="legend-item">
            <span className="swatch" style={{ backgroundColor: TIER_STYLE[t].bg, borderColor: TIER_STYLE[t].color }} />
            {TIER_STYLE[t].label}
          </span>
        ))}
      </div>
      <p className="muted small">
        The visualizer shows digits being placed. Candidate eliminations that happen between placements are not shown.
        {flaggedCount > 0 &&
          ` ${flaggedCount} of ${total} placements depended on such eliminations; their ticks have a pale bottom edge.`}
      </p>
    </div>
  );
}
