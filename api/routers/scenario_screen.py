from fastapi import APIRouter, Depends
from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from controllers.scenario_screen import (
    ScenarioScreenUpdatePayload,
    create_screens_for_scenario,
    get_screen,
    list_screens_for_scenario,
    update_screen,
)
from schemas.scenario_screen import ScenarioScreenCreate
from core.dependencies import get_db


router = APIRouter(
    prefix="",
    tags=["Scenario Screens"],
    default_response_class=HTMLResponse,
)


@router.get("/scenario-screens/{screen_id}", response_class=HTMLResponse)
def get_screen_route(screen_id: int, db: Session = Depends(get_db)):
    return get_screen(screen_id, db)


@router.get("/scenarios/{scenario_id}/screens", response_class=HTMLResponse)
def list_screens_for_scenario_route(
    scenario_id: int, db: Session = Depends(get_db)
):
    return list_screens_for_scenario(scenario_id, db)


@router.post(
    "/scenarios/{scenario_id}/screens",
    response_class=HTMLResponse,
    status_code=201,
)
def create_screens_for_scenario_route(
    scenario_id: int,
    payload: list[ScenarioScreenCreate],
    db: Session = Depends(get_db),
):
    return create_screens_for_scenario(scenario_id, payload, db)


@router.patch(
    "/scenario-screens/{screen_id}",
    response_class=HTMLResponse,
)
def update_screen_route(
    screen_id: int,
    payload: ScenarioScreenUpdatePayload,
    db: Session = Depends(get_db),
):
    return update_screen(screen_id, payload, db)
