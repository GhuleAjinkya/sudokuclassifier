// Puzzles are 81-character strings: digits 1-9, and '.' for an empty cell (row by row).

export const EMPTY = '.'.repeat(81);

export const rowOf = (i) => Math.floor(i / 9);
export const colOf = (i) => i % 9;
export const boxOf = (i) => Math.floor(rowOf(i) / 3) * 3 + Math.floor(colOf(i) / 3);

// Accepts pasted text: strips whitespace and treats 0 as blank.
export function normalize(text) {
  return String(text).replace(/\s+/g, '').replace(/0/g, '.');
}

export const isPuzzleString = (s) => /^[1-9.]{81}$/.test(s);

export const sees = (a, b) =>
  a !== b && (rowOf(a) === rowOf(b) || colOf(a) === colOf(b) || boxOf(a) === boxOf(b));

// Indices of cells whose digit also appears in a peer (same row, column or box).
export function findConflicts(values) {
  const bad = new Set();
  for (let i = 0; i < 81; i++) {
    if (values[i] === '.') continue;
    for (let j = i + 1; j < 81; j++) {
      if (values[j] === values[i] && sees(i, j)) {
        bad.add(i);
        bad.add(j);
      }
    }
  }
  return bad;
}

export function countGivens(values) {
  let n = 0;
  for (const ch of values) if (ch !== '.') n++;
  return n;
}

// ---------------------------------------------------------------------------------------------
// The solver's placement log records digits but not the candidate eliminations (pairs, pointing,
// wings, ...) that happen between them. A placement labelled "Naked Single" may therefore only be a
// single *after* some unprinted elimination. auditPlacements() replays the log on a plain grid
// (givens + earlier placements, no eliminations) and flags those steps.
//
// Returns one entry per placement: null if the label holds on a plain grid, otherwise
// { candidates } listing what a person would still see in that cell.
const PEERS = Array.from({ length: 81 }, (_, i) => {
  const out = [];
  for (let j = 0; j < 81; j++) if (sees(i, j)) out.push(j);
  return out;
});
const UNITS = (() => {
  const u = [];
  for (let k = 0; k < 9; k++) {
    u.push(Array.from({ length: 9 }, (_, c) => k * 9 + c)); // row k
    u.push(Array.from({ length: 9 }, (_, r) => r * 9 + k)); // column k
    u.push(Array.from({ length: 9 }, (_, n) => (Math.floor(k / 3) * 3 + Math.floor(n / 3)) * 9 + (k % 3) * 3 + (n % 3))); // box k
  }
  return u;
})();
const UNITS_OF = Array.from({ length: 81 }, (_, i) => UNITS.filter((unit) => unit.includes(i)));

export function auditPlacements(puzzle, placements) {
  const g = puzzle.split('').map((ch) => (ch === '.' ? 0 : Number(ch)));
  const plain = (i) => {
    if (g[i]) return [];
    const used = new Set();
    for (const j of PEERS[i]) if (g[j]) used.add(g[j]);
    const out = [];
    for (let d = 1; d <= 9; d++) if (!used.has(d)) out.push(d);
    return out;
  };
  return placements.map((p) => {
    const i = p.row * 9 + p.col;
    const candidates = plain(i);
    let ok = true;
    if (p.method === 'Naked Single') {
      ok = candidates.length === 1 && candidates[0] === p.digit;
    } else if (p.method === 'Hidden Single') {
      ok = UNITS_OF[i].some((unit) => unit.every((j) => j === i || g[j] !== 0 || !plain(j).includes(p.digit)));
    }
    g[i] = p.digit;
    return ok ? null : { candidates };
  });
}
