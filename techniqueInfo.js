// Names here must match the strings printed by the C++ solver exactly.

export const TIER_STYLE = {
  0: { label: 'Other', short: 'Other', color: '#374151', bg: '#e5e7eb' },
  1: { label: 'Tier 1 · Basic', short: 'Basic', color: '#1d4ed8', bg: '#dbeafe' },
  2: { label: 'Tier 2 · Intermediate', short: 'Intermediate', color: '#b45309', bg: '#fef3c7' },
  3: { label: 'Tier 3 · Advanced', short: 'Advanced', color: '#6d28d9', bg: '#ede9fe' },
  4: { label: 'Tier 4 · Search', short: 'Guessing', color: '#b91c1c', bg: '#fee2e2' },
};

export const SEARCH_NAME = 'Backtracking Search';

export const TECHNIQUES = {
  'Naked Single': { tier: 1, blurb: 'A cell that has only one digit left that could go in it.' },
  'Hidden Single': { tier: 1, blurb: 'A digit that has only one possible cell left in a row, column or box.' },
  'Naked Pair': { tier: 2, blurb: 'Two cells in a unit share the same two candidates, so those digits can be removed from the rest of the unit.' },
  'Naked Triple': { tier: 2, blurb: 'Three cells in a unit share only three candidates between them, so those digits can be removed elsewhere in the unit.' },
  'Naked Quad': { tier: 2, blurb: 'Four cells in a unit share only four candidates between them, so those digits can be removed elsewhere in the unit.' },
  'Hidden Pair': { tier: 2, blurb: 'Two digits fit in only the same two cells of a unit, so every other candidate can be removed from those cells.' },
  'Hidden Triple': { tier: 2, blurb: 'Three digits fit in only the same three cells of a unit, so every other candidate can be removed from those cells.' },
  'Hidden Quad': { tier: 2, blurb: 'Four digits fit in only the same four cells of a unit, so every other candidate can be removed from those cells.' },
  'Pointing Pair/Triple (Box -> Line)': { tier: 2, blurb: 'A digit confined to one line inside a box can be removed from the rest of that line.' },
  'Claiming (Line -> Box)': { tier: 2, blurb: 'A digit confined to one box within a line can be removed from the rest of that box.' },
  'X-Wing': { tier: 3, blurb: 'A digit appears in exactly two cells in each of two rows, lined up in the same two columns, so it can be removed from the rest of those columns (or the same idea with rows and columns swapped).' },
  'Swordfish': { tier: 3, blurb: 'Like an X-Wing, but spread across three rows and three columns.' },
  'Jellyfish': { tier: 3, blurb: 'Like an X-Wing, but spread across four rows and four columns.' },
  'XY-Wing': { tier: 3, blurb: 'A pivot cell with two candidates and two wing cells. Any cell that sees both wings cannot hold the digit the wings share.' },
  'XYZ-Wing': { tier: 3, blurb: 'A pivot with three candidates and two wings. Any cell that sees the pivot and both wings cannot hold their shared digit.' },
  'Skyscraper': { tier: 3, blurb: 'Two parallel lines each hold a digit in exactly two cells and share one end. Cells that see both free ends lose that digit.' },
  '2-String Kite': { tier: 3, blurb: 'A row and a column each hold a digit in exactly two cells, linked through one box. Cells that see both free ends lose that digit.' },
  'Simple Coloring': { tier: 3, blurb: 'Linked cells for one digit are colored in two alternating colors; a contradiction rules out a whole color.' },
  [SEARCH_NAME]: { tier: 4, blurb: 'Logic ran out, so the solver guesses a digit, follows the consequences, and undoes the guess if it hits a contradiction.' },
};

export const tierOf = (name) => (TECHNIQUES[name] ? TECHNIQUES[name].tier : 0);
export const blurbOf = (name) => (TECHNIQUES[name] ? TECHNIQUES[name].blurb : '');
