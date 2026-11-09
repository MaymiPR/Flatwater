"""Fits the reference H-Delay's tap pad (reference/hdelay/notes: x_3_hover released, x_4_down pressed; git-ignored) as a
shading model: per state, the colour as a smooth function of the depth in from the pad's rounded outline
(0.5 px bins) and of the angle around its centre (1, cos, sin, cos 2, sin 2, cos 4, sin 4). The maker's mark in the
middle is masked out. It prints a report (the fit's error) and writes nothing: the editor draws the pad in code from
the measured geometry (its outline, face and corners).

    python -I plugins/FwDelay/scripts/fit_tap_pad.py
"""
import math
import numpy as np
from PIL import Image

X0, Y0, X1, Y1, RAD = 24.0, 59.0, 118.0, 155.0, 6.0      # the outline's outer edge (black, measured) and its corners
FX0, FY0, FX1, FY1, FRAD = 36.5, 72.5, 103.5, 140.5, 5.0  # the face's frame (its outer light line, measured)
CX, CY = (X0 + X1) / 2, (Y0 + Y1) / 2
STEP, NBIN = 0.5, 96 + 70
NB = 8


def depth(x, y, x0=X0, y0=Y0, x1=X1, y1=Y1, rad=RAD):
    """distance in from a rounded rectangle's edge (positive inside)"""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2 - rad, (y1 - y0) / 2 - rad
    qx, qy = np.abs(x - cx) - hx, np.abs(y - cy) - hy
    out = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - rad
    return -out


def basis(x, y, zone_face):
    """the four sides' weights (soft: by which straight edge is nearest) and a linear change along each side"""
    x0 = np.where(zone_face, FX0, X0); y0 = np.where(zone_face, FY0, Y0); x1 = np.where(zone_face, FX1, X1); y1 = np.where(zone_face, FY1, Y1)
    d = np.stack([y - y0, y1 - y, x - x0, x1 - x], axis=-1)                 # top, bottom, left, right
    dm = d.min(axis=-1, keepdims=True)
    w = np.exp(-(d - dm) / (0.8 + 0.3 * np.maximum(dm, 0))); w /= w.sum(axis=-1, keepdims=True)   # softer deeper in
    u = np.stack([(x - (x0 + x1) / 2) / ((x1 - x0) / 2)] * 2 + [(y - (y0 + y1) / 2) / ((y1 - y0) / 2)] * 2, axis=-1)
    return np.concatenate([w, w * u], axis=-1)


def fit(path):
    I = np.asarray(Image.open(path).convert('RGB')).astype(float)
    ys, xs = np.mgrid[int(Y0) - 1:int(Y1) + 1, int(X0) - 1:int(X1) + 1]
    x = xs + 0.5; y = ys + 0.5
    # two zones: the body, by depth in from the pad's outline; the face, by depth in from its frame (stored 96 bins on)
    df = depth(x, y, FX0, FY0, FX1, FY1, FRAD)
    d = np.where(df > 0, 48.0 + df, np.minimum(depth(x, y), 47.5))
    Bm = basis(x, y, df > 0)
    mark = (np.abs(x - 69.5) < 22) & (np.abs(y - 105.5) < 18)
    V = I[ys, xs]
    out = np.zeros((NBIN, 3, NB))
    for b in range(NBIN):
        c = b * STEP
        m = (np.abs(d - c) < STEP) & ~mark & (d > -1.0)
        prev = out[b - 1] if b > 0 else None
        if m.sum() < 30 and prev is not None:
            out[b] = prev; continue
        X = Bm[m]
        # ridge toward the previous bin: a side the bin hardly sees keeps its neighbour's shading
        lam = 2.0
        for ch in range(3):
            c0 = prev[ch] if prev is not None else np.zeros(NB)
            out[b, ch] = np.linalg.solve(X.T @ X + lam * np.eye(NB), X.T @ V[m][:, ch] + lam * c0)
    # how well it reproduces the pad (outside the mark)
    rec = np.zeros_like(V)
    bi = np.clip(np.round(d / STEP).astype(int), 0, NBIN - 1)
    for ch in range(3): rec[..., ch] = np.einsum('ijk,ijk->ij', out[bi, ch], Bm)
    inside = (depth(x, y) > 0) & ~mark
    print('%s: rms %.2f' % (path.split('/')[-1], np.sqrt(np.mean((rec[inside] - V[inside]) ** 2))))
    return out


def main():
    up = fit('reference/hdelay/notes/x_3_hover.png')
    down = fit('reference/hdelay/notes/x_4_down.png')
    # a report only: the editor draws the pad in code from the measured geometry (plugins/FwDelay/Source/PluginEditor.cpp)
    print('released %s, pressed %s coefficients' % (up.shape, down.shape))


if __name__ == '__main__':
    main()
