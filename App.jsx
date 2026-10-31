import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import Board from './Board.jsx';
import Generator from './Generator.jsx';
import Editor from './Editor.jsx';
import Visualizer from './Visualizer.jsx';
import Report from './Report.jsx';
import { api } from './api.js';
import { EMPTY, auditPlacements, colOf, countGivens, findConflicts, normalize, rowOf } from './sudokuUtils.js';
import { tierOf } from './techniqueInfo.js';

const NO_PLACEMENTS = [];
const LEVEL_NAMES = { simple: 'Simple', easy: 'Easy', intermediate: 'Intermediate', expert: 'Expert' };

export default function App() {
  const [tab, setTab] = useState('generate'); // 'generate' | 'edit'
  const [puzzle, setPuzzle] = useState(EMPTY);
  const [undoStack, setUndoStack] = useState([]);
  const [selected, setSelected] = useState(null);
  const [generated, setGenerated] = useState(null); // { puzzle, level }
  const [analysisRaw, setAnalysis] = useState(null);
  const [analyzing, setAnalyzing] = useState(false);
  const [generating, setGenerating] = useState(false);
  const [error, setError] = useState('');
  const [step, setStep] = useState(0);
  const [config, setConfig] = useState(null);
  const [health, setHealth] = useState(null);
  const analyzeAbort = useRef(null);
  const generateAbort = useRef(null);

  // Results only count while they still describe the puzzle on the board.
  const analysis = analysisRaw && analysisRaw.puzzle === puzzle ? analysisRaw : null;

  useEffect(() => {
    api.health().then(setHealth).catch((e) => setHealth({ solver: { ok: false, error: e.message }, qqwing: { ok: false } }));
    api.config().then(setConfig).catch(() => {});
  }, []);

  // A new result (or an edited puzzle) always starts the replay from the beginning.
  useEffect(() => {
    setStep(0);
  }, [analysis]);

  // ------------------------------------------------------------ talking to the solver

  const analyze = useCallback(async (p) => {
    if (analyzeAbort.current) analyzeAbort.current.abort();
    const ac = new AbortController();
    analyzeAbort.current = ac;
    setAnalyzing(true);
    setError('');
    try {
      const result = await api.analyze(p, ac.signal);
      if (!ac.signal.aborted) setAnalysis(result);
    } catch (e) {
      if (!ac.signal.aborted) setError(e.message);
    } finally {
      if (analyzeAbort.current === ac) setAnalyzing(false);
    }
  }, []);

  const generate = useCallback(
    async (level) => {
      if (generateAbort.current) generateAbort.current.abort();
      const ac = new AbortController();
      generateAbort.current = ac;
      setGenerating(true);
      setError('');
      try {
        const data = await api.generate(level, ac.signal);
        if (ac.signal.aborted) return;
        const p = normalize(data.puzzle);
        setPuzzle(p);
        setUndoStack([]);
        setSelected(null);
        setGenerated({ puzzle: p, level });
        setAnalysis(null);
        analyze(p);
      } catch (e) {
        if (!ac.signal.aborted) setError(e.message);
      } finally {
        if (generateAbort.current === ac) setGenerating(false);
      }
    },
    [analyze]
  );

  const cancelGenerate = () => {
    if (generateAbort.current) generateAbort.current.abort();
    setGenerating(false);
  };

  // ------------------------------------------------------------ editing

  const commit = useCallback(
    (next) => {
      if (next === puzzle) return;
      setUndoStack((s) => [...s.slice(-99), puzzle]);
      setPuzzle(next);
    },
    [puzzle]
  );
  const setCell = (i, ch) => commit(puzzle.slice(0, i) + ch + puzzle.slice(i + 1));
  const undo = () => {
    if (undoStack.length === 0) return;
    setPuzzle(undoStack[undoStack.length - 1]);
    setUndoStack(undoStack.slice(0, -1));
  };

  // ------------------------------------------------------------ what the board shows

  const placements = analysis ? analysis.placements : NO_PLACEMENTS;
  const { values, tiers, currentCell } = useMemo(() => {
    const vals = puzzle.split('');
    const tr = new Array(81).fill(null);
    let cur = null;
    for (let s = 0; s < step && s < placements.length; s++) {
      const p = placements[s];
      const i = p.row * 9 + p.col;
      vals[i] = String(p.digit);
      tr[i] = tierOf(p.method);
      cur = i;
    }
    return { values: vals.join(''), tiers: tr, currentCell: step > 0 ? cur : null };
  }, [puzzle, placements, step]);

  const audit = useMemo(() => auditPlacements(puzzle, placements), [puzzle, placements]);
  const givens = useMemo(() => puzzle.split('').map((ch) => ch !== '.'), [puzzle]);
  const conflicts = useMemo(() => findConflicts(puzzle), [puzzle]);
  const givenCount = countGivens(puzzle);
  const canEdit = tab === 'edit' && step === 0 && !generating;

  const onBoardKeyDown = (e) => {
    const arrows = { ArrowUp: -9, ArrowDown: 9, ArrowLeft: -1, ArrowRight: 1 };
    if (e.key in arrows) {
      e.preventDefault();
      const from = selected == null ? 0 : selected;
      const r = rowOf(from);
      const c = colOf(from);
      if (e.key === 'ArrowUp' && r > 0) setSelected(from - 9);
      else if (e.key === 'ArrowDown' && r < 8) setSelected(from + 9);
      else if (e.key === 'ArrowLeft' && c > 0) setSelected(from - 1);
      else if (e.key === 'ArrowRight' && c < 8) setSelected(from + 1);
      else if (selected == null) setSelected(0);
      return;
    }
    if (!canEdit || selected == null) return;
    if (/^[1-9]$/.test(e.key)) {
      e.preventDefault();
      setCell(selected, e.key);
    } else if (e.key === 'Backspace' || e.key === 'Delete' || e.key === '0' || e.key === ' ') {
      e.preventDefault();
      setCell(selected, '.');
    }
  };

  // ------------------------------------------------------------ labels

  let label = 'Empty grid';
  if (generated && generated.puzzle === puzzle) label = `Generated · ${LEVEL_NAMES[generated.level]} · ${givenCount} clues`;
  else if (givenCount > 0) label = `Custom puzzle · ${givenCount} clues`;

  const solverDown = health && !health.solver.ok;
  const analyzeDisabled = analyzing || generating || conflicts.size > 0 || givenCount === 0 || solverDown;

  return (
    <div className="app">
      <header className="topbar">
        <h1>Sudoku Classifier</h1>
        <p className="muted">Generate or build a puzzle, watch the solver work through it, and see how hard it really is.</p>
      </header>

      {solverDown && (
        <div className="banner error">
          <strong>The solver could not be started.</strong> {health.solver.error}
        </div>
      )}

      <main className="layout">
        <section className="card board-card" aria-label="Puzzle">
          <div className="tabs" role="tablist">
            <button type="button" role="tab" aria-selected={tab === 'generate'} className={tab === 'generate' ? 'tab active' : 'tab'} onClick={() => setTab('generate')}>
              Generate
            </button>
            <button type="button" role="tab" aria-selected={tab === 'edit'} className={tab === 'edit' ? 'tab active' : 'tab'} onClick={() => setTab('edit')}>
              Create / Edit
            </button>
          </div>

          {tab === 'generate' ? (
            <Generator
              busy={generating}
              qqwingOk={health ? health.qqwing.ok : undefined}
              hasPuzzle={givenCount > 0}
              onGenerate={generate}
              onCancel={cancelGenerate}
              onEdit={() => setTab('edit')}
            />
          ) : (
            <Editor
              puzzle={puzzle}
              selected={selected}
              canEdit={canEdit}
              conflictCount={conflicts.size}
              canUndo={undoStack.length > 0}
              onSetCell={setCell}
              onUndo={undo}
              onClear={() => commit(EMPTY)}
              onLoad={commit}
            />
          )}

          <div className="board-wrap">
            <Board
              values={values}
              givens={givens}
              tiers={tiers}
              selected={selected}
              conflicts={conflicts}
              currentCell={currentCell}
              onSelect={setSelected}
              onKeyDown={onBoardKeyDown}
            />
          </div>

          <div className="row between board-actions">
            <span className="muted small">{label}</span>
            <button type="button" className="primary" onClick={() => analyze(puzzle)} disabled={analyzeDisabled}>
              {analyzing ? 'Analyzing…' : analysis ? 'Re-analyze' : 'Analyze puzzle'}
            </button>
          </div>

          {error && <div className="notice error">{error}</div>}

          {analysis && analysis.placements.length > 0 && <Visualizer analysis={analysis} audit={audit} step={step} setStep={setStep} />}
          {analysis && analysis.placements.length === 0 && analysis.outcome !== 'invalid' && (
            <p className="muted small">Nothing to replay: the puzzle was already complete.</p>
          )}
        </section>

        <section className="card report-card" aria-label="Difficulty report">
          {config ? (
            <Report analysis={analysis} config={config} analyzing={analyzing} />
          ) : (
            <p className="muted">Loading rating settings…</p>
          )}
        </section>
      </main>
    </div>
  );
}
