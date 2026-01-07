from __future__ import annotations

from typing import Sequence

from sqlalchemy.orm import Session

from core.exceptions import NotFoundError
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


def list_scenarios_by_artist(db: Session, artist_id: int) -> list[Scenario]:
    return (
        db.query(Scenario)
        .filter(Scenario.artist_id == artist_id)
        .order_by(Scenario.created_at.desc())
        .all()
    )


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
