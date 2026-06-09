import { Hono } from "hono";
import { getWeather } from "./weather";
import { getEmailBlock, markAllRead, requestRefresh } from "./email";
import { log, requestLogger } from "./logger";

const app = new Hono();

app.use("*", requestLogger);

app.onError((err, c) => {
  log.error("APP", "unhandled error", err);
  return c.json({ error: "internal" }, 500);
});

app.get("/health", (c) => c.json({ ok: true }));

// Emails only — for on-device pull-to-refresh. Returns { email, bmo }.
app.get("/emails", async (c) => {
  const block = await getEmailBlock();
  log.info("EMAIL", `manual refresh -> ${block.email.latest.length} latest, status=${block.bmo.status}`);
  return c.json(block);
});

// Refresh — queue a re-triage action (refresh.json) and return current state.
app.post("/emails/refresh", async (c) => {
  const block = await requestRefresh();
  log.info("EMAIL", `refresh requested -> status=${block.bmo.status}`);
  return c.json(block);
});

// Mark all emails as read — returns the updated { email, bmo } block.
app.post("/emails/read", async (c) => {
  const block = await markAllRead();
  return c.json(block);
});

// Full dashboard — weather + email + bmo. Matches firmware src/data.h.
app.get("/dashboard", async (c) => {
  const [weather, block] = await Promise.all([getWeather(), getEmailBlock()]);
  log.debug("DASH", `weather=${weather ? "ok" : "null"} bmo=${block.bmo.status}`);
  return c.json({ weather, ...block }); // { weather, email, bmo }
});

const port = Number(process.env.PORT ?? 8000);
log.info("BOOT", `dashboard-cyd api listening on http://0.0.0.0:${port}`);

export default {
  port,
  hostname: "0.0.0.0", // bind all interfaces so ESP32 on LAN can reach it
  fetch: app.fetch,
};
