import React, { useEffect, useState } from 'react';
import { countGivens, isPuzzleString, normalize } from './sudokuUtils.js';

export default function Editor({ puzzle, selected, canEdit, conflictCount, canUndo, onSetCell, onUndo, onClear, onLoad }) {
  const [text, setText] = useState(puzzle);
  const [loadError, setLoadError] = useState('');
  const [copied, setCopied] = useState(false);

  // Keep the text box in step with the board.
  useEffect(() => {
    setText(puzzle);
    setLoadError('');
  }, [puzzle]);

  const place = (ch) => {
    if (selected != null && canEdit) onSetCell(selected, ch);
  };

  const load = () => {
    const n = normalize(text);
    if (!isPuzzleString(n)) {
      setLoadError(`Need exactly 81 characters (digits 1–9, with . or 0 for blanks). You have ${n.length}.`);
      return;
    }
    setLoadError('');
    onLoad(n);
  };

  const copy = async () => {
    try {
      await navigator.clipboard.writeText(puzzle);
      setCopied(true);
      setTimeout(() => setCopied(false), 1500);
    } catch {
      setLoadError('Copy is not available in this browser. Select the text and copy it manually.');
    }
  };

  return (
    <div className="panel">
      <div className="row between">
        <div className="field-label">Click a cell, then type 1–9 (Backspace clears)</div>
        <div className="muted small">Clues: {countGivens(puzzle)} / 81</div>
      </div>

      <div className="numpad" role="group" aria-label="Number pad">
        {[1, 2, 3, 4, 5, 6, 7, 8, 9].map((n) => (
          <button key={n} type="button" onClick={() => place(String(n))} disabled={!canEdit || selected == null}>
            {n}
          </button>
        ))}
        <button type="button" className="wide" onClick={() => place('.')} disabled={!canEdit || selected == null}>
          Erase
        </button>
      </div>

      <div className="row">
        <button type="button" onClick={onUndo} disabled={!canUndo || !canEdit}>Undo</button>
        <button type="button" onClick={onClear} disabled={!canEdit || countGivens(puzzle) === 0}>Clear all</button>
      </div>

      {!canEdit && <p className="notice info">The visualizer is showing a replay. Press <strong>Reset</strong> under the board to edit again.</p>}
      {conflictCount > 0 && <p className="notice error">Cells in red repeat a digit in their row, column or box.</p>}

      <label className="field-label" htmlFor="puzzle-text">Puzzle string (81 characters)</label>
      <div className="row">
        <input
          id="puzzle-text"
          className="mono"
          value={text}
          spellCheck={false}
          onChange={(e) => setText(e.target.value)}
          onKeyDown={(e) => {
            if (e.key === 'Enter') load();
          }}
        />
        <button type="button" onClick={load} disabled={!canEdit}>Load</button>
        <button type="button" onClick={copy}>{copied ? 'Copied' : 'Copy'}</button>
      </div>
      {loadError && <p className="notice error">{loadError}</p>}
    </div>
  );
}
