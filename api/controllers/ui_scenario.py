from __future__ import annotations

import json

from fastapi import Depends, Request
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from core.dependencies import get_db
from core.templates import templates
from models.instrument import Instrument
from models.reagent import Reagent
from services import scenario as scenario_service
from services import scenario_run as run_service
from services.utils_instruments import split_instruments_by_container
from schemas.scenario_screen import ScenarioScreenRead


def list_scenarios_page(request: Request, db: Session = Depends(get_db)) -> HTMLResponse:
    scenarios = [s for s in scenario_service.list_scenarios(db) if s.is_active]
    return templates.TemplateResponse(
        "scenarios/list.html",
        {
            "request": request,
            "scenarios": scenarios,
        },
    )


def run_scenario_page(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_with_screens(db, scenario_id)
    screens = list(scenario.screens) if scenario.screens else []
    screens_json = json.dumps(
        [ScenarioScreenRead.model_validate(screen).model_dump(mode="json") for screen in screens]
    )
    run_state = run_service.start_scenario_run(scenario_id, db=db)
    reagents = db.query(Reagent).all()
    all_instruments = db.query(Instrument).all()
    transfer_instruments, container_instruments = split_instruments_by_container(all_instruments)
    containers_meta = run_state.get("containers_meta", {})
    container_names = [
        name for name, meta in containers_meta.items() if meta.get("is_container")
    ]
    instrument_map = {inst.id: inst.name for inst in all_instruments}
    return templates.TemplateResponse(
        "scenario_runs/run.html",
        {
            "request": request,
            "scenario": scenario,
            "run_state": run_state,
            "reagents": reagents,
            "transfer_instruments": transfer_instruments,
            "container_names": container_names,
            "containers_meta": containers_meta,
            "instrument_map": instrument_map,
            "screens": screens,
            "screens_json": screens_json,
        },
    )


def scenario_screen_partial(
    scenario_id: int, index: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_with_screens(db, scenario_id)
    screens = list(scenario.screens) if scenario.screens else []
    if index < 0 or index >= len(screens):
        return HTMLResponse(content="Tela não encontrada.", status_code=404)
    screen = screens[index]
    return templates.TemplateResponse(
        "scenario_runs/partials/screen.html",
        {"request": request, "screen": screen},
    )


def scenario_builder_page(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_with_screens(db, scenario_id)
    screens = list(scenario.screens) if scenario.screens else []
    initial_screen = screens[0] if screens else None
    return templates.TemplateResponse(
        "scenario_runs/builder.html",
        {
            "request": request,
            "scenario": scenario,
            "screens": screens,
            "initial_screen": initial_screen,
        },
    )


def builder_screen_partial(
    scenario_id: int, index: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_with_screens(db, scenario_id)
    screens = list(scenario.screens) if scenario.screens else []
    if index < 0 or index >= len(screens):
        return HTMLResponse(content="Tela não encontrada.", status_code=404)
    screen = screens[index]
    return templates.TemplateResponse(
        "scenario_runs/partials/builder_canvas.html",
        {"request": request, "screen": screen, "index": index},
    )
