// Email source — MOCK for now. Your agent will produce this shape later.
import { log } from "./logger";

export type EmailLatest = {
  from: string;
  subject: string;
  date: string;   // "09/06 09:31"
  reason: string; // why it matters (from the agent)
};

export type Bmo = {
  status: "ok" | "attention" | "urgent";
  message: string;
};

export type EmailBlock = {
  email: {
    important_count: number;
    urgent_count: number;
    latest: EmailLatest[];
  };
  bmo: Bmo;
};

const POOL: EmailLatest[] = [
  { from: "Banco", subject: "Confirmação necessária", date: "09/06 09:31", reason: "Pede ação do usuário" },
  { from: "Linear", subject: "CYD-1 assigned to you", date: "09/06 08:50", reason: "Tarefa atribuída a ti" },
  { from: "AWS", subject: "Billing alert: budget 80%", date: "09/06 07:12", reason: "Custo acima do esperado" },
  { from: "Mae", subject: "liga-me quando puderes", date: "08/06 21:40", reason: "Pessoal" },
  { from: "Vercel", subject: "Deploy succeeded", date: "08/06 19:05", reason: "Informativo" },
  { from: "GitHub", subject: "PR #42 merged", date: "08/06 17:22", reason: "Code review pronto" },
];

function shuffle<T>(a: T[]): T[] {
  const r = [...a];
  for (let i = r.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [r[i], r[j]] = [r[j], r[i]];
  }
  return r;
}

// Returns the { email, bmo } block. Swap this body for your agent's output.
export async function getEmailBlock(): Promise<EmailBlock> {
  const latest = shuffle(POOL).slice(0, 5);
  const urgent_count = Math.random() < 0.5 ? 1 : 0;
  const important_count = 1 + Math.floor(Math.random() * 3);

  const status: Bmo["status"] = urgent_count > 0 ? "urgent" : important_count > 0 ? "attention" : "ok";
  const message =
    status === "urgent"
      ? `Leleu, tem ${urgent_count} email urgente a precisar de ação agora.`
      : status === "attention"
        ? `Leleu, tem ${important_count} email que parece precisar de ação hoje.`
        : "Tudo tranquilo, nada urgente.";

  log.debug("EMAIL", `mock block: imp=${important_count} urg=${urgent_count} status=${status}`);
  // accents kept — firmware now uses a custom font with Latin-1 glyphs
  return { email: { important_count, urgent_count, latest }, bmo: { status, message } };
}

// Mark everything as read. Swap this body for your agent's real action later.
export async function markAllRead(): Promise<EmailBlock> {
  // TODO: call agent / Gmail to actually mark read.
  log.info("EMAIL", "mark all read");
  return {
    email: { important_count: 0, urgent_count: 0, latest: [] },
    bmo: { status: "ok", message: "Tudo lido, sem pendências. 🎉".replace(" 🎉", "") },
  };
}
