"""
Oscil E2E Test Suite

This package provides end-to-end testing capabilities for the Oscil plugin
using the OscilTestHarness and its HTTP API.
"""

from .oscil_client import OscilClient, OscilClientError
from .log_analyzer import LogAnalyzer, LogEntry, LogQuery
from .assertions import OscilAssertions, create_assertions

__all__ = [
    "OscilClient",
    "OscilClientError",
    "LogAnalyzer",
    "LogEntry",
    "LogQuery",
    "OscilAssertions",
    "create_assertions",
]















