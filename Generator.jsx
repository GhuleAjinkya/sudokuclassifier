import React, { useState } from 'react';

const LEVELS = [
  { id: 'simple', name: 'Simple', note: 'Obvious single-candidate moves only.' },
  { id: 'easy', name: 'Easy', note: 'Singles, with a bit more scanning.' },
  { id: 'intermediate', name: 'Intermediate', note: 'Needs pairs, triples and box/line tricks.' },
  { id: 'expert', name: 'Expert', note: 'Advanced patterns, sometimes guessing. Can take several seconds to generate.' },
];

export default function Generator({ busy, qqwingOk, hasPuzzle, onGenerate, onCancel, onEdit }) {
  const [level, setLevel] = useState('easy');
  const current = LEVELS.find((l) => l.id === level);

  return (
    <div className="panel">
      <div className="field-label" id="level-label">Difficulty to generate</div>
      <div className="segmented" role="radiogroup" aria-labelledby="level-label">
        {LEVELS.map((l) => (
          <button
            key={l.id}
            type="button"
            role="radio"
            aria-checked={level === l.id}
            className={level === l.id ? 'seg active' : 'seg'}
            onClick={() => setLevel(l.id)}
            disabled={busy}
          >
            {l.name}
          </button>
        ))}
      </div>
      <p className="muted small">{current.note}</p>

      <div className="row">
        {busy ? (
          <>
            <button type="button" className="primary" disabled>
              <span className="spinner" aria-hidden="true" /> Generating…
            </button>
            <button type="button" onClick={onCancel}>Cancel</button>
          </>
        ) : (
          <>
            <button type="button" className="primary" onClick={() => onGenerate(level)} disabled={qqwingOk === false}>
              Generate puzzle
            </button>
            {hasPuzzle && (
              <button type="button" onClick={onEdit}>Edit this puzzle</button>
            )}
          </>
        )}
      </div>
      {qqwingOk === false && (
        <p className="notice warn">qqwing could not be started, so generating is unavailable. You can still create a puzzle by hand.</p>
      )}
    </div>
  );
}
