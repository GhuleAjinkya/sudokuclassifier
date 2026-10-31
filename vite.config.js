import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import { apiMiddleware } from './solverApi.js';

// The /api routes (see solverApi.js) run inside the Vite dev server, so a single
// `npm run dev` starts both the UI and the bridge to sudoku_solver / qqwing.
function solverApiPlugin() {
  return {
    name: 'sudoku-solver-api',
    configureServer(server) {
      server.middlewares.use(apiMiddleware());
    },
  };
}

export default defineConfig({
  plugins: [react(), solverApiPlugin()],
  server: { port: 5173, open: true },
});
