// Names here must match the strings printed by the C++ solver exactly.

// One colour family per tier, used everywhere: board cells, outlines, timeline, legend, report.
//   color = text / strong colour, bg = cell tint, solid = outlines, timeline ticks and dots
export const TIER_STYLE = {
  0: { label: 'Other', short: 'Other', color: '#374151', bg: '#e5e7eb', solid: '#6b7280' },
  1: { label: 'Tier 1 · Basic', short: 'Basic', color: '#1e3a8a', bg: '#bfdbfe', solid: '#2563eb' },
  2: { label: 'Tier 2 · Intermediate', short: 'Intermediate', color: '#92400e', bg: '#fde68a', solid: '#d97706' },
  3: { label: 'Tier 3 · Advanced', short: 'Advanced', color: '#5b21b6', bg: '#ddd6fe', solid: '#7c3aed' },
  4: { label: 'Tier 4 · Search', short: 'Guessing', color: '#991b1b', bg: '#fecaca', solid: '#dc2626' },
};

// The parts of Tier 4 (backtracking) get their own looks so wrong turns stand out.
export const SEARCH_STYLE = {
  guess: { label: 'Guess', color: '#ffffff', bg: '#dc2626', solid: '#dc2626' },
  forced: { label: 'Forced by propagation', color: '#991b1b', bg: '#fecaca', solid: '#f87171' },
  dead: { label: 'Dead end', color: '#ffffff', bg: '#111827', solid: '#111827' },
  undo: { label: 'Undone (wrong turn)', color: '#374151', bg: '#e5e7eb', solid: '#9ca3af' },
  solved: { label: 'Solved', color: '#14532d', bg: '#bbf7d0', solid: '#16a34a' },
  cut: { label: 'Trace cut off', color: '#9a3412', bg: '#fed7aa', solid: '#ea580c' },
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

// The technique that REALLY made a logic step possible: for a placement that an elimination unlocked
// (e.g. an XY-Wing that turned a cell into a naked single) this is the XY-Wing, not the single.
export const reasonOf = (step) => step.root || step.method;
