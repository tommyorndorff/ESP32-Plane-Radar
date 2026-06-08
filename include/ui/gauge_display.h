#pragma once

namespace ui {

/** Full redraw: dark background, sparkline arc, CFS, stage, temp. */
void gaugeDisplayDraw();

/** Redraw data values only (background arc already drawn). */
void gaugeDisplayRefresh();

}  // namespace ui
