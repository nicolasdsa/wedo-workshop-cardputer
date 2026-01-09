from __future__ import annotations

from typing import Sequence

from sqlalchemy.orm import Session, selectinload

from core.exceptions import NotFoundError
from models.scenario_screen import ScenarioScreen
from services.scenario_screen import create_screens_for_scenario
from models.scenario import Scenario
from models.scenario_step import ScenarioStep


def create_scenario(
    db: Session,
    title: str,
    description: str,
    is_active: bool = True,
    steps: Sequence[dict] | None = None,
    artist_id: int | None = None,
    screens: Sequence[dict] | None = None,
) -> Scenario:
    scenario = Scenario(
        title=title,
        description=description,
        is_active=is_active,
        artist_id=artist_id,
    )
    db.add(scenario)
    db.flush()

    if steps:
        _replace_steps(db, scenario, steps)
    if screens:
        create_screens_for_scenario(db, scenario, screens)

    db.commit()
    db.refresh(scenario)
    return scenario


def get_scenario_by_id(db: Session, scenario_id: int) -> Scenario:
    scenario = db.get(Scenario, scenario_id)
    if not scenario:
        raise NotFoundError("Cenário não encontrado.")
    return scenario


def list_scenarios(db: Session) -> list[Scenario]:
    return db.query(Scenario).order_by(Scenario.created_at.desc()).all()


def list_scenarios_filtered(
    db: Session,
    query: str | None = None,
    show_inactive: bool = False,
    artist_ids: Sequence[int] | None = None,
) -> list[Scenario]:
    scenarios_query = db.query(Scenario).options(selectinload(Scenario.artist))
    if query:
        scenarios_query = scenarios_query.filter(Scenario.title.ilike(f"%{query}%"))
    if not show_inactive:
        scenarios_query = scenarios_query.filter(Scenario.is_active.is_(True))
    if artist_ids:
        scenarios_query = scenarios_query.filter(Scenario.artist_id.in_(artist_ids))
    return scenarios_query.order_by(Scenario.updated_at.desc()).all()


def list_scenarios_by_artist(
    db: Session, artist_id: int, only_active: bool = False
) -> list[Scenario]:
    scenarios_query = db.query(Scenario).filter(Scenario.artist_id == artist_id)
    if only_active:
        scenarios_query = scenarios_query.filter(Scenario.is_active.is_(True))
    return scenarios_query.order_by(Scenario.created_at.desc()).all()


def update_scenario(
    db: Session,
    scenario_id: int,
    title: str | None = None,
    description: str | None = None,
    is_active: bool | None = None,
    steps: Sequence[dict] | None = None,
) -> Scenario:
    scenario = get_scenario_by_id(db, scenario_id)

    if title is not None:
        scenario.title = title
    if description is not None:
        scenario.description = description
    if is_active is not None:
        scenario.is_active = is_active

    if steps is not None:
        _replace_steps(db, scenario, steps)

    db.add(scenario)
    db.commit()
    db.refresh(scenario)
    return scenario


def delete_scenario(db: Session, scenario_id: int) -> None:
    scenario = get_scenario_by_id(db, scenario_id)
    db.delete(scenario)
    db.commit()


def get_scenario_with_steps(db: Session, scenario_id: int) -> Scenario:
    scenario = db.query(Scenario).filter(Scenario.id == scenario_id).first()
    if not scenario:
        raise NotFoundError("Cenário não encontrado.")
    return scenario


def get_scenario_with_screens(db: Session, scenario_id: int) -> Scenario:
    scenario = (
        db.query(Scenario)
        .options(
            selectinload(Scenario.screens).selectinload(ScenarioScreen.components),
            selectinload(Scenario.screens).selectinload(ScenarioScreen.slider_images),
        )
        .filter(Scenario.id == scenario_id)
        .first()
    )
    if not scenario:
        raise NotFoundError("Cenário não encontrado.")
    return scenario


def _replace_steps(db: Session, scenario: Scenario, steps: Sequence[dict]) -> None:
    scenario.steps.clear()
    for step_data in sorted(steps, key=lambda s: s.get("order_index", 0)):
        scenario.steps.append(
            ScenarioStep(
                scenario_id=scenario.id,
                order_index=step_data["order_index"],
                action_type=step_data["action_type"],
                instrument_id=step_data.get("instrument_id"),
                reagent_id=step_data.get("reagent_id"),
                source_container_name=step_data.get("source_container_name"),
                target_container_name=step_data.get("target_container_name"),
                amount_value=step_data.get("amount_value"),
                amount_unit=step_data.get("amount_unit"),
                text_instruction=step_data["text_instruction"],
                sound_effect_path=step_data.get("sound_effect_path"),
            )
        )
