from __future__ import annotations

from typing import Sequence

from sqlalchemy.orm import Session

from core.exceptions import NotFoundError
from models.scenario import Scenario
from models.scenario_screen import ScenarioScreen, ScenarioScreenType
from models.scenario_screen_slider_image import ScenarioScreenSliderImage
from models.scenario_screen_component import ScenarioScreenComponent


def create_screens_for_scenario(
    db: Session,
    scenario: Scenario,
    screens_data: Sequence[dict],
    commit: bool = False,
) -> list[ScenarioScreen]:
    created: list[ScenarioScreen] = []

    def _get_value(item, key, default=None):
        if isinstance(item, dict):
            return item.get(key, default)
        return getattr(item, key, default)

    def _z_index(item, default=0):
        try:
            value = _get_value(item, "z_index", default)
            return int(value) if value is not None else default
        except (TypeError, ValueError):
            return default

    for screen_data in sorted(
        screens_data,
        key=lambda s: s.get("order_index", 0) if isinstance(s, dict) else getattr(s, "order_index", 0),
    ):
        slider_images = _get_value(screen_data, "slider_images")
        components_data = _get_value(screen_data, "components") or []
        screen = ScenarioScreen(
            scenario_id=scenario.id,
            order_index=_get_value(screen_data, "order_index", 0),
            screen_type=_get_value(screen_data, "screen_type", ScenarioScreenType.CANVAS) or ScenarioScreenType.CANVAS,
            title=_get_value(screen_data, "title"),
            body_text=_get_value(screen_data, "body_text"),
            image_path=_get_value(screen_data, "image_path"),
            gif_path=_get_value(screen_data, "gif_path"),
            button_label=_get_value(screen_data, "button_label"),
            animation_key=_get_value(screen_data, "animation_key"),
        )
        db.add(screen)
        db.flush()

        if slider_images:
            for image_data in sorted(
                slider_images,
                key=lambda i: i.get("order_index", 0) if isinstance(i, dict) else getattr(i, "order_index", 0),
            ):
                screen.slider_images.append(
                    ScenarioScreenSliderImage(
                        screen_id=screen.id,
                        order_index=image_data["order_index"] if isinstance(image_data, dict) else image_data.order_index,
                        image_path=image_data["image_path"] if isinstance(image_data, dict) else image_data.image_path,
                        caption=image_data.get("caption") if isinstance(image_data, dict) else image_data.caption,
                    )
                )

        if components_data:
            for idx, component_data in enumerate(
                sorted(
                    components_data,
                    key=lambda c: _z_index(c),  # type: ignore
                )
            ):
                z_value = _get_value(component_data, "z_index")
                screen.components.append(
                    ScenarioScreenComponent(
                        screen_id=screen.id,
                        component_type=_get_value(component_data, "component_type"),
                        z_index=z_value if z_value is not None else idx,
                        x=float(_get_value(component_data, "x", 0.0)),
                        y=float(_get_value(component_data, "y", 0.0)),
                        w=float(_get_value(component_data, "w", 0.0)),
                        h=float(_get_value(component_data, "h", 0.0)),
                        rotate=_get_value(component_data, "rotate"),
                        props=_get_value(component_data, "props"),
                    )
                )

        created.append(screen)

    if commit:
        db.commit()
        for screen in created:
            db.refresh(screen)

    return created


def list_screens_for_scenario(db: Session, scenario_id: int) -> list[ScenarioScreen]:
    return (
        db.query(ScenarioScreen)
        .filter(ScenarioScreen.scenario_id == scenario_id)
        .order_by(ScenarioScreen.order_index)
        .all()
    )


def get_screen(db: Session, screen_id: int) -> ScenarioScreen:
    screen = db.get(ScenarioScreen, screen_id)
    if not screen:
        raise NotFoundError("Tela de cenário não encontrada.")
    return screen


def update_screen_components(
    db: Session, screen_id: int, components: Sequence[dict], animation_key: str | None = None
) -> ScenarioScreen:
    screen = get_screen(db, screen_id)
    if animation_key is not None:
        screen.animation_key = animation_key
    screen.components.clear()

    def _get_value(item, key, default=None):
        if isinstance(item, dict):
            return item.get(key, default)
        return getattr(item, key, default)

    def _z_index(item, default=0):
        try:
            value = _get_value(item, "z_index", default)
            return int(value) if value is not None else default
        except (TypeError, ValueError):
            return default

    for idx, comp in enumerate(
        sorted(components, key=lambda c: _z_index(c))  # type: ignore
    ):
        z_value = _get_value(comp, "z_index")
        screen.components.append(
            ScenarioScreenComponent(
                screen_id=screen.id,
                component_type=_get_value(comp, "component_type"),
                z_index=z_value if z_value is not None else idx,
                x=float(_get_value(comp, "x", 0.0)),
                y=float(_get_value(comp, "y", 0.0)),
                w=float(_get_value(comp, "w", 0.0)),
                h=float(_get_value(comp, "h", 0.0)),
                rotate=_get_value(comp, "rotate"),
                props=_get_value(comp, "props"),
            )
        )

    db.add(screen)
    db.commit()
    db.refresh(screen)
    return screen
