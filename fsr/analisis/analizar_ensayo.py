#!/usr/bin/env python3
"""Análisis de un ensayo del FSR: curva, ajustes, histéresis y repetibilidad.

Uso:
    python3 analizar_ensayo.py ensayo.csv [--fmin 0.2] [--sin-graficas]

El CSV es el que exporta la página (pestaña Ensayo) o registrar_serial.py:
    t_s, z_mm, F1_N, F2_N, F_N, V_mV, R_ohm, fase, ciclo
fase: 1 = cargando, 2 = pausa con carga, 3 = descargando, 4 = pausa sin carga.
"""
import argparse
import csv
import math
import sys

import numpy as np


def load(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        sys.exit("El CSV está vacío")
    col = lambda k: np.array([float(r[k]) if r[k] not in ("", "inf") else np.inf for r in rows])
    return {k: col(k) for k in rows[0].keys()}


def fit_power(F, R):
    """R = A · F^(−n)  →  log R = log A − n log F"""
    x, y = np.log(F), np.log(R)
    b, a = np.polyfit(x, y, 1)
    pred = a + b * x
    r2 = 1 - np.sum((y - pred) ** 2) / np.sum((y - y.mean()) ** 2)
    return math.exp(a), -b, r2


def fit_linear(F, G):
    """G = k · F + G0 (conductancia en µS)"""
    k, g0 = np.polyfit(F, G, 1)
    pred = k * F + g0
    r2 = 1 - np.sum((G - pred) ** 2) / np.sum((G - G.mean()) ** 2)
    return k, g0, r2


def hysteresis(F, V, fase, bins=25):
    """Máxima diferencia de V entre carga (fase 1) y descarga (fase 3) a igual fuerza, en % del rango de V."""
    up, dn = fase == 1, fase == 3
    if up.sum() < 5 or dn.sum() < 5:
        return float("nan")
    lo, hi = max(F[up].min(), F[dn].min()), min(F[up].max(), F[dn].max())
    if hi <= lo:
        return float("nan")
    edges = np.linspace(lo, hi, bins + 1)
    diffs = []
    for a, b in zip(edges[:-1], edges[1:]):
        mu, md = (F >= a) & (F < b) & up, (F >= a) & (F < b) & dn
        if mu.sum() and md.sum():
            diffs.append(abs(V[md].mean() - V[mu].mean()))
    span = np.nanmax(V) - np.nanmin(V)
    return 100 * max(diffs) / span if diffs and span > 0 else float("nan")


def repeatability(F, V, fase, ciclo, bins=20):
    """Desviación estándar entre ciclos de V (carga) a igual fuerza, promedio en % del rango."""
    cs = [c for c in np.unique(ciclo) if c > 0]
    if len(cs) < 2:
        return float("nan")
    up = fase == 1
    edges = np.linspace(np.percentile(F[up], 5), np.percentile(F[up], 95), bins + 1)
    sds = []
    for a, b in zip(edges[:-1], edges[1:]):
        means = [V[(F >= a) & (F < b) & up & (ciclo == c)].mean() for c in cs
                 if ((F >= a) & (F < b) & up & (ciclo == c)).sum()]
        if len(means) >= 2:
            sds.append(np.std(means, ddof=1))
    span = np.nanmax(V) - np.nanmin(V)
    return 100 * float(np.mean(sds)) / span if sds and span > 0 else float("nan")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--fmin", type=float, default=0.2, help="fuerza mínima para los ajustes (N)")
    ap.add_argument("--sin-graficas", action="store_true")
    args = ap.parse_args()

    d = load(args.csv)
    F, V, R = d["F_N"], d["V_mV"], d["R_ohm"]
    fase, ciclo = d["fase"].astype(int), d["ciclo"].astype(int)
    ok = (F > args.fmin) & np.isfinite(R) & (R > 0)

    print(f"Muestras: {len(F)} · duración {d['t_s'][-1]:.1f} s · fuerza máx. {np.nanmax(F):.2f} N")
    if ok.sum() < 10:
        sys.exit("Muy pocos puntos por encima de --fmin para ajustar.")
    A, n, r2p = fit_power(F[ok], R[ok])
    k, g0, r2l = fit_linear(F[ok], 1e6 / R[ok])
    print(f"Ajuste potencia:   R = {A:.0f} Ω · F^(−{n:.3f})      R² = {r2p:.4f}")
    print(f"Ajuste lineal:     G = {k:.2f} µS/N · F + {g0:.2f} µS   R² = {r2l:.4f}")
    print(f"Histéresis:        {hysteresis(F, V, fase):.1f} % del rango de V")
    print(f"Repetibilidad:     {repeatability(F, V, fase, ciclo):.2f} % del rango de V (entre ciclos)")

    if args.sin_graficas:
        return
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(2, 2, figsize=(11, 8))
    ax[0, 0].plot(d["t_s"], F, label="F total")
    ax[0, 0].set(xlabel="t (s)", ylabel="F (N)", title="Fuerza")
    a2 = ax[0, 0].twinx(); a2.plot(d["t_s"], V, color="tab:orange", alpha=.7); a2.set_ylabel("V FSR (mV)")
    for c in sorted(set(ciclo) - {0}):
        m = ciclo == c
        ax[0, 1].plot(F[m], V[m], label=f"ciclo {c}")
    ax[0, 1].set(xlabel="F (N)", ylabel="V (mV)", title="Curva del FSR"); ax[0, 1].legend(fontsize=8)
    Fs = np.linspace(F[ok].min(), F[ok].max(), 100)
    ax[1, 0].loglog(F[ok], R[ok], ".", ms=2); ax[1, 0].loglog(Fs, A * Fs ** -n, "k-")
    ax[1, 0].set(xlabel="F (N)", ylabel="R FSR (Ω)", title=f"R = {A:.0f}·F^-{n:.2f}  (R²={r2p:.3f})")
    ax[1, 1].plot(F[ok], 1e6 / R[ok], ".", ms=2); ax[1, 1].plot(Fs, k * Fs + g0, "k-")
    ax[1, 1].set(xlabel="F (N)", ylabel="G (µS)", title=f"G = {k:.1f}·F + {g0:.1f}  (R²={r2l:.3f})")
    for a in ax.flat:
        a.grid(alpha=.3)
    fig.tight_layout()
    out = args.csv.rsplit(".", 1)[0] + "_analisis.png"
    fig.savefig(out, dpi=150)
    print(f"Gráficas: {out}")
    plt.show()


if __name__ == "__main__":
    main()
