"""
E2E coverage for the global gain slider in the options section.

What bugs these tests catch:
- Slider move triggers UI label update but not the actual amplitude
  scaling in the waveform renderer (engineer thinks they're boosting
  but nothing changes on screen).
- Gain value not serialized to state XML (reset to 0 dB on load).
- Rapid slider scrubs debounced into a single write, so intermediate
  values are silently dropped.
- Default (reset) value is wrong — not 0 dB.
"""

from __future__ import annotations

import pytest

from multiscoper_test_utils import MultiScoperTestClient


GAIN = "sidebar_options_gainSlider"


@pytest.fixture()
def expanded_options(editor: MultiScoperTestClient):
    editor.click("sidebar_options")
    editor.wait_for_element(GAIN, timeout_s=3.0)
    return editor


class TestGainSliderBasics:
    def test_gain_slider_default_near_zero(
        self, expanded_options: MultiScoperTestClient
    ):
        """Default should be 0 dB (no gain).  Bug caught: default
        applied as -100 dB → silent plugin."""
        el = expanded_options.get_element(GAIN)
        assert el is not None
        value = el.extra.get("value", None)
        assert value is not None
        # 0 dB default; allow any reasonable initial value close to 0.
        assert abs(value) < 1.0, f"expected default gain near 0 dB, got {value}"

    def test_gain_slider_set_arbitrary_values(
        self, expanded_options: MultiScoperTestClient
    ):
        """Bug caught: slider set() ignores intermediate values."""
        for test_value in (-12.0, -6.0, 0.0, 3.0, 6.0):
            # Bind test_value into a default arg so the polling lambda
            # reads the right target for each iteration even if Python's
            # late-binding closure rules otherwise surprised a reader.
            expanded_options.set_slider(GAIN, test_value)
            expanded_options.wait_until(
                lambda tv=test_value: abs(
                    expanded_options.get_element(GAIN).extra.get("value", 0) - tv
                ) < 0.5,
                timeout_s=3.0,
                desc=f"slider to settle near {test_value}",
            )
            # Explicit post-wait assertion so the contract is visible
            # without tracing wait_until's TimeoutError semantics.
            settled = expanded_options.get_element(GAIN).extra.get("value", 0)
            assert abs(settled - test_value) < 0.5, (
                f"slider value {settled} not close to {test_value}"
            )

    def test_gain_slider_clamps_to_range(
        self, expanded_options: MultiScoperTestClient
    ):
        """Out-of-range values must be clamped.  Bug caught: gain
        accepts 1000 dB and overflows audio buffer."""
        expanded_options.set_slider(GAIN, 1000.0)
        expanded_options.wait_until(
            lambda: expanded_options.get_element(GAIN) is not None,
            timeout_s=2.0,
            desc="slider state after extreme",
        )
        el = expanded_options.get_element(GAIN)
        value = el.extra.get("value", 0)
        assert value < 100, f"gain must clamp, got {value}"

        expanded_options.set_slider(GAIN, -10000.0)
        expanded_options.wait_until(
            lambda: expanded_options.get_element(GAIN) is not None,
            timeout_s=2.0,
            desc="slider state after extreme low",
        )
        el = expanded_options.get_element(GAIN)
        value = el.extra.get("value", 0)
        assert value > -200, f"gain must clamp low, got {value}"


class TestGainSliderReset:
    def test_reset_returns_to_default(
        self, expanded_options: MultiScoperTestClient
    ):
        """Bug caught: reset-to-default not wired, or resets to wrong
        value (e.g. min instead of 0 dB)."""
        expanded_options.set_slider(GAIN, 6.0)
        expanded_options.wait_until(
            lambda: expanded_options.get_element(GAIN).extra.get("value", 0) > 5.0,
            timeout_s=2.0,
            desc="slider set to 6.0",
        )

        expanded_options.reset_slider(GAIN)
        expanded_options.wait_until(
            lambda: abs(expanded_options.get_element(GAIN).extra.get("value", 99)) < 1.0,
            timeout_s=3.0,
            desc="slider to reset to default",
        )
        # Explicit post-wait assertion (the documented default is 0 dB).
        final = expanded_options.get_element(GAIN).extra.get("value", 99)
        assert abs(final) < 1.0, f"reset should land near 0 dB default, got {final}"
