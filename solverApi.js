// Bridge between the browser UI and the C++ programs in this folder.
//
//   GET  /api/health    -> can we start sudoku_solver (new enough to speak --json) and qqwing?
//   GET  /api/config    -> rating weights + thresholds (from `sudoku_solver --dump-config`)
//   POST /api/generate  -> { difficulty }  runs `qqwing --generate 1 --difficulty <d> --one-line`
//   POST /api/analyze   -> { puzzle }      runs `sudoku_solver --json --puzzle <81 chars>`
//
// Programs are started with execFile and an argument array (no shell), and every input is validated first.
//
// Optional environment variables: SOLVER_PATH, QQWING_PATH (full paths to the executables).

import { execFile } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.dirname(fileURLToPath(import.meta.url));
const IS_WIN = process.platform === 'win32';
const LEVELS = ['simple', 'easy', 'intermediate', 'expert'];
const PUZZLE_RE = /^[0-9.]{81}$/;

class HttpError extends Error {
  constructor(status, message, extra) {
    super(message);
    this.status = status;
    if (extra) Object.assign(this, extra);
  }
}

// ---------------------------------------------------------------- locating programs

function findBinary(envVar, baseName) {
  if (process.env[envVar]) return process.env[envVar];
  const local = path.join(ROOT, IS_WIN ? `${baseName}.exe` : baseName);
  if (fs.existsSync(local)) return local;
  return baseName; // fall back to whatever is on PATH
}
const solverPath = () => findBinary('SOLVER_PATH', 'sudoku_solver');
const qqwingPath = () => findBinary('QQWING_PATH', 'qqwing');

// If ratings.cfg sits next to this file, use it so edits to the weights/thresholds take effect.
function configArgs() {
  const cfg = path.join(ROOT, 'ratings.cfg');
  return fs.existsSync(cfg) ? ['--config', cfg] : [];
}

function run(bin, args, { timeoutMs = 30000, signal } = {}) {
  return new Promise((resolve, reject) => {
    execFile(
      bin,
      args,
      { cwd: ROOT, timeout: timeoutMs, signal, maxBuffer: 16 * 1024 * 1024, windowsHide: true, encoding: 'utf8' },
      (err, stdout, stderr) => {
        if (!err) return resolve(stdout);
        const name = path.basename(bin);
        if (err.name === 'AbortError' || err.code === 'ABORT_ERR') return reject(new HttpError(499, 'Request cancelled.'));
        if (err.code === 'ENOENT') {
          return reject(
            new HttpError(
              500,
              `Could not start "${name}". Put ${IS_WIN ? `${name.replace(/\.exe$/i, '')}.exe` : name} in the project folder, ` +
                `or set the ${name.startsWith('qq') ? 'QQWING_PATH' : 'SOLVER_PATH'} environment variable.`,
              { notFound: true }
            )
          );
        }
        if (err.killed || err.signal) return reject(new HttpError(504, `${name} took too long and was stopped.`));
        reject(new HttpError(500, (stderr || stdout || err.message || 'Unknown error').trim()));
      }
    );
  });
}

// ---------------------------------------------------------------- parsing

// Parses `sudoku_solver --json` output: a JSON array with one result per puzzle.
export function parseAnalysis(stdout) {
  let data;
  try {
    data = JSON.parse(stdout);
  } catch {
    throw new HttpError(
      500,
      'sudoku_solver did not return JSON, so it is probably an old build. Rebuild it from the current source (run.bat does this).'
    );
  }
  const r = Array.isArray(data) ? data[0] : null;
  if (!r || typeof r.puzzle !== 'string' || !Array.isArray(r.steps) || typeof r.outcome !== 'string') {
    throw new HttpError(500, 'Unexpected output from sudoku_solver. Rebuild it from the current source (run.bat does this).');
  }
  return r;
}

// Parses the `key = value` text printed by `sudoku_solver --dump-config`.
export function parseConfig(text) {
  const cfg = {
    weights: {},
    searchBase: 40,
    searchPerDepth: 5,
    thresholds: {
      difficulty: { medium: 1.2, high: 14 },
      length: { medium: 56, high: 58 },
      complexity: { medium: 65, high: 82 },
    },
  };
  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (!line || line.startsWith('#')) continue;
    const eq = line.lastIndexOf('=');
    if (eq < 0) continue;
    const key = line.slice(0, eq).trim();
    const value = Number(line.slice(eq + 1).trim());
    if (Number.isNaN(value)) continue;
    if (key.startsWith('weight.')) cfg.weights[key.slice(7)] = value;
    else if (key === 'search.base') cfg.searchBase = value;
    else if (key === 'search.perDepth') cfg.searchPerDepth = value;
    else if (key === 'difficulty.medium') cfg.thresholds.difficulty.medium = value;
    else if (key === 'difficulty.high') cfg.thresholds.difficulty.high = value;
    else if (key === 'length.medium') cfg.thresholds.length.medium = value;
    else if (key === 'length.long') cfg.thresholds.length.high = value;
    else if (key === 'complexity.medium') cfg.thresholds.complexity.medium = value;
    else if (key === 'complexity.high') cfg.thresholds.complexity.high = value;
  }
  return cfg;
}

// ---------------------------------------------------------------- route handlers

async function health() {
  const check = async (fn, onBadExit) => {
    try {
      await fn();
      return { ok: true };
    } catch (e) {
      // The program started but failed.
      if (!e.notFound && e.status === 500 && onBadExit) return onBadExit(e);
      return { ok: false, error: e.message };
    }
  };
  return {
    // With no puzzle, the solver uses its built-in sample, so this proves it can start AND speaks --json.
    // (An old build prints its usage text and exits with an error, or prints non-JSON text.)
    solver: await check(
      async () => parseAnalysis(await run(solverPath(), ['--json'], { timeoutMs: 15000 })),
      () => ({
        ok: false,
        error: 'sudoku_solver started but does not support --json, so it is probably an old build. Rebuild it from the current source (run.bat does this).',
      })
    ),
    // qqwing may not have a --version flag; being able to start it is enough.
    qqwing: await check(() => run(qqwingPath(), ['--version'], { timeoutMs: 10000 }), () => ({ ok: true })),
  };
}

async function getConfig() {
  const out = await run(solverPath(), [...configArgs(), '--dump-config'], { timeoutMs: 10000 });
  return parseConfig(out);
}

async function generate(body, signal) {
  const level = String(body && body.difficulty ? body.difficulty : '').toLowerCase();
  if (!LEVELS.includes(level)) throw new HttpError(400, `difficulty must be one of: ${LEVELS.join(', ')}`);
  // Expert puzzles are found by rejection sampling, so allow plenty of time.
  const out = await run(qqwingPath(), ['--generate', '1', '--difficulty', level, '--one-line'], { timeoutMs: 120000, signal });
  const puzzle = out
    .split(/\r?\n/)
    .map((s) => s.trim())
    .find((s) => PUZZLE_RE.test(s));
  if (!puzzle) throw new HttpError(500, 'qqwing did not return a puzzle.');
  return { puzzle: puzzle.replace(/0/g, '.'), difficulty: level };
}

async function analyze(body, signal) {
  const puzzle = String(body && body.puzzle ? body.puzzle : '').replace(/\s+/g, '').replace(/0/g, '.');
  if (!PUZZLE_RE.test(puzzle)) throw new HttpError(400, 'puzzle must be 81 characters: digits 1-9, with . for blanks.');
  const out = await run(solverPath(), ['--json', ...configArgs(), '--puzzle', puzzle], { timeoutMs: 30000, signal });
  return parseAnalysis(out);
}

// ---------------------------------------------------------------- http plumbing

function readJson(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let size = 0;
    req.on('data', (c) => {
      size += c.length;
      if (size > 65536) {
        reject(new HttpError(413, 'Request too large.'));
        req.destroy();
        return;
      }
      chunks.push(c);
    });
    req.on('end', () => {
      try {
        resolve(chunks.length ? JSON.parse(Buffer.concat(chunks).toString('utf8')) : {});
      } catch {
        reject(new HttpError(400, 'Invalid JSON.'));
      }
    });
    req.on('error', reject);
  });
}

function send(res, status, data) {
  res.statusCode = status;
  res.setHeader('Content-Type', 'application/json');
  res.end(JSON.stringify(data));
}

export function apiMiddleware() {
  return async (req, res, next) => {
    const url = (req.url || '').split('?')[0];
    if (!url.startsWith('/api/')) return next();

    // If the browser gives up (e.g. the user cancels a slow expert generation), stop the child process.
    const abort = new AbortController();
    res.on('close', () => {
      if (!res.writableEnded) abort.abort();
    });

    try {
      if (req.method === 'GET' && url === '/api/health') return send(res, 200, await health());
      if (req.method === 'GET' && url === '/api/config') return send(res, 200, await getConfig());
      if (req.method === 'POST' && url === '/api/generate') return send(res, 200, await generate(await readJson(req), abort.signal));
      if (req.method === 'POST' && url === '/api/analyze') return send(res, 200, await analyze(await readJson(req), abort.signal));
      throw new HttpError(404, 'Unknown API route.');
    } catch (err) {
      if (res.writableEnded || res.destroyed) return;
      send(res, err.status || 500, { error: err.message || 'Unexpected error.' });
    }
  };
}
