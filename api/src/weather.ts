// Weather via Open-Meteo (free, no API key).
import { log } from "./logger";

export type Weather = {
  temp: number;
  feels: number;
  temp_min: number;
  temp_max: number;
  humidity: number;
  desc: string;
  icon: string; // clear|clouds|rain|snow|storm|fog
};

// WMO weather_code -> { icon, desc }
function mapCode(code: number): { icon: string; desc: string } {
  if (code === 0) return { icon: "clear", desc: "clear sky" };
  if (code === 1) return { icon: "clear", desc: "mostly clear" };
  if (code === 2) return { icon: "clouds", desc: "partly cloudy" };
  if (code === 3) return { icon: "clouds", desc: "overcast" };
  if (code === 45 || code === 48) return { icon: "fog", desc: "fog" };
  if (code >= 51 && code <= 57) return { icon: "rain", desc: "drizzle" };
  if (code >= 61 && code <= 67) return { icon: "rain", desc: "rain" };
  if (code >= 80 && code <= 82) return { icon: "rain", desc: "rain showers" };
  if (code >= 71 && code <= 77) return { icon: "snow", desc: "snow" };
  if (code === 85 || code === 86) return { icon: "snow", desc: "snow showers" };
  if (code === 95) return { icon: "storm", desc: "thunderstorm" };
  if (code === 96 || code === 99) return { icon: "storm", desc: "thunderstorm, hail" };
  return { icon: "clouds", desc: "unknown" };
}

// Cache so the ESP32's 60s /dashboard poll doesn't hit Open-Meteo every time,
// and a slow/failed upstream doesn't stall the device (stale data served on error).
const CACHE_MS = Number(process.env.WEATHER_CACHE_MS ?? 10 * 60 * 1000);
let cache: { at: number; data: Weather } | null = null;

export async function getWeather(): Promise<Weather | null> {
  if (cache && Date.now() - cache.at < CACHE_MS) return cache.data;

  const lat = process.env.LAT ?? "38.7223";
  const lon = process.env.LON ?? "-9.1393";
  const unit = process.env.TEMP_UNIT ?? "celsius";

  const url =
    `https://api.open-meteo.com/v1/forecast?latitude=${lat}&longitude=${lon}` +
    `&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code` +
    `&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto` +
    `&temperature_unit=${unit}`;

  const t0 = performance.now();
  try {
    const res = await fetch(url, { signal: AbortSignal.timeout(8000) });
    if (!res.ok) {
      log.error("WEATHER", `open-meteo HTTP ${res.status}`);
      return cache?.data ?? null;
    }
    const j: any = await res.json();
    const c = j.current;
    const d = j.daily ?? {};
    const { icon, desc } = mapCode(c.weather_code);
    const data: Weather = {
      temp: c.temperature_2m,
      feels: c.apparent_temperature,
      temp_min: d.temperature_2m_min?.[0] ?? c.temperature_2m,
      temp_max: d.temperature_2m_max?.[0] ?? c.temperature_2m,
      humidity: c.relative_humidity_2m,
      desc,
      icon,
    };
    cache = { at: Date.now(), data };
    log.info(
      "WEATHER",
      `${desc} ${c.temperature_2m}° (min ${d.temperature_2m_min?.[0]} / max ${d.temperature_2m_max?.[0]}, ${c.relative_humidity_2m}%) in ${(performance.now() - t0).toFixed(0)}ms`,
    );
    return data;
  } catch (err) {
    log.error("WEATHER", "fetch failed", err);
    if (cache) {
      log.warn("WEATHER", `serving stale cache (${Math.round((Date.now() - cache.at) / 1000)}s old)`);
      return cache.data;
    }
    return null;
  }
}
