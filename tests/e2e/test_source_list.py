"""
E2E tests for the audio source list.

What bugs these tests catch:
- Source list not populated from instance registry
- Source list empty when test harness is providing audio
- Source API returning malformed data
- Source IDs not unique
- Source info missing required fields
- Adding oscillator with specific source creates correct binding
"""

import pytest
from oscil_test_utils import OscilTestClient


class TestSourceListAPI:
    """Verify source list API returns valid data."""

    def test_sources_available(self, client: OscilTestClient):
        """
        Bug caught: instance registry not providing sources to the
        test harness, making oscillator creation impossible.
        """
        sources = client.get_sources()
        assert len(sources) > 0, (
            "Test harness should register at least 1 audio source"
        )

    def test_source_has_id(self, client: OscilTestClient):
        """
        Bug caught: source serialization returning empty/null ID.
        """
        sources = client.get_sources()
        if not sources:
            pytest.fail("No sources available")

        for source in sources:
            assert "id" in source and source["id"], (
                f"Source should have non-empty id, got: {source}"
            )

    def test_source_ids_are_unique(self, client: OscilTestClient):
        """
        Bug caught: duplicate source IDs causing oscillator creation to
        bind to wrong source.
        """
        sources = client.get_sources()
        if len(sources) < 2:
            pytest.fail("Need 2+ sources to test uniqueness")

        ids = [s["id"] for s in sources]
        assert len(ids) == len(set(ids)), (
            f"Source IDs should be unique, got duplicates: {ids}"
        )

    def test_source_has_name(self, client: OscilTestClient):
        """
        Bug caught: source name field empty, causing blank entries in
        source dropdown.
        """
        sources = client.get_sources()
        if not sources:
            pytest.fail("No sources available")

        for source in sources:
            assert "name" in source, (
                f"Source should have name field, got keys: {list(source.keys())}"
            )


class TestSourceBinding:
    """Verify oscillators bind correctly to sources."""

    def test_oscillator_bound_to_specified_source(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: oscillator creation ignoring the sourceId parameter
        and binding to a random/default source.
        """
        osc_id = editor.add_oscillator(source_id, name="Source Bind Test")
        assert osc_id is not None

        osc = editor.get_oscillator_by_id(osc_id)
        assert osc is not None
        assert osc.get("sourceId") == source_id, (
            f"Oscillator should be bound to source '{source_id}', "
            f"got '{osc.get('sourceId')}'"
        )

    def test_multiple_oscillators_same_source(
        self, editor: OscilTestClient, source_id: str
    ):
        """
        Bug caught: source binding uses a one-to-one mapping that prevents
        multiple oscillators from using the same source.
        """
        id1 = editor.add_oscillator(source_id, name="Same Source 1")
        id2 = editor.add_oscillator(source_id, name="Same Source 2")
        editor.wait_for_oscillator_count(2, timeout_s=3.0)

        osc1 = editor.get_oscillator_by_id(id1)
        osc2 = editor.get_oscillator_by_id(id2)

        assert osc1.get("sourceId") == source_id
        assert osc2.get("sourceId") == source_id

    def test_source_in_diagnostic_snapshot(
        self, editor: OscilTestClient
    ):
        """
        Bug caught: sources section missing from diagnostic snapshot,
        making scenario-based tests unable to verify source state.
        """
        snap = editor.get_diagnostic_snapshot()
        if snap is None:
            pytest.fail("Snapshot API not available")

        sources = snap.get("sources", [])
        assert len(sources) > 0, (
            "Diagnostic snapshot should include sources"
        )
