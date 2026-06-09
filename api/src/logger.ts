// Leveled, tagged logger for the API. Matches firmware log style.
//   log.info("WEATHER", "fetched %s", desc)
//   log.error("EMAIL", "imap failed", err)
// Min level via env LOG_LEVEL=debug|info|warn|error (default info).

import type { MiddlewareHandler } from "hono";

const LEVELS = { error: 0, warn: 1, info: 2, debug: 3 } as const;
type Level = keyof typeof LEVELS;

const COLORS: Record<Level, string> = {
  error: "\x1b[31m",
  warn: "\x1b[33m",
  info: "\x1b[32m",
  debug: "\x1b[90m",
};
const RESET = "\x1b[0m";
const NO_COLOR = !!process.env.NO_COLOR;

const min: Level = (process.env.LOG_LEVEL as Level) in LEVELS
  ? (process.env.LOG_LEVEL as Level)
  : "info";

function emit(level: Level, tag: string, args: unknown[]) {
  if (LEVELS[level] > LEVELS[min]) return;
  const ts = new Date().toISOString();
  const head = `[${ts} ${tag} ${level.toUpperCase().padEnd(5)}]`;
  const line = [head, ...args.map(fmt)].join(" ");
  const out = NO_COLOR ? line : `${COLORS[level]}${line}${RESET}`;
  (level === "error" ? console.error : console.log)(out);
}

function fmt(a: unknown): string {
  if (a instanceof Error) return a.stack ?? a.message;
  if (typeof a === "object") return JSON.stringify(a);
  return String(a);
}

export const log = {
  error: (tag: string, ...a: unknown[]) => emit("error", tag, a),
  warn: (tag: string, ...a: unknown[]) => emit("warn", tag, a),
  info: (tag: string, ...a: unknown[]) => emit("info", tag, a),
  debug: (tag: string, ...a: unknown[]) => emit("debug", tag, a),
};

// Request logger middleware: method, path, status, duration, client IP.
export const requestLogger: MiddlewareHandler = async (c, next) => {
  const t0 = performance.now();
  const ip =
    c.req.header("x-forwarded-for")?.split(",")[0]?.trim() ??
    (c.env as any)?.remoteAddr ??
    "-";
  await next();
  const ms = (performance.now() - t0).toFixed(1);
  const s = c.res.status;
  const level: Level = s >= 500 ? "error" : s >= 400 ? "warn" : "info";
  emit(level, "HTTP", [`${c.req.method} ${c.req.path} -> ${s} ${ms}ms ip=${ip}`]);
};
