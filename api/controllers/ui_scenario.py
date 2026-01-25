from __future__ import annotations

import json

from fastapi import Depends, Request
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from core.dependencies import get_db
from core.templates import templates
from models.instrument import Instrument
from models.reagent import Reagent
from services import artist as artist_service
from services import scenario as scenario_service
from services import scenario_run as run_service
from services.utils_instruments import split_instruments_by_container
from schemas.scenario_screen import ScenarioScreenRead


def _parse_filter_params(params) -> tuple[str, bool, list[int]]:
    query = (params.get("query") or "").strip()
    show_inactive = (params.get("show_inactive") or "").lower() in {"1", "true", "on", "yes"}
    artist_ids: list[int] = []
    for raw_id in params.getlist("artists"):
        try:
            artist_ids.append(int(raw_id))
        except (TypeError, ValueError):
            continue
    return query, show_inactive, artist_ids


def list_scenarios_page(request: Request, db: Session = Depends(get_db)) -> HTMLResponse:
    query, show_inactive, artist_ids = _parse_filter_params(request.query_params)
    scenarios = scenario_service.list_scenarios_filtered(
        db, query=query, show_inactive=show_inactive, artist_ids=artist_ids
    )
    artists = artist_service.list_artists(db)
    selected_artists = [artist for artist in artists if artist.id in set(artist_ids)]
    return templates.TemplateResponse(
        "scenarios/list.html",
        {
            "request": request,
            "scenarios": scenarios,
            "artists": artists,
            "selected_artists": selected_artists,
            "query": query,
            "show_inactive": show_inactive,
        },
    )


def list_scenarios_partial(
    request: Request, params, db: Session = Depends(get_db)
) -> HTMLResponse:
    query, show_inactive, artist_ids = _parse_filter_params(params)
    scenarios = scenario_service.list_scenarios_filtered(
        db, query=query, show_inactive=show_inactive, artist_ids=artist_ids
    )
    return templates.TemplateResponse(
        "scenarios/partials/list.html",
        {
            "request": request,
            "scenarios": scenarios,
        },
    )


def new_scenario_modal(request: Request, db: Session = Depends(get_db)) -> HTMLResponse:
    return templates.TemplateResponse(
        "scenarios/partials/scenario_modal.html",
        {
            "request": request,
            "scenario": None,
            "modal_title": "Criar cenário",
            "submit_label": "Criar",
            "submit_url": "/ui/scenarios",
        },
    )


def edit_scenario_modal(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_by_id(db, scenario_id)
    return templates.TemplateResponse(
        "scenarios/partials/scenario_modal.html",
        {
            "request": request,
            "scenario": scenario,
            "modal_title": "Editar cenário",
            "submit_label": "Salvar",
            "submit_url": f"/ui/scenarios/{scenario_id}",
        },
    )


def delete_scenario_modal(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_by_id(db, scenario_id)
    return templates.TemplateResponse(
        "scenarios/partials/delete_modal.html",
        {
            "request": request,
            "scenario": scenario,
        },
    )


async def create_scenario_action(
    request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    form = await request.form()
    title = (form.get("title") or "").strip()
    description = (form.get("description") or "").strip()
    is_active = (form.get("is_active") or "").lower() in {"1", "true", "on", "yes"}
    artist_id_raw = (form.get("artist_id") or "").strip()
    artist_id = int(artist_id_raw) if artist_id_raw.isdigit() else None
    scenario_service.create_scenario(
        db=db,
        title=title or "Novo cenário",
        description=description,
        is_active=is_active,
        artist_id=artist_id,
    )
    return list_scenarios_partial(request, form, db)


async def update_scenario_action(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    form = await request.form()
    title = (form.get("title") or "").strip()
    description = (form.get("description") or "").strip()
    is_active = (form.get("is_active") or "").lower() in {"1", "true", "on", "yes"}
    artist_id_raw = (form.get("artist_id") or "").strip()
    artist_id = int(artist_id_raw) if artist_id_raw.isdigit() else None
    scenario = scenario_service.get_scenario_by_id(db, scenario_id)
    scenario_service.update_scenario(
        db=db,
        scenario_id=scenario_id,
        title=title or scenario.title,
        description=description,
        is_active=is_active,
    )
    if scenario.artist_id != artist_id:
        scenario.artist_id = artist_id
        db.add(scenario)
        db.commit()
        db.refresh(scenario)
    return list_scenarios_partial(request, form, db)


async def delete_scenario_action(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    form = await request.form()
    scenario_service.delete_scenario(db, scenario_id)
    return list_scenarios_partial(request, form, db)


def run_scenario_page(
    scenario_id: int, request: Request, db: Session = Depends(get_db)
) -> HTMLResponse:
    scenario = scenario_service.get_scenario_with_screens(db, scenario_id)
    screens = list(scenario.screens) if scenario.screens else []
    screens = sorted(screens, key=lambda screen: screen.order_index)
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
            "artist_id": scenario.artist_id,
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
    screens = sorted(screens, key=lambda screen: screen.order_index)
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
    screens = sorted(screens, key=lambda screen: screen.order_index)
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
    screen = next((item for item in screens if item.order_index == index), None)
    if not screen:
        return HTMLResponse(content="Tela não encontrada.", status_code=404)
    return templates.TemplateResponse(
        "scenario_runs/partials/builder_canvas.html",
        {"request": request, "screen": screen, "index": index},
    )
