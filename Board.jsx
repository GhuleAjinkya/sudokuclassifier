import React, { useRef } from 'react';
import { boxOf, colOf, rowOf } from './sudokuUtils.js';
import { SEARCH_STYLE, TIER_STYLE } from './techniqueInfo.js';

const DIGITS = [1, 2, 3, 4, 5, 6, 7, 8, 9];
const NO_FOCUS = { cells: new Set(), pattern: new Set(), strike: new Map(), ghosts: new Map(), dead: new Set(), keyDigits: new Set(), color: null };

// How a cell filled during the replay is coloured: by tier of its real reason for logic steps, red tones for the search.
function fillStyle(how) {
  if (!how) return undefined;
  if (how.source === 'guess') return { color: SEARCH_STYLE.guess.color, backgroundColor: SEARCH_STYLE.guess.bg };
  if (how.source === 'forced') return { color: SEARCH_STYLE.forced.color, backgroundColor: SEARCH_STYLE.forced.bg };
  const t = TIER_STYLE[how.tier];
  return { color: t.color, backgroundColor: t.bg };
}

// Draws the grid and nothing else. The parent decides what each cell shows and what the keys do.
//   values       81-char string ('.' = empty)
//   givens       boolean[81]: original clues (drawn bold)
//   conflicts    Set of cell indices that clash with a peer
//   view         optional replay state: { masks, how, focus, showCandidates }
export default function Board({ values, givens, selected, conflicts, view, onSelect, onKeyDown }) {
  const ref = useRef(null);
  const focus = view ? view.focus : NO_FOCUS;
  const showCands = Boolean(view && view.showCandidates);
  const selectedDigit = selected != null && values[selected] !== '.' ? values[selected] : null;

  const cells = [];
  for (let i = 0; i < 81; i++) {
    const r = rowOf(i);
    const c = colOf(i);
    const cls = ['cell'];
    if (c % 3 === 2 && c !== 8) cls.push('box-right');
    if (r % 3 === 2 && r !== 8) cls.push('box-bottom');
    if (c === 8) cls.push('last-col');
    if (r === 8) cls.push('last-row');
    if (givens[i]) cls.push('given');
    if (selected != null) {
      if (i === selected) cls.push('selected');
      else if (rowOf(selected) === r || colOf(selected) === c || boxOf(selected) === boxOf(i)) cls.push('peer');
    }
    if (selectedDigit && values[i] === selectedDigit && i !== selected) cls.push('same-digit');
    if (conflicts.has(i)) cls.push('conflict');
    if (focus.cells.has(i)) cls.push('current');
    if (focus.pattern.has(i)) cls.push('pattern');
    if (focus.dead.has(i)) cls.push('dead');

    const how = view ? view.how[i] : null;
    const style = { ...fillStyle(how), '--focus': focus.color || '#111827' };
    const digit = values[i] === '.' ? '' : values[i];
    const strikes = focus.strike.get(i);
    const ghost = focus.ghosts.get(i);
    const inPattern = focus.pattern.has(i);

    cells.push(
      <div
        key={i}
        role="gridcell"
        aria-selected={i === selected}
        aria-label={`Row ${r + 1}, column ${c + 1}, ${digit || 'empty'}`}
        className={cls.join(' ')}
        style={style}
        onClick={() => {
          onSelect(i);
          if (ref.current) ref.current.focus();
        }}
      >
        {digit && (
          <span key={focus.cells.has(i) ? `now-${digit}` : 'static'} className="digit">
            {digit}
          </span>
        )}
        {!digit && showCands && (
          <div className="cands" aria-hidden="true">
            {DIGITS.map((d) => {
              const present = (view.masks[i] >> (d - 1)) & 1;
              const struck = strikes && strikes.has(d);
              const key = inPattern && focus.keyDigits.has(d);
              return (
                <span key={d} className={`cand${struck ? ' struck' : ''}${key ? ' key' : ''}`}>
                  {present || struck ? d : ''}
                </span>
              );
            })}
          </div>
        )}
        {!digit && ghost && <span className="ghost" title="This guess was undone">{ghost}</span>}
      </div>
    );
  }

  return (
    <div ref={ref} className="board" role="grid" aria-label="Sudoku board" tabIndex={0} onKeyDown={onKeyDown}>
      {cells}
    </div>
  );
}
