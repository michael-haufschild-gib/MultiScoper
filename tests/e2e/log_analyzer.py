"""
LogAnalyzer - Parser and analyzer for Oscil debug logs.

The debug log contains JSON entries with hypothesis IDs, locations,
messages, data, and timestamps. This module provides tools for:
- Parsing log entries
- Filtering by hypothesis ID, location, or message content
- Extracting execution paths
- Verifying expected behaviors occurred
"""

import json
import re
import os
from pathlib import Path
from typing import Optional, Dict, Any, List, Callable
from dataclasses import dataclass, field
from datetime import datetime


@dataclass
class LogEntry:
    """Represents a single debug log entry."""
    hypothesis_id: str
    location: str
    message: str
    data: Dict[str, Any]
    timestamp: int
    raw: str = ""
    
    @classmethod
    def from_json(cls, json_str: str) -> Optional["LogEntry"]:
        """Parse a JSON log entry."""
        try:
            obj = json.loads(json_str.strip())
            return cls(
                hypothesis_id=obj.get("hypothesisId", ""),
                location=obj.get("location", ""),
                message=obj.get("message", ""),
                data=obj.get("data", {}),
                timestamp=obj.get("timestamp", 0),
                raw=json_str.strip()
            )
        except json.JSONDecodeError:
            return None
    
    def timestamp_datetime(self) -> datetime:
        """Convert timestamp to datetime."""
        return datetime.fromtimestamp(self.timestamp / 1000.0)
    
    def matches_hypothesis(self, hypothesis_id: str) -> bool:
        """Check if this entry matches a hypothesis ID (supports comma-separated)."""
        if not hypothesis_id:
            return True
        entry_ids = [h.strip() for h in self.hypothesis_id.split(",")]
        return hypothesis_id in entry_ids
    
    def matches_location(self, location_pattern: str) -> bool:
        """Check if location matches a pattern (supports wildcards)."""
        if not location_pattern:
            return True
        pattern = location_pattern.replace("*", ".*")
        return bool(re.search(pattern, self.location, re.IGNORECASE))
    
    def matches_message(self, message_pattern: str) -> bool:
        """Check if message matches a pattern."""
        if not message_pattern:
            return True
        return bool(re.search(message_pattern, self.message, re.IGNORECASE))


@dataclass
class LogQuery:
    """Query builder for filtering log entries."""
    hypothesis_id: Optional[str] = None
    location: Optional[str] = None
    message: Optional[str] = None
    data_filter: Optional[Callable[[Dict], bool]] = None
    after_timestamp: Optional[int] = None
    before_timestamp: Optional[int] = None
    
    def matches(self, entry: LogEntry) -> bool:
        """Check if an entry matches this query."""
        if self.hypothesis_id and not entry.matches_hypothesis(self.hypothesis_id):
            return False
        if self.location and not entry.matches_location(self.location):
            return False
        if self.message and not entry.matches_message(self.message):
            return False
        if self.data_filter and not self.data_filter(entry.data):
            return False
        if self.after_timestamp and entry.timestamp < self.after_timestamp:
            return False
        if self.before_timestamp and entry.timestamp > self.before_timestamp:
            return False
        return True


@dataclass
class ExecutionPath:
    """Represents a sequence of log entries forming an execution path."""
    entries: List[LogEntry] = field(default_factory=list)
    
    def add(self, entry: LogEntry):
        """Add an entry to the path."""
        self.entries.append(entry)
    
    @property
    def locations(self) -> List[str]:
        """Get all locations in order."""
        return [e.location for e in self.entries]
    
    @property
    def messages(self) -> List[str]:
        """Get all messages in order."""
        return [e.message for e in self.entries]
    
    def contains_sequence(self, locations: List[str]) -> bool:
        """Check if the path contains a sequence of locations in order."""
        if not locations:
            return True
        path_locations = self.locations
        idx = 0
        for loc in path_locations:
            if locations[idx] in loc:
                idx += 1
                if idx >= len(locations):
                    return True
        return False
    
    def duration_ms(self) -> int:
        """Get duration from first to last entry in milliseconds."""
        if len(self.entries) < 2:
            return 0
        return self.entries[-1].timestamp - self.entries[0].timestamp


class LogAnalyzer:
    """
    Analyzer for Oscil debug logs.
    
    Usage:
        analyzer = LogAnalyzer("/path/to/debug.log")
        entries = analyzer.filter(hypothesis_id="H4")
        path = analyzer.extract_path("H4")
        assert path.contains_sequence(["refreshPanels", "updateWaveform"])
    """
    
    DEFAULT_LOG_PATH = "/Users/Spare/Documents/code/MultiScoper/.cursor/debug.log"
    
    def __init__(self, log_path: Optional[str] = None):
        """
        Initialize the analyzer.
        
        Args:
            log_path: Path to the debug log file
        """
        self.log_path = Path(log_path or self.DEFAULT_LOG_PATH)
        self._entries: List[LogEntry] = []
        self._loaded = False
    
    def reload(self) -> "LogAnalyzer":
        """Reload log entries from file."""
        self._entries = []
        self._loaded = False
        return self.load()
    
    def load(self) -> "LogAnalyzer":
        """Load log entries from file (lazy loading)."""
        if self._loaded:
            return self
        
        if not self.log_path.exists():
            self._entries = []
            self._loaded = True
            return self
        
        self._entries = []
        with open(self.log_path, "r") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                entry = LogEntry.from_json(line)
                if entry:
                    self._entries.append(entry)
        
        self._loaded = True
        return self
    
    def clear(self):
        """Clear the log file and cached entries."""
        if self.log_path.exists():
            with open(self.log_path, "w") as f:
                f.write("")
        self._entries = []
        self._loaded = True
    
    @property
    def entries(self) -> List[LogEntry]:
        """Get all log entries."""
        self.load()
        return self._entries
    
    def get_all(self) -> List[Dict[str, Any]]:
        """Get all log entries as dictionaries."""
        self.load()
        return [
            {
                "hypothesisId": e.hypothesis_id,
                "location": e.location,
                "message": e.message,
                "data": e.data,
                "timestamp": e.timestamp,
            }
            for e in self._entries
        ]
    
    @property
    def count(self) -> int:
        """Get the number of log entries."""
        return len(self.entries)
    
    def filter(
        self,
        hypothesis_id: Optional[str] = None,
        location: Optional[str] = None,
        message: Optional[str] = None,
        data_filter: Optional[Callable[[Dict], bool]] = None,
        after_timestamp: Optional[int] = None,
        before_timestamp: Optional[int] = None
    ) -> List[LogEntry]:
        """Filter log entries by criteria."""
        query = LogQuery(
            hypothesis_id=hypothesis_id,
            location=location,
            message=message,
            data_filter=data_filter,
            after_timestamp=after_timestamp,
            before_timestamp=before_timestamp
        )
        return [e for e in self.entries if query.matches(e)]
    
    def find_first(
        self,
        hypothesis_id: Optional[str] = None,
        location: Optional[str] = None,
        message: Optional[str] = None
    ) -> Optional[LogEntry]:
        """Find the first matching entry."""
        matches = self.filter(hypothesis_id=hypothesis_id, location=location, message=message)
        return matches[0] if matches else None
    
    def find_last(
        self,
        hypothesis_id: Optional[str] = None,
        location: Optional[str] = None,
        message: Optional[str] = None
    ) -> Optional[LogEntry]:
        """Find the last matching entry."""
        matches = self.filter(hypothesis_id=hypothesis_id, location=location, message=message)
        return matches[-1] if matches else None
    
    def count_matches(
        self,
        hypothesis_id: Optional[str] = None,
        location: Optional[str] = None,
        message: Optional[str] = None
    ) -> int:
        """Count matching entries."""
        return len(self.filter(hypothesis_id=hypothesis_id, location=location, message=message))
    
    def extract_path(self, hypothesis_id: str) -> ExecutionPath:
        """Extract execution path for a hypothesis."""
        path = ExecutionPath()
        for entry in self.filter(hypothesis_id=hypothesis_id):
            path.add(entry)
        return path
    
    def extract_all_paths(self) -> Dict[str, ExecutionPath]:
        """Extract execution paths for all hypothesis IDs."""
        paths: Dict[str, ExecutionPath] = {}
        for entry in self.entries:
            for h_id in entry.hypothesis_id.split(","):
                h_id = h_id.strip()
                if h_id not in paths:
                    paths[h_id] = ExecutionPath()
                paths[h_id].add(entry)
        return paths
    
    # =========================================================================
    # Assertion Helpers
    # =========================================================================
    
    def assert_called(
        self,
        location: str,
        message: Optional[str] = None,
        times: Optional[int] = None,
        at_least: Optional[int] = None,
        at_most: Optional[int] = None
    ) -> "LogAnalyzer":
        """Assert that a location was called."""
        count = self.count_matches(location=location, message=message)
        
        if times is not None and count != times:
            raise AssertionError(
                f"Expected {location} to be called {times} time(s), but was called {count} time(s)"
            )
        if at_least is not None and count < at_least:
            raise AssertionError(
                f"Expected {location} to be called at least {at_least} time(s), but was called {count} time(s)"
            )
        if at_most is not None and count > at_most:
            raise AssertionError(
                f"Expected {location} to be called at most {at_most} time(s), but was called {count} time(s)"
            )
        if times is None and at_least is None and at_most is None and count == 0:
            raise AssertionError(f"Expected {location} to be called, but it was never called")
        
        return self
    
    def assert_not_called(self, location: str, message: Optional[str] = None) -> "LogAnalyzer":
        """Assert that a location was not called."""
        count = self.count_matches(location=location, message=message)
        if count > 0:
            raise AssertionError(f"Expected {location} to not be called, but it was called {count} time(s)")
        return self
    
    def assert_sequence(self, locations: List[str], hypothesis_id: Optional[str] = None) -> "LogAnalyzer":
        """Assert that locations were called in sequence."""
        if hypothesis_id:
            path = self.extract_path(hypothesis_id)
        else:
            path = ExecutionPath(entries=self.entries)
        
        if not path.contains_sequence(locations):
            actual = path.locations
            raise AssertionError(
                f"Expected sequence {locations} not found in actual path:\n{actual}"
            )
        return self
    
    def assert_data_contains(
        self,
        location: str,
        key: str,
        expected_value: Any = None
    ) -> "LogAnalyzer":
        """Assert that a log entry contains specific data."""
        entries = self.filter(location=location)
        if not entries:
            raise AssertionError(f"No log entries found for location {location}")
        
        for entry in entries:
            if key in entry.data:
                if expected_value is None or entry.data[key] == expected_value:
                    return self
        
        if expected_value is not None:
            raise AssertionError(
                f"No log entry at {location} has data[{key}] == {expected_value}"
            )
        else:
            raise AssertionError(f"No log entry at {location} has key {key}")
    
    def get_data_values(self, location: str, key: str) -> List[Any]:
        """Get all values for a data key from matching entries."""
        entries = self.filter(location=location)
        values = []
        for entry in entries:
            if key in entry.data:
                values.append(entry.data[key])
        return values
    
    def get_unique_data_values(self, location: str, key: str) -> set:
        """Get unique values for a data key."""
        return set(self.get_data_values(location, key))
    
    # =========================================================================
    # Pretty Printing
    # =========================================================================
    
    def print_entries(
        self,
        hypothesis_id: Optional[str] = None,
        location: Optional[str] = None,
        limit: int = 50
    ):
        """Print log entries for debugging."""
        entries = self.filter(hypothesis_id=hypothesis_id, location=location)[:limit]
        for entry in entries:
            ts = entry.timestamp_datetime().strftime("%H:%M:%S.%f")[:-3]
            print(f"[{ts}] {entry.location}: {entry.message}")
            if entry.data:
                for k, v in entry.data.items():
                    print(f"    {k}: {v}")
    
    def print_summary(self):
        """Print a summary of log entries."""
        paths = self.extract_all_paths()
        print(f"Total entries: {self.count}")
        print(f"Hypothesis IDs: {len(paths)}")
        for h_id, path in sorted(paths.items()):
            locs = set(path.locations)
            print(f"  {h_id}: {len(path.entries)} entries across {len(locs)} locations")
    
    def to_dataframe(self):
        """Convert to pandas DataFrame (if pandas is available)."""
        try:
            import pandas as pd
            data = []
            for entry in self.entries:
                row = {
                    "timestamp": entry.timestamp,
                    "hypothesis_id": entry.hypothesis_id,
                    "location": entry.location,
                    "message": entry.message,
                    **{f"data_{k}": v for k, v in entry.data.items()}
                }
                data.append(row)
            return pd.DataFrame(data)
        except ImportError:
            raise ImportError("pandas is required for to_dataframe()")

