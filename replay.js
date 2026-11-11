// Rebuilds what the board looks like at any point of a solve, from the solver's JSON.
//
// A solve is a list of "frames":
//   logic frames   one per Tier 1-3 step: a placement, or an elimination (with the pattern behind it)
//   search frames  one per event of the backtracking trace: guess, forced, contradiction, backtrack, solved
//
// Each logic step also says what it relied on: a placement that an XY-Wing unlocked carries root "XY-Wing"
// and points at the XY-Wing step. The board colours that digit by its REAL reason (Tier 3), not by the
// single that happened to write it down.
//
// replay() applies the first N frames to the puzzle and returns the digits on the board, the candidate
// ("pencil mark") masks and how each cell was filled. Wrong guesses are really undone: a backtrack frame
// restores the snapshot taken when the guess was made.
import { SEARCH_NAME, SEARCH_STYLE, TIER_STYLE, blurbOf, reasonOf, tierOf } from './techniqueInfo.js';
import { sees } from './sudokuUtils.js';

const PEERS = Array.from({ length: 81 }, (_, i) => {
  const out = [];
  for (let j = 0; j < 81; j++) if (sees(i, j)) out.push(j);
  return out;
});
const idx = (r, c) => r * 9 + c;
const cellOf = (r, c) => `r${r + 1}c${c + 1}`;
const plural = (n, word) => `${n} ${word}${n === 1 ? '' : 's'}`;
const capitalize = (s) => (s ? s[0].toUpperCase() + s.slice(1) : s);

export function buildFrames(analysis) {
  const frames = [];
  analysis.steps.forEach((s, k) => {
    if (s.method === SEARCH_NAME) return; // the search is replayed from its trace, wrong turns included
    // enabledBy indexes analysis.steps; logic steps come first, so a step's index is also its frame index.
    const relies = (s.enabledBy || []).map((j) => ({ step: j + 1, method: analysis.steps[j].method }));
    frames.push({ phase: 'logic', index: k, relies, ...s });
  });
  const search = analysis.search;
  if (search) {
    for (const e of search.trace) frames.push({ phase: 'search', ...e });
    if (search.traceTruncated) frames.push({ phase: 'search', type: 'truncated' });
  }
  return frames;
}

// The tier a logic frame is shown in: a placement by its real reason, an elimination by its own technique.
const frameTier = (f) => tierOf(f.kind === 'placement' ? reasonOf(f) : f.method);

// Colour of a frame on the timeline.
export function frameColor(f) {
  if (f.phase === 'logic') return TIER_STYLE[frameTier(f)].solid;
  switch (f.type) {
    case 'guess': return SEARCH_STYLE.guess.solid;
    case 'forced': return SEARCH_STYLE.forced.solid;
    case 'contradiction': return SEARCH_STYLE.dead.solid;
    case 'backtrack': return SEARCH_STYLE.undo.solid;
    case 'solved': return SEARCH_STYLE.solved.solid;
    default: return SEARCH_STYLE.cut.solid;
  }
}

function plainState(puzzle) {
  const val = puzzle.split('').map((ch) => (ch === '.' ? 0 : Number(ch)));
  const mask = new Array(81).fill(0);
  for (let i = 0; i < 81; i++) {
    if (val[i]) continue;
    let m = 0x1ff;
    for (const j of PEERS[i]) if (val[j]) m &= ~(1 << (val[j] - 1));
    mask[i] = m;
  }
  return { val, mask, how: new Array(81).fill(null) };
}

export function replay(puzzle, frames, count, solution) {
  let { val, mask, how } = plainState(puzzle);
  const stack = []; // one snapshot per active guess

  const place = (i, d, h) => {
    val[i] = d;
    mask[i] = 0;
    how[i] = h;
    const bit = ~(1 << (d - 1));
    for (const j of PEERS[i]) mask[j] &= bit;
  };

  for (let n = 0; n < count && n < frames.length; n++) {
    const f = frames[n];
    if (f.phase === 'logic') {
      if (f.kind === 'placement') place(idx(f.row, f.col), f.digit, { tier: frameTier(f), source: 'logic' });
      else for (const [r, c, d] of f.removed) mask[idx(r, c)] &= ~(1 << (d - 1));
    } else if (f.type === 'guess') {
      stack.push({ val: val.slice(), mask: mask.slice(), how: how.slice() });
      place(idx(f.row, f.col), f.digit, { tier: 4, source: 'guess' });
    } else if (f.type === 'forced') {
      place(idx(f.row, f.col), f.digit, { tier: 4, source: 'forced' });
    } else if (f.type === 'backtrack') {
      const snap = stack.pop();
      if (snap) ({ val, mask, how } = snap);
    } else if (f.type === 'truncated' && solution) {
      // The recorded trace stopped early: jump to the final answer.
      stack.length = 0;
      for (let i = 0; i < 81; i++) {
        const d = Number(solution[i]);
        if (val[i] !== d) how[i] = { tier: 4, source: 'forced' };
        val[i] = d;
        mask[i] = 0;
      }
    }
  }
  return { val, mask, how, depth: stack.length };
}

function unitCells(kind, index) {
  const out = [];
  for (let k = 0; k < 9; k++) {
    if (kind === 'row') out.push(idx(index, k));
    else if (kind === 'column') out.push(idx(k, index));
    else out.push(idx(Math.floor(index / 3) * 3 + Math.floor(k / 3), (index % 3) * 3 + (k % 3)));
  }
  return out;
}

// Everything the board and caption need for "the board after `step` frames".
export function frameView(puzzle, frames, step, solution) {
  const cur = replay(puzzle, frames, step, solution);
  const frame = step > 0 ? frames[step - 1] : null;
  const focus = { cells: new Set(), pattern: new Set(), strike: new Map(), ghosts: new Map(), dead: new Set(), keyDigits: new Set(), color: null };

  if (frame) {
    focus.color = frameColor(frame);
    if (frame.phase === 'logic') {
      if (frame.kind === 'placement') {
        focus.cells.add(idx(frame.row, frame.col));
        // Outline the pattern of the elimination this digit relies on (e.g. the XY-Wing's three cells).
        for (const rel of frame.relies) {
          const src = frames[rel.step - 1];
          if (!src || !src.pattern) continue;
          for (const [r, c] of src.pattern) focus.pattern.add(idx(r, c));
          for (const d of src.digits) focus.keyDigits.add(d);
        }
      } else {
        for (const [r, c] of frame.pattern) focus.pattern.add(idx(r, c));
        for (const d of frame.digits) focus.keyDigits.add(d);
        for (const [r, c, d] of frame.removed) {
          const i = idx(r, c);
          if (!focus.strike.has(i)) focus.strike.set(i, new Set());
          focus.strike.get(i).add(d);
        }
      }
    } else if (frame.type === 'guess' || frame.type === 'forced') {
      focus.cells.add(idx(frame.row, frame.col));
    } else if (frame.type === 'contradiction') {
      if (frame.row >= 0) focus.dead.add(idx(frame.row, frame.col));
      if (frame.unit) for (const i of unitCells(frame.unit, frame.unitIndex)) focus.dead.add(i);
    } else if (frame.type === 'backtrack') {
      const prev = replay(puzzle, frames, step - 1, solution);
      for (let i = 0; i < 81; i++) if (prev.val[i] && !cur.val[i]) focus.ghosts.set(i, prev.val[i]);
    }
  }

  return {
    values: cur.val.map((d) => (d ? String(d) : '.')).join(''),
    masks: cur.mask,
    how: cur.how,
    depth: cur.depth,
    frame,
    focus,
  };
}

// "relies on step 12 (XY-Wing), step 9 (Naked Pair)" for a logic frame, or ''.
const reliesText = (f) => (f.relies && f.relies.length ? `Relies on ${f.relies.map((r) => `step ${r.step} (${r.method})`).join(', ')}.` : '');

// "Naked Pair → XY-Wing" for the full chain, compressing repeats ("Naked Pair ×2").
export function chainText(chain) {
  const parts = [];
  for (let i = 0; i < chain.length; ) {
    let j = i;
    while (j < chain.length && chain[j] === chain[i]) j++;
    parts.push(j - i > 1 ? `${chain[i]} ×${j - i}` : chain[i]);
    i = j;
  }
  return parts.join(' → ');
}

// Plain-English caption for a frame.
export function describeFrame(frame, focus) {
  if (!frame) return null;
  if (frame.phase === 'logic') {
    const style = TIER_STYLE[frameTier(frame)];
    if (frame.kind === 'placement') {
      const lines = [];
      if (frame.root) {
        lines.push(`This digit only became possible because of ${frame.root}, which turned the cell into a ${frame.method.toLowerCase()}.`);
        lines.push(reliesText(frame));
        if (frame.chain.length > frame.relies.length) lines.push(`Full chain: ${chainText(frame.chain)} → ${frame.method}.`);
      } else if (frame.detail) {
        lines.push(capitalize(frame.detail));
      }
      return {
        title: `Placed ${frame.digit} at ${cellOf(frame.row, frame.col)}`,
        chip: frame.root ? `${frame.root} → ${frame.method}` : frame.method,
        style,
        lines: lines.filter(Boolean),
        blurb: blurbOf(frame.root || frame.method),
      };
    }
    const byDigit = new Map();
    for (const [r, c, d] of frame.removed) {
      if (!byDigit.has(d)) byDigit.set(d, []);
      byDigit.get(d).push(cellOf(r, c));
    }
    const removed = [...byDigit].map(([d, cells]) => `${d} from ${cells.join(', ')}`).join('; ');
    return {
      title: `Removed ${plural(frame.removed.length, 'candidate')}`,
      chip: frame.method,
      style,
      lines: [
        `Pattern: ${frame.pattern.map(([r, c]) => cellOf(r, c)).join(', ')} (${plural(frame.digits.length, 'digit')} ${frame.digits.join(', ')})${frame.detail ? ` · ${frame.detail}` : ''}`,
        `Removed: ${removed}`,
        reliesText(frame),
      ].filter(Boolean),
      blurb: blurbOf(frame.method),
    };
  }

  switch (frame.type) {
    case 'guess': {
      const first = frame.options[0] === frame.digit;
      return {
        title: `Guess ${frame.digit} at ${cellOf(frame.row, frame.col)}`,
        chip: 'Guess',
        style: SEARCH_STYLE.guess,
        lines: [
          `This cell could hold ${frame.options.join(' or ')}. ${first ? 'Trying the first option' : 'The earlier option led to a dead end, so trying the next one'} (guess depth ${frame.depth}).`,
        ],
        blurb: 'Nothing on the board forces a digit here, so the solver picks one and follows the consequences.',
      };
    }
    case 'forced':
      return {
        title: `Forced: ${frame.digit} at ${cellOf(frame.row, frame.col)}`,
        chip: frame.reason,
        style: SEARCH_STYLE.forced,
        lines: [`Given the guesses so far, this cell is a ${frame.reason.toLowerCase()} (guess depth ${frame.depth}).`],
        blurb: '',
      };
    case 'contradiction':
      return {
        title: 'Dead end',
        chip: 'Contradiction',
        style: SEARCH_STYLE.dead,
        lines: [capitalize(frame.reason), `So the latest guess${frame.depth > 1 ? 'es' : ''} must be wrong (guess depth ${frame.depth}).`],
        blurb: '',
      };
    case 'backtrack': {
      const n = focus ? focus.ghosts.size : 0;
      return {
        title: `Undo guess ${frame.digit} at ${cellOf(frame.row, frame.col)}`,
        chip: 'Wrong turn',
        style: SEARCH_STYLE.undo,
        lines: [`Clearing the guess and the ${plural(Math.max(n - 1, 0), 'cell')} it forced. The crossed-out digits show what was removed.`],
        blurb: '',
      };
    }
    case 'solved':
      return { title: 'Solved', chip: 'Solved', style: SEARCH_STYLE.solved, lines: ['The search reached a complete, valid grid.'], blurb: '' };
    default:
      return {
        title: 'Search trace cut off',
        chip: 'Trace cut off',
        style: SEARCH_STYLE.cut,
        lines: ['The solver kept searching, but only the first part of the search was recorded. Showing the final solution.'],
        blurb: '',
      };
  }
}
