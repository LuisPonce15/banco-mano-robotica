#!/usr/bin/env python3
"""Guarda en CSV los datos que manda el firmware fsr_ensayo por USB.

Uso:
    python3 registrar_serial.py /dev/cu.usbserial-0001 ensayo.csv
    python3 registrar_serial.py COM5 ensayo.csv --cmd "r 3 0.5 2 3"

Lee las líneas D,... y las escribe con el mismo formato que exporta la página.
Las líneas que empiezan con # se muestran en pantalla. Ctrl+C para terminar.
Con --cmd manda ese comando al empezar y termina solo al recibir "FIN del ensayo".
"""
import argparse
import csv
import sys
import time

import serial  # pip install pyserial

HEADER = ["t_s", "z_mm", "F1_N", "F2_N", "F_N", "V_mV", "R_ohm", "fase", "ciclo"]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("puerto")
    ap.add_argument("salida")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--cmd", help="comando a mandar al empezar, por ejemplo 'r 3 0.5 2 3'")
    args = ap.parse_args()

    rm, suma, vcc = 10000.0, True, 3300.0
    with serial.Serial(args.puerto, args.baud, timeout=0.2) as port, open(args.salida, "w", newline="") as f:
        time.sleep(2.0)                      # el ESP32 se reinicia al abrir el puerto
        port.reset_input_buffer()
        port.write(b"s\n")
        if args.cmd:
            port.write((args.cmd + "\n").encode())
        w = csv.writer(f)
        w.writerow(HEADER)
        t0, n = None, 0
        try:
            while True:
                line = port.readline().decode(errors="replace").strip()
                if not line:
                    continue
                if line.startswith("S,"):
                    p = line.split(",")
                    rm, suma = float(p[9]), p[10] == "suma"
                elif line.startswith("D,"):
                    p = line.split(",")
                    t = int(p[1]) / 1000.0
                    t0 = t if t0 is None else t0
                    z, f1, f2, mv = map(float, p[2:6])
                    F = f1 + f2 if suma else f1
                    R = rm * (vcc - mv) / mv if mv > 1 else float("inf")
                    w.writerow([f"{t - t0:.3f}", z, f1, f2, f"{F:.4f}", mv, f"{R:.1f}", p[6], p[7]])
                    n += 1
                    if n % 50 == 0:
                        print(f"\r{n} muestras · F = {F:7.3f} N · V = {mv:6.0f} mV", end="", file=sys.stderr)
                else:
                    print("\n" + line)
                    if args.cmd and "FIN del ensayo" in line:
                        break
        except KeyboardInterrupt:
            pass
    print(f"\nGuardado {args.salida} ({n} muestras)")


if __name__ == "__main__":
    main()
