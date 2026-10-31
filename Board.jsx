import React, { useRef } from 'react';
import { boxOf, colOf, rowOf } from './sudokuUtils.js';
import { TIER_STYLE } from './techniqueInfo.js';

// Pure display component. The parent decides what each cell shows and what keys do.
//   values      81-char string ('.' = empty)
//   givens      boolean[81]  - original clues (drawn bold)
//   tiers       (number|null)[81] - cells filled by the visualizer, tinted by technique tier
//   conflicts   Set of cell indices that clash with a peer
//   currentCell index of the cell placed at the current visualizer step
export default function Board({ values, givens, tiers, selected, conflicts, currentCell, onSelect, onKeyDown }) {
  const ref = useRef(null);
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
    if (i === currentCell) cls.push('current');

    const tier = tiers[i];
    const style = tier != null ? { color: TIER_STYLE[tier].color, backgroundColor: TIER_STYLE[tier].bg } : undefined;
    const digit = values[i] === '.' ? '' : values[i];

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
          <span key={i === currentCell ? `now-${values[i]}` : 'static'} className="digit">
            {digit}
          </span>
        )}
      </div>
    );
  }

  return (
    <div
      ref={ref}
      className="board"
      role="grid"
      aria-label="Sudoku board"
      tabIndex={0}
      onKeyDown={onKeyDown}
    >
      {cells}
    </div>
  );
}
