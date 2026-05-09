from __future__ import annotations

import freeusd


def test_plug_load_all_runs() -> None:
    freeusd.plug.load_all()


def test_work_run_invokes() -> None:
    called: list[int] = []

    def job() -> None:
        called.append(1)

    freeusd.work.run(job)
    assert called == [1]


def test_trace_push_pop() -> None:
    freeusd.trace.push("scope")
    freeusd.trace.pop()
