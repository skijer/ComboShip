"""Protect source files when a repair writes a separate destination."""
from pathlib import Path


def require_separate_destination(destination, *inputs):
    destination = Path(destination)
    for source in inputs:
        if source is None:
            continue
        source = Path(source)
        if (destination.resolve() == source.resolve() or
                (destination.exists() and source.exists() and destination.samefile(source))):
            raise ValueError('Destination must be separate from every protected input')
