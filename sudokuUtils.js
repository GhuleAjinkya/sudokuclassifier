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
