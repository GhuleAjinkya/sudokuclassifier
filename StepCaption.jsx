import React from 'react';
import { SEARCH_STYLE, TIER_STYLE } from './techniqueInfo.js';
import { describeFrame } from './replay.js';

// What is happening on the board right now, plus the colour key. Shown under the grid.
export default function StepCaption({ frames, step, view, hasSearch }) {
  const info = step > 0 ? describeFrame(view.frame, view.focus) : null;

  return (
    <div className="step-panel">
      <div className="caption" style={info ? { borderLeftColor: info.style.solid || info.style.color } : undefined} aria-live="polite">
        {info ? (
          <>
            <div className="caption-title">
              <span className="muted small">Step {step} of {frames.length}</span>
              <strong>{info.title}</strong>
            </div>
            <div className="chips">
              <span className="chip" style={{ color: info.style.color, backgroundColor: info.style.bg }}>{info.chip}</span>
            </div>
            {info.lines.map((line) => (
              <p key={line} className="caption-line">{line}</p>
            ))}
            {info.blurb && <p className="muted small">{info.blurb}</p>}
          </>
        ) : frames.length > 0 ? (
          <p className="muted">Press <strong>Play</strong> to watch the solver work, or step through with <strong>Next</strong>. A placed digit takes the colour of the tier of the technique that really made it possible.</p>
        ) : (
          <p className="muted">Generate or build a puzzle and press Analyze. The solver's steps will play out on this grid.</p>
        )}
      </div>

      <div className="legend" aria-label="Colour key">
        {[1, 2, 3, 4].map((t) => (
          <span key={t} className="legend-item">
            <span className="swatch" style={{ backgroundColor: TIER_STYLE[t].bg, borderColor: TIER_STYLE[t].solid }} />
            {TIER_STYLE[t].label}
          </span>
        ))}
      </div>
      {hasSearch && (
        <div className="legend" aria-label="Search colour key">
          {['guess', 'forced', 'dead', 'undo'].map((k) => (
            <span key={k} className="legend-item">
              <span className="swatch" style={{ backgroundColor: SEARCH_STYLE[k].bg, borderColor: SEARCH_STYLE[k].solid }} />
              {SEARCH_STYLE[k].label}
            </span>
          ))}
        </div>
      )}
      <p className="muted small">
        Dashed outline = the pattern's cells. A crossed-out red candidate was just removed. Faded crossed-out digits are guesses that were undone.
      </p>
    </div>
  );
}
