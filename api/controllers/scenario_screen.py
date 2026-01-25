from __future__ import annotations

import json

from fastapi.responses import HTMLResponse
from sqlalchemy.orm import Session

from pydantic import BaseModel, Field

from schemas.scenario_screen import (
    ScenarioScreenComponentCreate,
    ScenarioScreenCreate,
    ScenarioScreenRead,
)
from services import scenario as scenario_service
from services import scenario_screen as scenario_screen_service


def get_screen(screen_id: int, db: Session):
    screen = scenario_screen_service.get_screen(db, screen_id)
    data = ScenarioScreenRead.model_validate(screen).model_dump(mode="json")
    return HTMLResponse(content=json.dumps(data), status_code=200)


def list_screens_for_scenario(scenario_id: int, db: Session):
    scenario_service.get_scenario_by_id(db, scenario_id)
    screens = scenario_screen_service.list_screens_for_scenario(db, scenario_id)
    data = [ScenarioScreenRead.model_validate(item).model_dump(mode="json") for item in screens]
    return HTMLResponse(content=json.dumps(data), status_code=200)


def create_screens_for_scenario(
    scenario_id: int, screens: list[ScenarioScreenCreate] | ScenarioScreenCreate, db: Session
):
    scenario = scenario_service.get_scenario_by_id(db, scenario_id)
    payload_list = screens if isinstance(screens, list) else [screens]
    created = scenario_screen_service.create_screens_for_scenario(
        db=db, scenario=scenario, screens_data=[screen.model_dump() for screen in payload_list]
    )
    db.commit()
    for screen in created:
        db.refresh(screen)
    data = [ScenarioScreenRead.model_validate(item).model_dump(mode="json") for item in created]
    return HTMLResponse(content=json.dumps(data), status_code=201)


class ScenarioScreenUpdatePayload(BaseModel):
    components: list[ScenarioScreenComponentCreate] = Field(default_factory=list)
    animation_key: str | None = None


class ScenarioScreenReorderPayload(BaseModel):
    screen_ids: list[int] = Field(default_factory=list)


def update_screen(
    screen_id: int, payload: ScenarioScreenUpdatePayload, db: Session
):
    updated = scenario_screen_service.update_screen_components(
        db=db,
        screen_id=screen_id,
        components=[component.model_dump() for component in payload.components],
        animation_key=payload.animation_key,
    )
    data = ScenarioScreenRead.model_validate(updated).model_dump(mode="json")
    return HTMLResponse(content=json.dumps(data), status_code=200)


def delete_screen(screen_id: int, db: Session):
    scenario_screen_service.delete_screen(db=db, screen_id=screen_id)
    return HTMLResponse(content="", status_code=204)


def reorder_screens(
    scenario_id: int, payload: ScenarioScreenReorderPayload, db: Session
):
    screens = scenario_screen_service.reorder_screens_for_scenario(
        db=db, scenario_id=scenario_id, screen_ids=payload.screen_ids
    )
    data = [ScenarioScreenRead.model_validate(item).model_dump(mode="json") for item in screens]
    return HTMLResponse(content=json.dumps(data), status_code=200)
