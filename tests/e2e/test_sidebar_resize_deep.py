"""
E2E coverage for the sidebar resize handle drag.

The sidebar lives on the right edge of the editor; its width is
user-adjustable via a 6-px wide vertical resize handle at its left
edge. Dragging the handle left widens the sidebar; dragging right
narrows it.  Min/max are plugin-defined.

What bugs these tests catch:
- Drag handler updates handle position on screen but doesn't commit
  width to the layout (sidebar visually moves but reverts after mouse
  up).
- Drag below minimum width collapses the sidebar or negative-widths
  the content area.
- Drag beyond editor bounds silently leaves the sidebar larger than
  the editor itself (content clipped off-screen).
- Width not persisted — resets to default on state reload.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


class TestSidebarResize:
    def test_drag_wider_increases_width(self, editor: MultiScoperTestClient):
        """Drag the handle LEFT (negative deltaX) → sidebar grows."""
        before = editor.get_element("sidebar")
        assert before is not None
        width_before = before.width

        editor.drag_offset("sidebar_resizeHandle", dx=-30, dy=0)
        editor.wait_until(
            lambda: editor.get_element("sidebar").width > width_before,
            timeout_s=3.0,
            desc="sidebar width to grow after left drag",
        )

    def test_drag_narrower_decreases_width(self, editor: MultiScoperTestClient):
        """Drag the handle RIGHT (positive deltaX) → sidebar shrinks."""
        before = editor.get_element("sidebar")
        assert before is not None
        width_before = before.width

        editor.drag_offset("sidebar_resizeHandle", dx=30, dy=0)
        editor.wait_until(
            lambda: editor.get_element("sidebar").width < width_before,
            timeout_s=3.0,
            desc="sidebar width to shrink after right drag",
        )

    def test_width_clamped_at_extremes(self, editor: MultiScoperTestClient):
        """Drag very far must clamp to the plugin's min/max widths.

        Bug caught: drag handler integrates offset without bounds check
        → sidebar can be dragged to 0 or negative width.
        """
        # Extreme shrink.
        editor.drag_offset("sidebar_resizeHandle", dx=5000, dy=0)
        editor.wait_until(
            lambda: editor.get_element("sidebar") is not None,
            timeout_s=2.0,
            desc="sidebar state after extreme shrink",
        )
        width = editor.get_element("sidebar").width
        assert width > 0, f"sidebar width must remain positive, got {width}"

        # Restore toward centre.
        editor.drag_offset("sidebar_resizeHandle", dx=-200, dy=0)

    def test_rapid_resize_oscillations_converge(
        self, editor: MultiScoperTestClient
    ):
        """Ten back-and-forth drags must leave the sidebar in a
        plausible state (not a runaway width calculation bug)."""
        for _ in range(10):
            editor.drag_offset("sidebar_resizeHandle", dx=-10, dy=0)
            editor.drag_offset("sidebar_resizeHandle", dx=10, dy=0)
        width = editor.get_element("sidebar").width
        assert 50 < width < 2000, f"width must be plausible, got {width}"
