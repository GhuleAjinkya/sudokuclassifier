import React, { useEffect, useMemo, useState } from 'react';
import { frameColor } from './replay.js';

const SPEEDS = [
  { label: 'Slow', ms: 1200 },
  { label: 'Normal', ms: 650 },
  { label: 'Fast', ms: 300 },
  { label: 'Very fast', ms: 100 },
  { label: 'Turbo', ms: 25 },
];

// One colour band per run of same-coloured frames, so even a long search draws as a single cheap element.
function runsGradient(frames) {
  if (frames.length === 0) return '#e5e7eb';
  const stops = [];
  let start = 0;
  for (let i = 1; i <= frames.length; i++) {
    if (i === frames.length || frameColor(frames[i]) !== frameColor(frames[start])) {
      const a = ((start / frames.length) * 100).toFixed(3);
      const b = ((i / frames.length) * 100).toFixed(3);
      stops.push(`${frameColor(frames[start])} ${a}% ${b}%`);
      start = i;
    }
  }
  return `linear-gradient(to right, ${stops.join(', ')})`;
}

// Playback controls only. The board and the step caption are drawn elsewhere, from the same `step`.
export default function Visualizer({ frames, step, setStep, showCandidates, setShowCandidates }) {
  const total = frames.length;
  const [playing, setPlaying] = useState(false);
  const [speed, setSpeed] = useState(1);

  // A new result starts paused.
  useEffect(() => {
    setPlaying(false);
  }, [frames]);

  useEffect(() => {
    if (!playing) return undefined;
    const id = setInterval(() => setStep((s) => Math.min(s + 1, total)), SPEEDS[speed].ms);
    return () => clearInterval(id);
  }, [playing, speed, total, setStep]);

  useEffect(() => {
    if (playing && step >= total) setPlaying(false);
  }, [playing, step, total]);

  const gradient = useMemo(() => runsGradient(frames), [frames]);
  const logicCount = frames.filter((f) => f.phase === 'logic').length;
  const searchCount = total - logicCount;
  const wrongTurns = frames.filter((f) => f.type === 'backtrack').length;

  const togglePlay = () => {
    if (playing) {
      setPlaying(false);
      return;
    }
    if (step >= total) setStep(0);
    setPlaying(true);
  };
  const go = (n) => {
    setPlaying(false);
    setStep(Math.max(0, Math.min(n, total)));
  };

  return (
    <div className="visualizer">
      <div className="viz-head">
        <h3>Solve visualizer</h3>
        <span className="muted small">step {step} / {total}</span>
      </div>

      <div className="timeline">
        <div className="timeline-bar" style={{ background: gradient }} />
        <div className="timeline-future" style={{ left: `${total ? (step / total) * 100 : 0}%` }} />
        <input
          type="range"
          min={0}
          max={total}
          value={step}
          aria-label="Position in the solve"
          onChange={(e) => go(Number(e.target.value))}
        />
      </div>
      <div className="phases muted small">
        <span style={{ flexGrow: Math.max(logicCount, 1) }}>Logic · {logicCount} steps</span>
        {searchCount > 0 && (
          <span style={{ flexGrow: Math.max(searchCount, 1) }}>
            Search · {searchCount} events{wrongTurns > 0 ? ` · ${wrongTurns} wrong turn${wrongTurns === 1 ? '' : 's'}` : ''}
          </span>
        )}
      </div>

      <div className="controls">
        <button type="button" onClick={() => go(0)} disabled={step === 0}>Reset</button>
        <button type="button" onClick={() => go(step - 1)} disabled={step === 0}>Back</button>
        <button type="button" className="primary" onClick={togglePlay}>
          {playing ? 'Pause' : step >= total ? 'Replay' : 'Play'}
        </button>
        <button type="button" onClick={() => go(step + 1)} disabled={step >= total}>Next</button>
        <button type="button" onClick={() => go(total)} disabled={step >= total}>Finish</button>
      </div>

      <div className="row between">
        <label className="check">
          <input type="checkbox" checked={showCandidates} onChange={(e) => setShowCandidates(e.target.checked)} />
          Show candidates
        </label>
        <label className="speed">
          <span className="muted small">Speed</span>
          <select value={speed} onChange={(e) => setSpeed(Number(e.target.value))}>
            {SPEEDS.map((s, i) => (
              <option key={s.label} value={i}>{s.label}</option>
            ))}
          </select>
        </label>
      </div>
    </div>
  );
}
